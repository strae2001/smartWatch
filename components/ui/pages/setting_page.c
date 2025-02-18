// #include "lvgl/lvgl.h"
// #include "incl/setting_page.h"
#include "../ui.h"
#include "incl/screen_ledc.h"
#include "incl/myWIFI.h"

static const char* TAG = "settingPage";

LV_IMG_DECLARE(light_img);

static lv_obj_t* return_btn;
static lv_obj_t* wifi_symbol;
static lv_obj_t* wifi_name;
static lv_obj_t* netConfig_symbol;

// 配网成功自动连上wifi, 点亮wifi按键标志
static uint8_t lv_netConfig_OK_lighten_wifiBtn = 0;

// LVGL定时器
static lv_timer_t* netConfig_timer_handle = NULL;       // 创建定时器控件句柄
static lv_timer_t* wifi_timer_handle = NULL;
static bool s_netConfig_flag = false;       // 配网状态扫描定时器启动标志位
static bool s_wifi_flag = false;            // wifi状态扫描定时器启动标志位

// 按键状态
static uint8_t wifi_btn_state = 0;          // 默认未激活

// 要在屏幕中显示的ssid
static uint8_t wifi_ssid_text[32] = {0};

static void wifi_timer_cb(struct _lv_timer_t * t)
{
    
    if(get_wifi_status())
        lv_label_set_text(wifi_name, (char*)wifi_ssid_text);
    else
        lv_label_set_text(wifi_name, "");   
}

