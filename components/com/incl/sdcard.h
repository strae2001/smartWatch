#ifndef __SDCARD_FILE_H
#define __SDCARD_FILE_H

#include "esp_err.h"

/** 初始化SD卡
 * @param 无
 * @return 成功或失败，只有成功初始化SD卡，才可以获取文件列表
*/
esp_err_t sdcard_init(void);

/** 获取SD卡的文件列表
 * @param file 返回的文件列表
 * @return 文件列表长度
*/
int get_sdcard_filelist(const char (**file)[256]);

#endif