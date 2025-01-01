#ifndef _LOGICAL_LIGHT_H
#define _LOGICAL_LIGHT_H_

#include "af.h"

void llight_disable_external_updates(void);
void llight_enable_external_updates(void);
sl_status_t llight_turnon_light(uint8_t endpoint);
sl_status_t llight_turnoff_light(uint8_t endpoint);
sl_status_t llight_set_level(uint8_t endpoint);
sl_status_t llight_set_rgb_color(uint8_t endpoint, uint16_t red, uint16_t green, uint16_t blue);

#endif // _LOGICAL_LIGHT_H_
