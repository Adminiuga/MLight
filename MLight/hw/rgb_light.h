#ifndef _RGB_LIGHT_H
#define _RGB_LIGHT_H_

#include <stdint.h>

void rgb_light_init(void);
void rgb_light_turnon();
void rgb_light_turnoff();
void rgb_light_set_rgbcolor(uint16_t red, uint16_t green, uint16_t blue);
void rgb_light_set_brightness(uint8_t brightness);

/**
 * @brief request proper maximum sleep levels, depending if PWM is being in use
 */
void handle_sleep_requirements();
uint8_t rgb_light_get_brightness();

#endif // _RGB_LIGHT_H_