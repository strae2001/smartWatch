#include "incl/weather.h"
#include "string.h"

static daily_weather_t w_data[3] = {0};
static char first_parse_done = 0;

esp_err_t parse_weather_data(char* buffer, uint8_t days)
{
    for (uint8_t i = 0; i < days; i++)
        memset(&w_data[i], 0, sizeof(daily_weather_t));

    //1. 将JSON字符串转换成JSON结构体
    cJSON* jsonStruct = cJSON_Parse((const char*)buffer);
    //2. 解析JSON各个字段
    cJSON* resArr = cJSON_GetObjectItem(jsonStruct, "results");
    cJSON* Obj_resArr = cJSON_GetArrayItem(resArr, 0);
    
    // 位置信息
    cJSON* local_obj = cJSON_GetObjectItem(Obj_resArr, "location");
    cJSON* city_obj = cJSON_GetObjectItem(local_obj, "name");

    // 天气信息
    cJSON* dailyArr;
    cJSON* obj_dailyArr;
    cJSON* date_obj; cJSON* codeDay_obj; cJSON* high_obj; cJSON* low_obj;
    dailyArr = cJSON_GetObjectItem(Obj_resArr, "daily");
    for (uint8_t i = 0; i < days; i++)
    {
        obj_dailyArr = cJSON_GetArrayItem(dailyArr, i);
        date_obj = cJSON_GetObjectItem(obj_dailyArr, "date");
        codeDay_obj = cJSON_GetObjectItem(obj_dailyArr, "code_day");
        high_obj = cJSON_GetObjectItem(obj_dailyArr, "high");
        low_obj = cJSON_GetObjectItem(obj_dailyArr, "low");
        strcpy(w_data[i].city, city_obj->valuestring);
        strcpy(w_data[i].date, date_obj->valuestring);
        strcpy(w_data[i].code_day, codeDay_obj->valuestring);
        strcpy(w_data[i].high, high_obj->valuestring);
        strcpy(w_data[i].low, low_obj->valuestring);
        printf("%s %s %s temperature:%s~%s\r\n", w_data[i].city, w_data[i].date, w_data[i].code_day, w_data[i].low, w_data[i].high);
    }

    //3. 清除JSON结构体
    cJSON_Delete(jsonStruct);
    
    if(0 == first_parse_done)
        first_parse_done = 1;
    else
        first_parse_done = -1;
    return ESP_OK;
}

void copy_weatherData(daily_weather_t* wDate)
{
    memcpy(wDate, w_data, sizeof(w_data));
}

char check_first_parse(void)
{
    return first_parse_done;
}