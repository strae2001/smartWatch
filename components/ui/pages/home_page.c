#include "../ui.h"
#include "incl/Ntp_time.h"
#include "incl/weather.h"
#include "incl/myWIFI.h"
#include "esp_log.h"

LV_IMG_DECLARE(Sunny)
LV_IMG_DECLARE(rainy)
LV_IMG_DECLARE(cloudy)
LV_IMG_DECLARE(dis_wifi_32)

static const char* TAG = "home_page";

static lv_timer_t* wifi_timer_handle = NULL;
static lv_timer_t* get_timer_handle = NULL;         // 周期地获取时间定时器句柄

static daily_weather_t weather_data[3] = {0};

static void show_weather(void)
{
    copy_weatherData(weather_data);
    if(weather_data[0].city[0] != '\0')
    {
        uint8_t weather_code = atoi(weather_data[0].code_day);
        switch (weather_code)
        {
        case 0:case 1:case 2:case 3:        // 天气代码0~3均为晴
            lv_label_set_text(weather_label, "Sunny");
            lv_img_set_src(weather_img, &Sunny);
            break;
        
        case 4:case 5:case 6:case 7:case 8:case 9:
            lv_label_set_text(weather_label, "Cloudy");
            lv_img_set_src(weather_img, &cloudy);
            break;
        
        case 10:case 11:case 12:case 13:case 14:
        case 15:case 16:case 17:case 18:
            lv_label_set_text(weather_label, "Rainy");
            lv_img_set_src(weather_img, &rainy);
            break;

        default:
            break;
        }
        char temp_data[20] = {0};
        snprintf(temp_data, sizeof(temp_data), "%s~%s°C", 
            weather_data[0].low, weather_data[0].high);
        lv_label_set_text(temp_label, temp_data);
    }
}

static void get_nowtim_weather_cb(struct _lv_timer_t * t)
{
    char tim[12] = {0};
    char date[24] = {0};
    static uint32_t update_weather_tim = 0;
    static uint8_t first_show_done = 0;
    copy_timeDate_str(tim, date);

    if(tim[0] != '\0' && date[0] != '\0')
    {
        lv_label_set_text(tim_label, tim);
        lv_label_set_text(date_label, date);
    }

    if(0 == update_weather_tim || t->last_run < update_weather_tim)
        update_weather_tim = t->last_run;
    if(t->last_run - update_weather_tim >= 500*2*60*2)      // 2分钟获取一次weather数据
    {
        update_weather_tim = t->last_run;
        show_weather();
        // printf("update weather tim: %ld\r\n", update_weather_tim);
    }

    if(1 == check_first_parse() && 0 == first_show_done)
    {
        first_show_done = 1;
        show_weather();
    }
}

