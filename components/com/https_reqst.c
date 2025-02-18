#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "sdkconfig.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && CONFIG_EXAMPLE_USING_ESP_TLS_MBEDTLS
#include "esp_crt_bundle.h"
#endif
#include "incl/Ntp_time.h"
#include "incl/https_reqst.h"
#include "incl/weather.h"
#include "incl/myWIFI.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT "443"
#define WEB_URL "https://api.seniverse.com/v3/weather/daily.json?key=SW0TYis5C1ku_SYTO&location=ip&language=zh-Hans&unit=c&start=0&days=3"
                 
#define SERVER_URL_MAX_SZ 256

static EventGroupHandle_t reqEventGp_handle;
static uint8_t START_REQ_EVENTBIT = BIT0;

static const char *TAG = "https_req";
static bool https_request_start_flag = false;        // 启动https请求标志位

static const char HOWSMYSSL_REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
                             "Host: "WEB_SERVER"\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";

static void https_get_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST)
{
    char buf[1024] = {0};
    int ret, len;

    esp_tls_t *tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
        goto exit;
    }
    
    if (esp_tls_conn_http_new_sync(WEB_SERVER_URL, &cfg, tls) == 1) {
        ESP_LOGI(TAG, "Connection established...");
    } else {
        ESP_LOGE(TAG, "Connection failed...");
        int esp_tls_code = 0, esp_tls_flags = 0;
        esp_tls_error_handle_t tls_e = NULL;
        esp_tls_get_error_handle(tls, &tls_e);
        /* Try to get TLS stack level error and certificate failure flags, if any */
        ret = esp_tls_get_and_clear_last_error(tls_e, &esp_tls_code, &esp_tls_flags);
        if (ret == ESP_OK) {
            ESP_LOGE(TAG, "TLS error = -0x%x, TLS flags = -0x%x", esp_tls_code, esp_tls_flags);
        }
        goto cleanup;
    }

    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 strlen(REQUEST) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto cleanup;
        }
    } while (written_bytes < strlen(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");
    uint8_t read_cnt = 0;
    do {
        len = sizeof(buf) - 1;
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        } else if (ret < 0) {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        } else if (ret == 0) {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        
        /* Print response directly to stdout as it is read */
        if(++read_cnt == 2)
        {  
            // printf("%s\r\n", buf);
            parse_weather_data(buf, 3);
        }
        else printf("%s", buf);
    } while (1);

cleanup:
    esp_tls_conn_destroy(tls);
exit:
    for (int countdown = 10; countdown >= 0; countdown--) {
        ESP_LOGI(TAG, "%d...", countdown);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && CONFIG_EXAMPLE_USING_ESP_TLS_MBEDTLS
static void https_get_request_using_crt_bundle(void)
{
    ESP_LOGI(TAG, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    https_get_request(cfg, WEB_URL, HOWSMYSSL_REQUEST);
}
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && CONFIG_EXAMPLE_USING_ESP_TLS_MBEDTLS

static void send_https_request_task(void* params)
{
    static uint16_t req_cnt = 0;    // 请求次数
    static EventBits_t wait_bit;

    while(1)
    {
        wait_bit = xEventGroupWaitBits(reqEventGp_handle, START_REQ_EVENTBIT, pdTRUE, pdTRUE, portMAX_DELAY);
        if(wait_bit & START_REQ_EVENTBIT)
        {
            ESP_LOGI(TAG, "Start %dth https_request", ++req_cnt);
            if (esp_reset_reason() == ESP_RST_POWERON) {     // 从nvs中更新系统时间, 用于https验证
                ESP_LOGI(TAG, "Updating time from NVS");
                ESP_ERROR_CHECK(update_time_from_nvs());
            }
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE && CONFIG_EXAMPLE_USING_ESP_TLS_MBEDTLS
            https_get_request_using_crt_bundle();
#endif
            ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
            ESP_LOGI(TAG, "Finish https_request");
        }
    }
}

static void access_https_request(void* args)
{
    xEventGroupSetBits(reqEventGp_handle, START_REQ_EVENTBIT);
}

static void https_request_task(void *pvparameters)
{
    static uint64_t timer_period = 180000000ull;        // 3分钟发送一次https请求

    reqEventGp_handle = xEventGroupCreate();
    xEventGroupSetBits(reqEventGp_handle, START_REQ_EVENTBIT);     // 默认启动后发送一次https请求

    const esp_timer_create_args_t https_request_timer_args = {
            .callback = access_https_request,
    };
    esp_timer_handle_t https_request_timer;
    ESP_ERROR_CHECK(esp_timer_create(&https_request_timer_args, &https_request_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(https_request_timer, timer_period));

    xTaskCreatePinnedToCore(send_https_request_task, "send_https_req_task", 8192, NULL, 5, NULL, 1);
    vTaskDelete(NULL);
}

void https_request_start(void)
{
    if(!get_wifi_status())
        return;

    if(false == https_request_start_flag)
    {
        https_request_start_flag = true;
        static uint64_t timer_period = 180000000ull;      /* Timer interval once every day (3mins) */
        const esp_timer_create_args_t nvs_update_timer_args = {
            .callback = (void*)fetch_and_store_time_in_nvs,
        };
        esp_timer_handle_t nvs_update_timer;
        ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, timer_period));

        xTaskCreatePinnedToCore(&https_request_task, "https_get_task", 8192, NULL, 3, NULL, 1);
    }
}
