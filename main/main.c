#include <stdio.h>
#include <time.h>
#include "incl/myWIFI.h"
#include "incl/Ntp_time.h"
#include "incl/https_reqst.h"
#include "incl/lv_port.h"
#include "lvgl/lvgl.h"
#include "lv_demos.h"
#include "incl/sdcard.h"
#include "incl/screen_ledc.h"
#include "ui.h"

// #include "driver/gpio.h"
// #include "esp_timer.h"
// #include "esp_sleep.h"
// #include "esp_wake_stub.h"
// #include "driver/rtc_io.h"

static const char* TAG = "main";

// sntp时间获取完成回调
void mySntp_done_cb(void)
{
    ESP_LOGI(TAG, "mySntp_done, Beijing time was successfully obtained");
}

void lvgl_task_run(void * param)
{
    ui_init();
    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    wifi_sta_init();    // wifi设置为sta模式
    sdcard_init();      // sd卡初始化
    lv_port_init();     // 初始化lvgl
    scr_backlight_control_init();       // 初始化屏幕背光控制
    xTaskCreatePinnedToCore(lvgl_task_run, "lvgl_task", 8192, NULL, 3, NULL, 1);

    while (1)
    {
        mySntp_start(mySntp_done_cb);
        https_request_start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
