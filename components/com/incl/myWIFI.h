#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"

/**
 * @description: 初始化wifi为sta模式
 * @return {*}
 */
void wifi_sta_init(void);

/**
 * @description: 启动smartConfig配网
 * @return {*}
 */
void smartConfig_start(void);

// 获取wifi连接状态
uint8_t get_wifi_status(void);

// 判断是否处于配网中
bool is_inSmartConfig(void);

// 判断是否配网成功
bool isOK_SmartConfig(void);

// 将配网成功标志设为0, 即置为配网不成功
void isOK_SmartConfig_Set_0(void);

// 向外提供ssid获取接口
void copy_wifi_ssid(uint8_t* dest);

#endif