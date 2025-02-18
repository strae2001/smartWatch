#include <stdio.h>
#include "incl/Ntp_time.h"
#include "incl/myWIFI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_netif_sntp.h"
#include "esp_netif.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "soc/rtc.h"
#include "rtc.h"

static const char* TAG = "myNTP";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

#define CONFIG_EXAMPLE_CONNECT_WIFI     // 使用wifi连接网络
#define STORAGE_NAMESPACE           "storage"           // 自定义NVS命名空间

// sntp初始化标志位
bool sntp_inited_flag = 0;
// 获取sntp时间完成回调指针
static mySntp_done_t f_mySntp_done = NULL;
// 启动sntp同步标志
static bool mySntp_start_flag = false;

// 互斥信号量(解决nvs读写资源竞争)
static SemaphoreHandle_t nvs_MuxSem;

// 打印时间事件标志组
static EventGroupHandle_t printEventGp_handle;
static uint8_t PRINT_EVENTBIT = BIT1;

// 时间日期备份
static char mydate[32] = {0};
static char mytime[32] = {0};

/// @brief 获取网络时间并同步系统时间
/// @return 成功：ESP_OK
static esp_err_t obtain_time(void);

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}

static esp_err_t mySntp_init(void)
{
    esp_err_t ret;
#if LWIP_DHCP_GET_NTP_SRV 
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(3, ESP_SNTP_SERVER_LIST("cn.pool.ntp.org", "time.windows.com", "ntp.sjtu.edu.cn"));
    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    config.ip_event_to_renew = IP_EVENT_STA_GOT_IP;
#else
    config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;
#endif
    config.sync_cb = time_sync_notification_cb; // only if we need the notification function
    ret = esp_netif_sntp_init(&config);

#endif /* LWIP_DHCP_GET_NTP_SRV */

#if LWIP_DHCP_GET_NTP_SRV
    ESP_LOGI(TAG, "Starting SNTP");
    ret += esp_netif_sntp_start();
#endif
    print_servers();

    if(ret == ESP_OK) 
        return ret;
    return ESP_FAIL;
}

static void print_time_task(void* params)
{
    time_t now;
    struct tm timeinfo;
    static uint16_t cnt = 0;
    static bool is_fristPrint = true;
    static EventBits_t wait_bit;

    while (1)
    {
        wait_bit = xEventGroupWaitBits(printEventGp_handle, PRINT_EVENTBIT, pdTRUE, pdTRUE, portMAX_DELAY);
        if(wait_bit & PRINT_EVENTBIT)
        {
            time(&now);
            localtime_r(&now, &timeinfo);
            // Is time set? If not, tm_year will be (1970 - 1900).
            if (is_fristPrint || timeinfo.tm_year < (2016 - 1900)) {
                is_fristPrint = false;
                ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
                if(!sntp_inited_flag)
                {
                    if(mySntp_init() == ESP_OK)
                        sntp_inited_flag = 1;
                }
                if(obtain_time() != ESP_OK)
                {
                    ESP_LOGE(TAG, "obtain time failed!");
                }
                // update 'now' variable with current time
                time(&now);
            }

            char strftime_buf[64];

            // Set timezone to China Standard Time
            setenv("TZ", "CST-8", 1);     
            tzset();
            localtime_r(&now, &timeinfo);

            // 将时间戳写入nvs
            nvs_handle_t my_handle = 0;
            xSemaphoreTakeRecursive(nvs_MuxSem, portMAX_DELAY);     // 获取锁
            if(tim_write_into_nvs(&my_handle, &now) == ESP_OK)
                ESP_LOGI(TAG, "tim_write_into_nvs successful");
            else ESP_LOGE(TAG, "tim_write_into_nvs failed");
            xSemaphoreGiveRecursive(nvs_MuxSem);                    // 释放锁

            // 打印时间
            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            strftime(mydate, sizeof(mydate), "%Y-%m-%d  %a.", &timeinfo);
            strftime(mytime, sizeof(mytime), "%H:%M", &timeinfo);
            ESP_LOGI(TAG, "%dth: The current date/time in BeiJing is: %s", ++cnt, strftime_buf);
            if(1 == cnt)
                f_mySntp_done();    // 首次成功获取sntp时间后调用
        }
    }
}

static void access_print_now_time(void* args)
{
    xEventGroupSetBits(printEventGp_handle, PRINT_EVENTBIT);
}

void sntp_task(void* param)
{
    xEventGroupSetBits(printEventGp_handle, PRINT_EVENTBIT);     // 默认启动后打印一次时间
    xTaskCreatePinnedToCore(print_time_task, "print_time_task", 4096, NULL, 5, NULL, 1);
    
    vTaskDelete(NULL);
}

