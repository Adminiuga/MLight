#ifndef _MAIN_APP_H
#define _MAIN_APP_H

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif

#if defined(SL_CATALOG_LED0_PRESENT)
#include "sl_led.h"
#include "sl_simple_led_instances.h"
#define led_turn_on(led) sl_led_turn_on(led)
#define led_turn_off(led) sl_led_turn_off(led)
#define led_toggle(led) sl_led_toggle(led)
#define LED0     (&sl_led_led0)
#if defined(SL_CATALOG_LED1_PRESENT)
#define LED1     (&sl_led_led1)
#endif // SL_CATALOG_LED1_PRESENT
#else // !SL_CATALOG_LED0_PRESENT
#define led_turn_on(led)
#define led_turn_off(led)
#define led_toggle(led)
#endif // SL_CATALOG_LED0_PRESENT

#define BUTTON0         0
#define BUTTON1         1

#endif // _MAIN_APP_H