#include "../ui.h"
#include "incl/sdcard.h"

static const char* TAG = "filelistPage";

// 当前解析文件的总大小
static long fsize = 0;

// 由于设备资源紧张, 限制打开超大文件, 最大支持解析100KB文件
#define FILE_SIZE_MAX 100*1024

// 文件列表项事件回调
void lv_list_btn_event_cb(lv_event_t * e)
{   
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);         // 获取触发事件的对象
    switch (code)
    {
    case LV_EVENT_CLICKED:
    {
        // 获取点击的列表项的文件名
        const char* fileName = lv_list_get_btn_text(s_lv_file_list, obj);
        if(fileName)
        {
            if(strstr(fileName, ".txt") || strstr(fileName, ".TXT"))
            {
                fsize = textfile_read_All(fileName, &file_data);
                if(fsize >= 0 && fsize <= FILE_SIZE_MAX)
                {
                    if(file_data.len)
                    {
                        lv_label_set_text(file_name, fileName);     // 显示文件名
                        page_turning();     // 打开文件后显示第一页内容
                    }
                    // 刷新屏幕
                    lv_obj_scroll_to(scroll_container, 0, 0, LV_ANIM_OFF);   // 重置滚动条
                    lv_scr_load_anim(fileContent_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
                }
            }
        }
        break;
    }
       
    default:
        break;
    }
}

static void lv_filelistBackBtn_handle(lv_event_t* e)
{
    lv_event_code_t btn_ev = lv_event_get_code(e);         // 获取事件代码

    switch (btn_ev)
    {
    case LV_EVENT_CLICKED:      // 点击事件:
        lv_scr_load_anim(home_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        break;
      
    default:
        break;
    }
    
}

void filelist_page_init(void)
{
    // 1.创建文件列表页面, 页面bg设置为白色
    filelist_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(filelist_scr, lv_color_white(), 0);

    // 获取页面长宽
    lv_coord_t  hor_res = lv_disp_get_hor_res(NULL);
    lv_coord_t  ver_res = lv_disp_get_ver_res(NULL);

    // 返回按键
    lv_obj_t* return_btn = lv_btn_create(filelist_scr);
    lv_obj_set_style_shadow_opa(return_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(return_btn, 40, 30);
    lv_obj_set_style_bg_color(return_btn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 15, 10);

    lv_obj_t* return_symbol = lv_label_create(return_btn);
    lv_obj_align(return_symbol, LV_ALIGN_CENTER, -5, 0);
    lv_obj_set_style_text_color(return_symbol, lv_color_hex(0x434343), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(return_symbol, LV_SYMBOL_LEFT);
    lv_obj_add_event_cb(return_btn, lv_filelistBackBtn_handle, LV_EVENT_CLICKED, NULL);

    // 创建文件页面标题标签
    s_lv_file_title = lv_label_create(filelist_scr);
    lv_label_set_text(s_lv_file_title,"SDCard File");   // 设置标签文字
    lv_obj_align(s_lv_file_title,LV_ALIGN_TOP_MID,0,20);    // 设置标签位置: 顶端居中
    lv_obj_set_style_bg_color(s_lv_file_title, lv_color_white(), 0);        // 标签颜色为白色
    lv_obj_set_style_text_font(s_lv_file_title,&lv_font_montserrat_24,0);   // 设置标签文字大小
    lv_obj_set_style_text_color(s_lv_file_title,lv_color_black(),0);        // 设置标签文字颜色为黑色

    // 创建文件列表
    s_lv_file_list = lv_list_create(filelist_scr);
    lv_obj_set_size(s_lv_file_list, hor_res-60, ver_res - 100);
    lv_obj_align(s_lv_file_list, LV_ALIGN_BOTTOM_MID, 0, -40);          // 底部居中，向上偏移40
    lv_obj_set_style_text_color(s_lv_file_list,lv_color_black(),0);     // 列表文字黑色
    lv_obj_set_style_bg_color(s_lv_file_list,lv_color_white(),0);       // 列表为白色
    lv_obj_set_style_text_font(s_lv_file_list, &lv_font_montserrat_20, 0);   // 设置列表文字大小

    // 获取SD卡文件列表
    const char (*filelist)[256] = NULL;
    int filename_cnt = get_sdcard_filelist(&filelist);
    // 遍历文件列表, 使文件项可点击
    for (int i = 0; i < filename_cnt; i++)
    {
        lv_obj_t* btn = lv_list_add_btn(s_lv_file_list, NULL, *filelist);       // 作为按钮添加至文件列表中
        // 为文件列表项添加点击事件
        lv_obj_add_event_cb(btn, lv_list_btn_event_cb, LV_EVENT_CLICKED, NULL);
        filelist++;
    }
}