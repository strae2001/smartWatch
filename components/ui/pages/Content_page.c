#include "../ui.h"
#include "esp_log.h"

static const char* TAG = "ContentPage";

// 每页的字符数量 
#define PAGE_CHAR_COUNT 600

// 当前显示页
static int current_page = 0; 

// 翻页显示
void page_turning(void)
{
    printf("page turn...\r\n");
    static char page_text[PAGE_CHAR_COUNT + 1];
    int start_index = current_page * PAGE_CHAR_COUNT;      // 要显示数据的起始索引
    if (start_index >= file_data.len)
        return;
    int end_index = start_index + PAGE_CHAR_COUNT;         // 要显示数据的结束索引
    if (end_index > file_data.len)
        end_index = file_data.len;
    memcpy(page_text, file_data.text_block + start_index, end_index - start_index);
    page_text[end_index - start_index] = '\0';
    lv_label_set_text(content, page_text);
    memset(page_text, 0, sizeof(page_text));
}

static void scroll_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);         // 获取触发事件的对象
    
    // 最大滚动长度 = 文本框label的高度 - 滚动容器的高度
    int scroll_y_max = lv_obj_get_height(lv_obj_get_child(obj, 0)) - lv_obj_get_height(obj);
    switch (code)
    {
    case LV_EVENT_SCROLL_END:
    {
        int y = lv_obj_get_scroll_y(obj);       // 获取当前滚轮y坐标
        // 根据滚动位置判断是否需要切换页
        if(y < 0)   
        {                   // 滚轮至顶上一页：当y为负数即向上滚动至顶时, 切换上一页
            if (current_page > 0) { 
                current_page--; 
                page_turning(); 
                lv_obj_scroll_to_y(obj, lv_obj_get_height(obj), LV_ANIM_OFF); 
            }
        }
        else if(scroll_y_max - y < 10)     // 滚轮至底下一页：最大滚动长度 - 当前滚轮y坐标 < 10 时表示滚轮已划动至页面底部
        {   
            // 计算总页数, 向上取整; 例: 200字不足一页, 仍使用一页显示
            int total_pages = (file_data.len + PAGE_CHAR_COUNT - 1) / PAGE_CHAR_COUNT;
            
            // 最后一页索引为total_pages -1
            if(current_page < total_pages -1)    
            {                   // 当未至最后一页时可翻页
                current_page++;
                page_turning();
                lv_obj_scroll_to_y(obj, 0, LV_ANIM_OFF); 
            }
        }
        break;
    }
    
    default:
        break;
    }
}

static void lv_ContentBackBtn_handle(lv_event_t* e)
{
    lv_event_code_t btn_ev = lv_event_get_code(e);         // 获取事件代码

    switch (btn_ev)
    {
    case LV_EVENT_CLICKED:
        // 释放当前解析文件所读取的所有内容
        if(file_data.text_block)
        {
            free(file_data.text_block);
            file_data.text_block = NULL;
        }
        file_data.len = 0;
        current_page = 0;      // 重置页码
        lv_scr_load_anim(filelist_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        break;
      
    default:
        break;
    }
    
}

void Content_page_init(void) 
{
    fileContent_scr = lv_obj_create(NULL);

    // 创建可滚动的容器
    scroll_container = lv_obj_create(fileContent_scr);
    lv_obj_set_size(scroll_container, LV_HOR_RES, LV_VER_RES - 45);  // 滚动容器大小为屏幕分辨率减nav高度
    lv_obj_align(scroll_container, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_pad_all(scroll_container, 0, 0);
    lv_obj_set_scrollbar_mode(scroll_container, LV_SCROLLBAR_MODE_AUTO);  // 启用滚动条
    lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER);        // 滚动方向为垂直滚动
    lv_obj_set_style_radius(scroll_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(scroll_container, scroll_event_cb, LV_EVENT_SCROLL_END, NULL);

    // 创建文本内容标签
    content = lv_label_create(scroll_container);
    lv_label_set_long_mode(content, LV_LABEL_LONG_WRAP);  // 设置长文本模式为自动换行
    lv_obj_set_width(content, LV_HOR_RES-10);  // 设置 label 宽度为屏幕宽度
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 5, 2);
    lv_obj_set_style_text_font(content, &lv_font_montserrat_16, 0);  

    // 创建nav顶部导航栏
    ui_nav = lv_obj_create(fileContent_scr);
    lv_obj_remove_style_all(ui_nav);
    lv_obj_set_width(ui_nav, 280);
    lv_obj_set_height(ui_nav, 45);
    lv_obj_set_align(ui_nav, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(ui_nav, lv_color_hex(0xE0DFDF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_nav, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_nav, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_nav, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(ui_nav, LV_OBJ_FLAG_FLOATING);      // nav悬浮

    // 顶部文件名
    file_name = lv_label_create(ui_nav);
    lv_obj_align(file_name, LV_ALIGN_CENTER, 0, 2);
    lv_obj_set_style_text_font(file_name, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 顶部回退键
    lv_obj_t* return_btn = lv_btn_create(ui_nav);
    lv_obj_set_style_shadow_opa(return_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(return_btn, 40, 30);
    lv_obj_set_style_bg_color(return_btn, lv_color_hex(0xE0DFDF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(return_btn, LV_ALIGN_LEFT_MID, 15, 2);
    lv_obj_t* return_symbol = lv_label_create(return_btn);
    lv_obj_align(return_symbol, LV_ALIGN_CENTER, -5, 0);
    lv_obj_set_style_text_color(return_symbol, lv_color_hex(0x434343), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(return_symbol, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(return_btn, lv_ContentBackBtn_handle, LV_EVENT_CLICKED, NULL);
}




