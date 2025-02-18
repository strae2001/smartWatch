#include "driver/gpio.h"
#include "driver/ledc.h"
#include "incl/screen_ledc.h"
#include "esp_log.h"

extern gpio_num_t s_bl_gpio;        // lcd屏幕背光控制引脚 (低电平点亮)

void scr_backlight_control_init(void)
{
    // 初始化定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,   // 低速模式
        .timer_num       = LEDC_TIMER_1,          // 指定定时器1
        .duty_resolution = LEDC_TIMER_12_BIT,     // 占空比分辨率12位, 2^12-1
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK          // 时钟系统自动选择
    };
    ledc_timer_config(&ledc_timer);

    // ledc输出通道初始化
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,      
        .channel        = LEDC_CHANNEL_0,         // 指定输出通道0
        .timer_sel      = LEDC_TIMER_1,           // 关联定时器1
        .intr_type      = LEDC_INTR_DISABLE,      // 禁止中断
        .gpio_num       = s_bl_gpio,           // 指定输出的IO口
        .duty           = 0,                // 设置默认占空比为0
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

esp_err_t set_blight_duty_cycle(int duty_percentage)
{
    if (duty_percentage < 0 || duty_percentage > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    // 根据占空比计算要设置的值（12位分辨率下）
    int duty_value = (1 << 12) * duty_percentage / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_value);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    return ESP_OK;
}