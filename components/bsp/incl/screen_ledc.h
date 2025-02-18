#ifndef __SCREEN_LEDC_H
#define __SCREEN_LEDC_H

#include "esp_err.h"

void scr_backlight_control_init(void);

esp_err_t set_blight_duty_cycle(int duty_percentage);

#endif