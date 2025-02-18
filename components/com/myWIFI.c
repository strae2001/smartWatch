#include "incl/myWIFI.h"
#include "esp_smartconfig.h"
#include <string.h>

static const char* TAG = "myWIFI";

// 事件标志组句柄
EventGroupHandle_t s_wifi_event_group;
static const int CONNECTED_BIT = BIT0;      // 连接成功事件位
static const int ESPTOUCH_DONE_BIT = BIT1;  // smartconfig完成事件位

// 用一个标志来表示是否处于smartconfig中
static bool s_is_inSmartconfig = false;
// 配网成功标志位
static bool s_isOK_smartconfig = false;
// wifi连接标志位
static uint8_t wifi_connect_flag = 0;    

static uint8_t myWifi_ssid[32] = {0};

// 事件处理回调
void event_handler_cb(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:     //WIFI以STA模式启动后触发此事件
            esp_wifi_connect();
            break;
        
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "conneted success!");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            // 清除连接标志位
            wifi_connect_flag = 0;
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            break;

        default:
            break;
        }
    }
    if(event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "WIFI GOT IP");
            // 获取到IP，置位连接事件标志位
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            wifi_connect_flag = 1;
            break;
        
        default:
            break;
        }
    }
    if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");   // smartconfig 扫描完成
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");  // smartconfig 找到对应的通道
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        // smartconfig 获取到SSID和密码
        ESP_LOGI(TAG, "Got SSID and password");
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        memcpy(myWifi_ssid, evt->ssid, sizeof(evt->ssid));    // 备份ssid

        // 从event_data中提取SSID和密码
        wifi_config_t wifi_cfg;
        bzero(&wifi_cfg, sizeof(wifi_config_t));
        memcpy(wifi_cfg.sta.ssid, evt->ssid, sizeof(wifi_cfg.sta.ssid));
        memcpy(wifi_cfg.sta.password, evt->password, sizeof(wifi_cfg.sta.password));
        wifi_cfg.sta.bssid_set = evt->bssid_set;
        if (wifi_cfg.sta.bssid_set == true) {
            memcpy(wifi_cfg.sta.bssid, evt->bssid, sizeof(wifi_cfg.sta.bssid));
        }
        //重新连接WIFI
        ESP_ERROR_CHECK( esp_wifi_disconnect());
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg) );
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        /* smartconfig 已发送回应 */
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void wifi_sta_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());  // nvs初始化(wifi数据存储在nvs分区中)
    ESP_ERROR_CHECK(esp_netif_init());  // 初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());   //创建一个默认系统事件调度循环
    esp_netif_create_default_wifi_sta();    //使用默认配置创建STA对象

    // 初始化wifi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册WIFI事件、IP事件、SmartConfig事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_cb, NULL));
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler_cb, NULL);

    // wifi配置
    wifi_config_t wifi_cfg = 
    {
        .sta = 
        {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,      // 加密方式
            .pmf_cfg = 
            {
                .capable = true,        // 启用保护管理帧
                .required = false       // 禁止仅与保护管理帧设备通信
            }
        }
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
}

void smartconfig_task(void * param)
{
    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);  //设定SmartConfig版本
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    s_isOK_smartconfig = false;
    esp_smartconfig_start(&cfg);    //启动SmartConfig
    
    ESP_LOGI(TAG, "smartconfig  has started!");

    EventBits_t evflag;
    while (1)
    {
        // 等待wifi连接完成事件、smartConfig配网完成通知事件
        evflag = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, pdTRUE, pdTRUE, pdMS_TO_TICKS(30000));
        if(evflag & ESPTOUCH_DONE_BIT && evflag & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Connected to ap");
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();       // 停止smartconfig配网
            s_is_inSmartconfig = false;
            s_isOK_smartconfig = true;
            vTaskDelete(NULL);            // 退出配网任务
        }
        else
        {
            ESP_LOGE(TAG, "smartconfig failed! please retry!");
            esp_smartconfig_stop();
            s_is_inSmartconfig = false;
            s_isOK_smartconfig = false;
            vTaskDelete(NULL);
        }
    }
}

void smartConfig_start(void)
{
    if(false == s_is_inSmartconfig)
    {
        // 创建自定义wifi事件
        s_wifi_event_group = xEventGroupCreate();
        esp_wifi_start();       // 启动wifi
        s_is_inSmartconfig = true;
        // 创建配网任务
        xTaskCreatePinnedToCore(smartconfig_task, "smartconfig_task", 4096, NULL, 5, NULL, 1);
    }
}

void copy_wifi_ssid(uint8_t* dest)
{
    memcpy(dest, myWifi_ssid, sizeof(myWifi_ssid));
}

uint8_t get_wifi_status(void)
{
    return wifi_connect_flag;
}

bool is_inSmartConfig(void)
{
    return s_is_inSmartconfig;
}

bool isOK_SmartConfig(void)
{
    return s_isOK_smartconfig;
}

void isOK_SmartConfig_Set_0(void)
{
    s_isOK_smartconfig = false;
}