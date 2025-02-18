#include <stdio.h>
#include "string.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "incl/text_parse.h"

static const char* TAG = "text_parse";

long textfile_read_All(const char* fname, text_block_data_t* text_data)
{
    char textfile_name[256] = {0};
    snprintf(textfile_name, sizeof(textfile_name), "/sdcard/%s", fname);
    FILE* fp = fopen(textfile_name, "r");
    if(!fp)
    {
        ESP_LOGE(TAG,"fopen failed!");
        return -1;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    // 文本大小限制
    if(fsize > 100*1024)
    {
        ESP_LOGE(TAG,"file is too big to read!");
        fclose(fp);
        return fsize;
    }
    fseek(fp, 0, SEEK_SET);     // 复位文件指针

    // 文件内容读取缓冲区
    uint8_t* read_buffer = (uint8_t*)malloc(fsize + 100);
    if(!read_buffer)
    {
        fclose(fp);
        return -1;
    }
        
    size_t read_bytes = 0;
    uint8_t* text_block_buffer = NULL;   // 读取到的数据指针
    read_bytes = fread(read_buffer, 1, fsize + 100, fp);
    if(read_bytes > 0)
    {
        text_block_buffer = (uint8_t*)malloc(read_bytes + 1);
        memcpy(text_block_buffer, read_buffer, read_bytes);
        text_block_buffer[read_bytes] = '\0';
        // 传递数据
        text_data->text_block = text_block_buffer;
        text_data->len = read_bytes;
        text_block_buffer = NULL;
    }
    else 
    {
        ESP_LOGE(TAG,"read_bytes is 0!");
        text_data->len = 0;
    }

    if(read_buffer)
    {
        free(read_buffer);
        read_buffer = NULL;
    }

    if(fp) fclose(fp);
    return fsize;
}