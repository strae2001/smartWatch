#ifndef _TEXT_PARSE_H_
#define _TEXT_PARSE_H_

#include "stdio.h"
#include <stdint.h>

// 读取的文本数据单元
typedef struct 
{
    uint8_t* text_block;         // 文本块
    size_t len;                  // 块大小
}text_block_data_t;

/// @brief 读取文本文件所有内容
/// @param fname 
/// @param text_data 
/// @return    返回文件大小
long textfile_read_All(const char* fname, text_block_data_t* text_data);

#endif