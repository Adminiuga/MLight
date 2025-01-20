#ifndef _LOGICAL_LIGHT_H
#define _LOGICAL_LIGHT_H_

#include "af.h"

void llight_disable_external_updates(void);
void llight_enable_external_updates(void);
sl_status_t llight_turnon_light(uint8_t endpoint);
sl_status_t llight_turnoff_light(uint8_t endpoint);
sl_status_t llight_turnonoff_light(uint8_t endpoint, bool turnOn);
sl_status_t llight_set_level(uint8_t endpoint, uint8_t level);

#endif // _LOGICAL_LIGHT_H_
