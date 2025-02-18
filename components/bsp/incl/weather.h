#ifndef __WEATHER_H
#define __WEATHER_H

#include "esp_err.h"
#include "cJSON.h"

typedef struct 
{
    char city[15];
    char date[20];
    char code_day[10];       // 天气代码
    char high[5];
    char low[5];
}daily_weather_t;

/**
 * @description: 解析json天气数据包
 * @param {char*} buffer    raw data
 * @param {uint8_t} days    解析几天的数据
 * @return {*}  成功：ESP_OK
 */
esp_err_t parse_weather_data(char* buffer, uint8_t days);

// 向外提供获取天气数据的接口
void copy_weatherData(daily_weather_t* wDate);

// 向外提供检查首次解析天气数据是否成功的接口
char check_first_parse(void);

#endif