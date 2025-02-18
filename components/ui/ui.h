#ifndef __MY_UI_H
#define __MY_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "incl/text_parse.h"

// SCREEN: home_page
extern lv_obj_t* home_scr;
extern lv_obj_t* ui_weather_box;
extern lv_obj_t* ui_setting_app;
extern lv_obj_t* ui_document_app;
extern lv_obj_t* weather_label;
extern lv_obj_t* temp_label;
extern lv_obj_t* tim_label;
extern lv_obj_t* date_label;
extern lv_obj_t* weather_img;
void home_page_init(void);
// CUSTOM VARIABLES

// SCREEN: Setting_page
extern lv_obj_t* setting_page_scr;
extern lv_obj_t* light_slider;
extern lv_obj_t* voice_slider;
extern lv_obj_t* wifi_btn;
extern lv_obj_t* netConfig;
extern lv_style_t style_part_main;
extern lv_style_t style_part_knob;
extern lv_style_t style_part_indicator;
void setting_page_init(void);
// CUSTOM VARIABLES

// SCREEN: filelist_page
extern lv_obj_t* filelist_scr;
extern lv_obj_t* s_lv_file_title;
extern lv_obj_t* s_lv_file_list;
extern lv_timer_t* s_reader_timer;      // 读取文件内容定时器
void filelist_page_init(void);
// CUSTOM VARIABLES

extern text_block_data_t file_data;    // 解析出来的所有文件数据

// SCREEN: Content_page
extern lv_obj_t* fileContent_scr;
extern lv_obj_t* scroll_container;
extern lv_obj_t* ui_nav;
extern lv_obj_t* file_name;
extern lv_obj_t* content;
void Content_page_init(void);
void page_turning(void);        // 翻页, 更新内容页面显示
// CUSTOM VARIABLES


// UI INIT
void ui_init(void);

#ifdef __cplusplus
}
#endif

#endif