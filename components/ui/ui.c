#include "ui.h"

// SCREEN: home_page
lv_obj_t* home_scr;
lv_obj_t* ui_weather_box;
lv_obj_t* ui_setting_app;
lv_obj_t* ui_document_app;
lv_obj_t* weather_label;
lv_obj_t* temp_label;
lv_obj_t* tim_label;
lv_obj_t* date_label;
lv_obj_t* weather_img;
void home_page_init(void);
// CUSTOM VARIABLES

// SCREEN: Setting_page
lv_obj_t* setting_page_scr;
lv_obj_t* light_slider;
lv_obj_t* voice_slider;
lv_obj_t* wifi_btn;
lv_obj_t* netConfig;
lv_style_t style_part_main;
lv_style_t style_part_knob;
lv_style_t style_part_indicator;
void setting_page_init(void);
// CUSTOM VARIABLES

// SCREEN: filelist_page
lv_obj_t* filelist_scr = NULL;
lv_obj_t* s_lv_file_title = NULL;
lv_obj_t* s_lv_file_list = NULL;
void filelist_page_init(void);
// CUSTOM VARIABLES

// SCREEN: Content_page
lv_obj_t* fileContent_scr;
lv_obj_t* scroll_container;
lv_obj_t* ui_nav;
lv_obj_t* file_name;
lv_obj_t* content;
void Content_page_init(void);
// CUSTOM VARIABLES

text_block_data_t file_data = {0};    // 解析出来的所有文件数据

void ui_init(void)
{
    home_page_init();
    setting_page_init();
    filelist_page_init();
    Content_page_init();

    lv_disp_load_scr(home_scr);
}