static void netConfig_timer_cb(struct _lv_timer_t * t)
{
    if(is_inSmartConfig() == false && isOK_SmartConfig() == true)
    {
        lv_obj_set_style_bg_color(netConfig, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(netConfig_symbol, lv_color_hex(0x707070), 0);
        // 配网成功后, 会自动连上wifi
        if(lv_netConfig_OK_lighten_wifiBtn)
        {
            lv_netConfig_OK_lighten_wifiBtn = 0;
            wifi_btn_state = 1;
            lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(wifi_symbol, lv_color_hex(0x707070), 0);
            if(wifi_timer_handle == NULL)
                    wifi_timer_handle = lv_timer_create(wifi_timer_cb, 500, NULL);
            // 获取并显示ssid
            copy_wifi_ssid(wifi_ssid_text);
            lv_label_set_text(wifi_name, (char*)wifi_ssid_text);
        }
    }
}

void light_slider_ev_cb(lv_event_t * e)
{
    lv_event_code_t btn_ev =  lv_event_get_code(e);         // 获取事件代码
    switch (btn_ev)
    {
    case LV_EVENT_VALUE_CHANGED:        // 滑条数值改变事件
    {      
        lv_obj_t* slider_obj = lv_event_get_target(e);      // 获取触发事件的对象
        int32_t slider_val = lv_slider_get_value(slider_obj);     // 获取滑条数值

        /* 写入pwm控制 */
        set_blight_duty_cycle(100 - slider_val);
        break;
    }

    default:
        break;
    }
}

static void lv_settingBtn_handle(lv_event_t* e)
{
    lv_event_code_t btn_ev = lv_event_get_code(e);         // 获取事件代码
    lv_obj_t* obj = lv_event_get_target(e);
    switch (btn_ev)
    {
    case LV_EVENT_CLICKED:
        if(obj == return_btn)
            lv_scr_load_anim(home_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        else if(obj == netConfig)
        {
            if(is_inSmartConfig() == false && isOK_SmartConfig() == false && s_netConfig_flag == false)
            {
                smartConfig_start();        // 启动配网
                if(netConfig_timer_handle == NULL)
                    netConfig_timer_handle = lv_timer_create(netConfig_timer_cb, 500, NULL);
                s_netConfig_flag = true;
                lv_netConfig_OK_lighten_wifiBtn = 1;
            }
            else if((is_inSmartConfig() == false && isOK_SmartConfig() == true && s_netConfig_flag)
            || (is_inSmartConfig() == false && isOK_SmartConfig() == false && s_netConfig_flag))
            {
                if (netConfig_timer_handle)
                {
                    lv_timer_del(netConfig_timer_handle);
                    netConfig_timer_handle = NULL;
                }
                s_netConfig_flag = false;
                lv_obj_set_style_bg_color(netConfig, lv_color_hex(0x707070), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(netConfig_symbol, lv_color_hex(0xffffff), 0);
                isOK_SmartConfig_Set_0();       // 重置配网标志位，为下一次配网做准备
            }
        }
        else if(obj == wifi_btn)
        {
            if(!wifi_btn_state && get_wifi_status() == false && is_inSmartConfig() == false && isOK_SmartConfig() == true)
            {
                esp_wifi_connect();     // 连接wifi
                wifi_btn_state = 1;
                if(wifi_timer_handle == NULL)
                    wifi_timer_handle = lv_timer_create(wifi_timer_cb, 500, NULL);
                s_wifi_flag = true;
            }
            else if((wifi_btn_state && get_wifi_status() == true && is_inSmartConfig() == false && isOK_SmartConfig() == true)
            || (wifi_btn_state && get_wifi_status() == true && is_inSmartConfig() == false && isOK_SmartConfig() == false)
            || (wifi_btn_state && get_wifi_status() == false && is_inSmartConfig() == false && isOK_SmartConfig() == true))
            {
                if(wifi_timer_handle)
                {
                    lv_timer_del(wifi_timer_handle);
                    wifi_timer_handle = NULL;
                }
                s_wifi_flag = false;
                esp_wifi_disconnect();     // 断开wifi
                wifi_btn_state = 0;
            }
            else if(get_wifi_status() == false && is_inSmartConfig() == false && isOK_SmartConfig() == false)
            {
                wifi_btn_state = wifi_btn_state ? 0 : 1;
            }
        }
        break;
      
    default:
        break;
    }
    if(wifi_btn_state)
    {
        lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(wifi_symbol, lv_color_hex(0x707070), 0);
    }
    else
    {
        lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x707070), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(wifi_symbol, lv_color_hex(0xffffff), 0);
        lv_label_set_text(wifi_name, "");
    }
    
}

static void slider_style_init(void)
{
    /* 初始化样式 */
    lv_style_init(&style_part_main);
    lv_style_init(&style_part_knob);
    lv_style_init(&style_part_indicator);

    /* 设置 PART MAIN 样式 */
    lv_style_set_radius(&style_part_main, 15);			// 设置四个角的圆角
    lv_style_set_bg_color(&style_part_main,
    lv_color_hex(0xc43e1c));		// 设置背景颜色
    lv_style_set_pad_top(&style_part_main, -2); 		// 设置顶部(top)的填充(top)大小
    lv_style_set_pad_bottom(&style_part_main, -2);		// 设置底部部(bottom)的填充(top)大小

    /* 设置 PART KNOB 样式 */
    // 将 knob 部分整个设置为透明，就能达到去除旋钮的效果
    lv_style_set_opa(&style_part_knob, LV_OPA_0);

    /* 设置 PART INDICATOR 样式 */
    lv_style_set_radius(&style_part_indicator, 0);		// 设置四个角的圆角
    lv_style_set_bg_color(&style_part_indicator,
    lv_color_hex(0xffffff));		// 设置背景颜色
}

static void create_light_slider(void)
{
    light_slider = lv_slider_create(setting_page_scr);
    lv_obj_align(light_slider, LV_ALIGN_CENTER, 20, 15);
    lv_slider_set_range(light_slider, 10, 100);
    lv_slider_set_value(light_slider, 40, LV_ANIM_ON);
    set_blight_duty_cycle(100 - 40);            // 屏幕默认亮度占空比设为40
    lv_obj_set_size(light_slider, 60, 150);
    // 初始化各部分控件样式
    slider_style_init();

    lv_obj_t* s_light_img = lv_img_create(light_slider);
    lv_img_set_src(s_light_img, &light_img);
    lv_obj_align(s_light_img, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_event_cb(light_slider, light_slider_ev_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_style(light_slider, &style_part_main, LV_PART_MAIN);
    lv_obj_add_style(light_slider, &style_part_knob, LV_PART_KNOB);
    lv_obj_add_style(light_slider, &style_part_indicator, LV_PART_INDICATOR);
}

static void create_voice_slider(void)
{
    voice_slider = lv_slider_create(setting_page_scr);
    lv_obj_align(voice_slider, LV_ALIGN_RIGHT_MID, -20, 15);
    lv_slider_set_range(voice_slider, 10, 100);
    lv_slider_set_value(voice_slider, 50, LV_ANIM_ON);
    lv_obj_set_size(voice_slider, 60, 150);
    // 初始化各部分控件样式
    slider_style_init();

    lv_obj_t* label = lv_label_create(voice_slider);
    lv_label_set_text(label, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x3a404d), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_add_style(voice_slider, &style_part_main, LV_PART_MAIN);
    lv_obj_add_style(voice_slider, &style_part_knob, LV_PART_KNOB);
    lv_obj_add_style(voice_slider, &style_part_indicator, LV_PART_INDICATOR);
}

// wifi connect-disconnect
static void create_wifi_module(void)
{
    wifi_btn = lv_btn_create(setting_page_scr);
    lv_obj_set_size(wifi_btn, 60, 60);
    lv_obj_set_style_radius(wifi_btn, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(wifi_btn, light_slider, LV_ALIGN_TOP_LEFT, -90, 2);
    lv_obj_set_style_shadow_opa(wifi_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x707070), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(wifi_btn, lv_settingBtn_handle, LV_EVENT_CLICKED, NULL);

    // WIFI-icon
    wifi_symbol = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_symbol, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_symbol, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(wifi_symbol, lv_color_hex(0xffffff), 0);
    lv_obj_align(wifi_symbol, LV_ALIGN_CENTER, 0, 0);

    // WIFI-SSID
    wifi_name = lv_label_create(setting_page_scr);
    lv_obj_set_style_text_font(wifi_name, &lv_font_montserrat_12, 0);
    lv_label_set_text(wifi_name, "");
    lv_obj_set_style_text_color(wifi_name, lv_color_white(), 0);
    lv_obj_align_to(wifi_name, wifi_btn, LV_ALIGN_OUT_BOTTOM_MID, -12, 2);
    lv_obj_set_style_anim_speed(wifi_name, 25, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(wifi_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(wifi_name, 30);
}

// WIFI SmartConfig
static void create_netConfig_module(void)
{
    netConfig = lv_btn_create(setting_page_scr);
    lv_obj_set_size(netConfig, 62, 62);
    lv_obj_set_style_radius(netConfig, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align_to(netConfig, wifi_btn, LV_ALIGN_BOTTOM_MID, 0, 98);
    lv_obj_set_style_shadow_opa(netConfig, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(netConfig, lv_color_hex(0x707070), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(netConfig, lv_settingBtn_handle, LV_EVENT_CLICKED, NULL);

    // netConfig
    netConfig_symbol = lv_label_create(netConfig);
    lv_label_set_text(netConfig_symbol, "  NET\nConfig");
    lv_obj_set_style_text_font(netConfig_symbol, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(netConfig_symbol, lv_color_hex(0xffffff), 0);
    lv_obj_align(netConfig_symbol, LV_ALIGN_CENTER, 0, 0);
}

void setting_page_init(void)
{
    setting_page_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(setting_page_scr, lv_color_hex(0x828282), LV_PART_MAIN | LV_STATE_DEFAULT);

    // 返回
    return_btn = lv_btn_create(setting_page_scr);
    lv_obj_set_style_shadow_opa(return_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(return_btn, 35, 35);
    lv_obj_set_style_bg_color(return_btn, lv_color_hex(0x828282), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(return_btn, LV_ALIGN_TOP_LEFT, 10, 13);
    lv_obj_add_event_cb(return_btn, lv_settingBtn_handle, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(return_btn);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    create_voice_slider();
    create_light_slider();
    create_wifi_module();
    create_netConfig_module();
}