static esp_err_t obtain_time(void)
{
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    uint8_t retry = 0;
    const uint8_t retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    esp_netif_sntp_deinit();

    printf("after sync tim_stamp: %lld\r\n", now);

    if(retry == retry_count)
        return ESP_ERR_TIMEOUT;
    return  ESP_OK;
}

esp_err_t fetch_and_store_time_in_nvs(void *args)
{
    nvs_handle_t my_handle = 0;
    esp_err_t err;
    
    if(!sntp_inited_flag)
    {
        ESP_LOGE(TAG, "sntp don't init");
        err = ESP_FAIL;
        goto exit;
    }

    if (obtain_time() != ESP_OK) {      // 获取网络时间并同步
        err = ESP_FAIL;
        goto exit;
    }

    time_t now;
    time(&now);

    // 将时间戳写入nvs
    xSemaphoreTakeRecursive(nvs_MuxSem, portMAX_DELAY);     // 获取锁
    err = tim_write_into_nvs(&my_handle, &now);
    xSemaphoreGiveRecursive(nvs_MuxSem);                    // 释放锁
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "tim_write_into_nvs failed");
        goto exit;
    }

exit:
    if (my_handle != 0) {
        nvs_close(my_handle);
    }
    esp_netif_sntp_deinit();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error updating time in nvs");
    } else {
        ESP_LOGI(TAG, "Updated time in NVS");
    }
    return err;
}

esp_err_t update_time_from_nvs(void)
{
    nvs_handle_t my_handle = 0;
    esp_err_t err = 0;
    int64_t timestamp = 0;
    static bool is_setTimeofDay = false;
    
    // 从nvs中读取网络时间
    xSemaphoreTakeRecursive(nvs_MuxSem, portMAX_DELAY);     // 获取锁
    err = tim_read_from_nvs(&my_handle, &timestamp);    
    xSemaphoreGiveRecursive(nvs_MuxSem);                    // 释放锁

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Time not found in NVS. Syncing time from SNTP server.");
        if (fetch_and_store_time_in_nvs(NULL) != ESP_OK) {
            err = ESP_FAIL;
        } else {
            err = ESP_OK;
        }
    } else if (err == ESP_OK) {
        struct timeval get_nvs_time;
        get_nvs_time.tv_sec = timestamp;
        printf("time in nvs:%lld\r\n", timestamp);
        if(is_setTimeofDay == false)
        {
            settimeofday(&get_nvs_time, NULL);
            is_setTimeofDay = true;
        }
    }

    // exit
    if (my_handle != 0) {
        nvs_close(my_handle);
    }
    return err;
}

void mySntp_start(mySntp_done_t f)
{
    if(!get_wifi_status())
        return;

    if(false == mySntp_start_flag)       // 仅当第一次发起sntp同步时创建定时函数和任务
    {
        f_mySntp_done = f;
        ESP_LOGI(TAG, "mySntp start");
        mySntp_start_flag = true;

        nvs_MuxSem = xSemaphoreCreateMutex();       // 创建互斥锁
        printEventGp_handle = xEventGroupCreate();  // 创建打印时间事件标志组

        static uint32_t timer_period = 60000000ull;     // 单位us, 即定时器周期为1分钟
        const esp_timer_create_args_t mySntp_periodic_timer_args = {
                .arg = NULL,
                .callback = access_print_now_time,
                .name = "access_print_time",
        };
        esp_timer_handle_t print_timer_handle;
        esp_timer_create(&mySntp_periodic_timer_args, &print_timer_handle);
        esp_timer_start_periodic(print_timer_handle, timer_period);

        xTaskCreatePinnedToCore(sntp_task, "sntp_task", 4096, NULL, 5, NULL, 1);
    }
}

esp_err_t tim_write_into_nvs(nvs_handle_t* handle, time_t* t)
{
    esp_err_t err;
    //Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, handle);
    if (err != ESP_OK) {
        return err;
    }

    //Write
    err = nvs_set_i64(*handle, "timestamp", *t);
    if (err != ESP_OK) {
        return err;
    }

    // commit
    err = nvs_commit(*handle);
    if (err != ESP_OK) {
        return err;
    }
    nvs_close(*handle);
    return err;
}

esp_err_t tim_read_from_nvs(nvs_handle_t* handle, int64_t* timestamp)
{
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS");
        return err;
    }
    err = nvs_get_i64(*handle, "timestamp", timestamp);
    nvs_close(*handle);
    return err;
}

void copy_timeDate_str(char* tim, char* date)
{
    memcpy(tim, mytime, sizeof(mytime));
    memcpy(date, mydate, sizeof(mydate));
}