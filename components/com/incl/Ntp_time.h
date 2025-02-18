#ifndef __NTP_TIME_H
#define __NTP_TIME_H

#include "esp_err.h"
#include "nvs_flash.h"
#include <time.h>

// sntp时间获取完成回调定义
typedef void (*mySntp_done_t)(void);

/**
 * @description: 开始启动sntp获取网络时间
 * @param {mySntp_done_t} f   回调函数
 * @return {*}
 */
void mySntp_start(mySntp_done_t f);

/**
 * @description: 将时间戳备份至nvs
 * @param {nvs_handle_t*} handle    nvs操作句柄
 * @param {time_t*} t               时间戳指针
 * @return {*}  成功: ESP_OK;   失败: others
 */
esp_err_t tim_write_into_nvs(nvs_handle_t* handle, time_t* t);
esp_err_t tim_read_from_nvs(nvs_handle_t* handle, int64_t* timestamp);

/**
 * @description: Fetch the current time from SNTP and stores it in NVS.
 * @param {void*}
 * @return {*}  Successful: ESP_OK;    Failed: other flag
 */
esp_err_t fetch_and_store_time_in_nvs(void*);

/**
 * @description: Update the system time from time stored in NVS.
 * @return {*}  Successful: ESP_OK;    Failed: other flag
 */
esp_err_t update_time_from_nvs(void);

// 对外提供获取时间日期的接口
void copy_timeDate_str(char* tim, char* date);

#endif