static void check_wifi_timer_cb(struct _lv_timer_t * t)
{
    static lv_obj_t* home_wifi_img;
    if(get_wifi_status())
    {
        if (home_wifi_img != NULL)
            lv_obj_del(home_wifi_img);
        home_wifi_img = lv_label_create(home_scr);
        lv_label_set_text(home_wifi_img, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_font(home_wifi_img, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(home_wifi_img, lv_color_hex(0x71C438), 0);
        lv_obj_align(home_wifi_img, LV_ALIGN_CENTER, 0, 8);

        if(get_timer_handle == NULL)
            get_timer_handle = lv_timer_create(get_nowtim_weather_cb, 500, NULL);
    }
    else
    {
        if (home_wifi_img != NULL)
            lv_obj_del(home_wifi_img);
        home_wifi_img = lv_img_create(home_scr);
        lv_img_set_src(home_wifi_img, &dis_wifi_32);
        lv_obj_align(home_wifi_img, LV_ALIGN_CENTER, 0, 8);
    }
}

static void lv_appBtn_handle(lv_event_t* e)
{
    lv_event_code_t btn_ev = lv_event_get_code(e);         // 获取事件代码
    lv_obj_t* obj = lv_event_get_target(e);
    switch (btn_ev)
    {
    case LV_EVENT_CLICKED:      // 点击事件:
        if(obj == ui_setting_app)
            lv_scr_load_anim(setting_page_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        if(obj == ui_document_app)
            lv_scr_load_anim(filelist_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        break;
        
    default:
        break;
    }
}

static void create_weather_module(void)
{
    ui_weather_box = lv_obj_create(home_scr);
    lv_obj_remove_style_all(ui_weather_box);
    lv_obj_set_width(ui_weather_box, 100);
    lv_obj_set_height(ui_weather_box, 60);
    lv_obj_set_x(ui_weather_box, 25);
    lv_obj_set_y(ui_weather_box, 26);
    lv_obj_clear_flag(ui_weather_box, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_weather_box, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_weather_box, lv_color_hex(0xBCBCBC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_weather_box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    // 天气
    weather_label = lv_label_create(ui_weather_box);
    lv_obj_set_width(weather_label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(weather_label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(weather_label, -10);
    lv_obj_set_y(weather_label, 12);
    lv_obj_set_align(weather_label, LV_ALIGN_TOP_RIGHT);
    lv_label_set_text(weather_label, "Sunny");
    lv_obj_set_style_text_font(weather_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    // 天气图片
    weather_img = lv_img_create(ui_weather_box);
    lv_img_set_src(weather_img, &Sunny);
    lv_obj_align(weather_img, LV_ALIGN_TOP_LEFT, 6, 15);

    // 温度
    temp_label = lv_label_create(ui_weather_box);
    lv_obj_set_width(temp_label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(temp_label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(temp_label, -8);
    lv_obj_set_y(temp_label, -8);
    lv_obj_set_align(temp_label, LV_ALIGN_BOTTOM_RIGHT);
    lv_label_set_text(temp_label, "10~15°C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void create_timDate_module(void)
{
    // tim
    tim_label = lv_label_create(home_scr);
    lv_obj_set_width(tim_label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(tim_label, LV_SIZE_CONTENT);    /// 1
    lv_obj_align_to(tim_label, ui_weather_box, LV_ALIGN_OUT_RIGHT_MID, 8, -18);
    lv_label_set_text(tim_label, "00:00");
    lv_obj_set_style_text_font(tim_label, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    // date
    date_label = lv_label_create(home_scr);
    lv_obj_set_width(date_label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(date_label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(date_label, 0);
    lv_obj_set_y(date_label, -17);
    lv_obj_set_align(date_label, LV_ALIGN_CENTER);
    lv_label_set_text(date_label, "1970-01-01  Mon.");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void create_app_module(void)
{
    // Settings APP
    ui_setting_app = lv_btn_create(home_scr);
    lv_obj_set_style_shadow_opa(ui_setting_app, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(ui_setting_app, 40, 40);
    lv_obj_set_style_bg_color(ui_setting_app, lv_color_hex(0xBCBCBC), LV_STATE_DEFAULT);
    lv_obj_align_to(ui_setting_app, ui_weather_box, LV_ALIGN_BOTTOM_MID, 0, 130);
    lv_obj_add_event_cb(ui_setting_app, lv_appBtn_handle, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(ui_setting_app);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x3f3f3f), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // Document APP
    ui_document_app = lv_btn_create(home_scr);
    lv_obj_set_style_shadow_opa(ui_document_app, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(ui_document_app, 40, 40);
    lv_obj_set_style_bg_color(ui_document_app, lv_color_hex(0xBCBCBC), LV_STATE_DEFAULT);
    lv_obj_align_to(ui_document_app, ui_setting_app, LV_ALIGN_RIGHT_MID, 70, 0);
    lv_obj_add_event_cb(ui_document_app, lv_appBtn_handle, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(ui_document_app);
    lv_label_set_text(label, LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x3f3f3f), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void home_page_init(void)
{
    home_scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_img_recolor(home_scr, lv_color_hex(0x464646), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_recolor_opa(home_scr, 255, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    create_weather_module();
    create_timDate_module();
    create_app_module();

     if(wifi_timer_handle == NULL)
        wifi_timer_handle = lv_timer_create(check_wifi_timer_cb, 500, NULL);
}
