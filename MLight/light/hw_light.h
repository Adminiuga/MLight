#ifndef _HW_LIGHT_H
#define _HW_LIGHT_H_

#include <stdint.h>
#include "sl_simple_rgb_pwm_led.h"

enum RGB_channel_name_t {
    CH_RED = 0,
    CH_GREEN,
    CH_BLUE,
    CH_WHITE
};

void hw_light_init(void);
void hw_light_turnon();
void hw_light_turnoff();
void hw_light_set_rgbcolor(uint16_t red, uint16_t green, uint16_t blue);
void hw_light_set_brightness(uint8_t brightness);
sl_status_t hw_light_turn_on_ch(const sl_led_rgb_pwm_t *led_handle, enum RGB_channel_name_t ch);
sl_status_t hw_light_turn_off_ch(const sl_led_rgb_pwm_t *led_handle, enum RGB_channel_name_t ch);

/**
 * @brief request proper maximum sleep levels, depending if PWM is being in use
 */
void handle_sleep_requirements();
uint8_t hw_light_get_brightness();

#endif // _HW_LIGHT_H_