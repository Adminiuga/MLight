#include "em_gpio.h"
#include "sl_component_catalog.h"
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
#include "sl_power_manager.h"
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#include "rgb_light.h"
#include "sl_zigbee_debug_print.h"
#include "sl_simple_rgb_pwm_led.h"
#include "sl_simple_rgb_pwm_led_rgb_led0_config.h"

#ifndef PWM_SLEEP_THRESHOLD
#define PWM_SLEEP_THRESHOLD 254
#endif //PWM_SLEEP_THRESHOLD
#define MAX(a, b) (a > b) ? a : b
#define RGB_LIGHT (&sl_simple_rgb_pwm_led_rgb_led0)

extern sl_led_rgb_pwm_t sl_simple_rgb_pwm_led_rgb_led0;

typedef struct {
  uint16_t  targetLevel;
  bool      isPowerManagementRequested;
} rgb_state_t;

static rgb_state_t rgbState = {
  .targetLevel = 254,
  .isPowerManagementRequested = false
};

    
// Forward declarations for static functions    
#if defined(SL_SIMPLE_RGB_ENABLE_PORT) && defined(SL_SIMPLE_RGB_ENABLE_PIN)
static void rgb_light_enable(void);
static void rgb_light_disable(void);
#else
#define rgb_light_enable(...)
#define rgb_light_disable(...)
#endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN


/**
 * @brief Initialize the RGB LED
 */
void rgb_light_init(void)
{
    #if defined(SL_SIMPLE_RGB_ENABLE_PORT) && defined(SL_SIMPLE_RGB_ENABLE_PIN)
    GPIO_PinModeSet(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN, gpioModePushPull, 0);
    rgb_light_disable();
    // for thunderboard sense 2 enable each of the RGB LEDs
    GPIO_PinModeSet(gpioPortI, 0, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 1, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 2, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 3, gpioModePushPull, 1);
    #endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN
    rgb_light_set_rgbcolor(PWM_SLEEP_THRESHOLD, PWM_SLEEP_THRESHOLD, PWM_SLEEP_THRESHOLD);
}

/**
 * @brief Turn on the RGB LED
 */
void rgb_light_turnon()
{
  sl_zigbee_app_debug_println("Turning on RGB light");
  rgb_light_enable();
  sl_led_turn_on((const sl_led_t*) RGB_LIGHT);
  handle_sleep_requirements();
}

/**
 * @brief Turn off the RGB LED
 */
void rgb_light_turnoff()
{
  sl_zigbee_app_debug_println("Turning off RGB light");
  sl_led_turn_off((const sl_led_t*) RGB_LIGHT);
  handle_sleep_requirements();
  rgb_light_disable();
}

/**
 * @brief Set the RGB color of the LED
 * @param red Red color value [0-255]
 * @param green Green color value [0-255]
 * @param blue Blue color value [0-255]
 */
void rgb_light_set_rgbcolor(uint16_t red, uint16_t green, uint16_t blue)
{
    sl_led_set_rgb_color(RGB_LIGHT, red, green, blue);
    handle_sleep_requirements();
}

/**
 * @brief Set the brightness of the RGB LED recalculated each of the channels
 * @param brightness Brightness value [0-255]
 */
void rgb_light_set_brightness(uint8_t brightness)
{
  uint16_t red, green, blue;
  sl_zigbee_app_debug_print("Setting brightness from %d to %d", rgbState.targetLevel, brightness);
  sl_led_get_rgb_color(RGB_LIGHT, &red, &green, &blue);
  red = MAX(red, 1);
  green = MAX(green, 1);
  blue = MAX(blue, 1);

  sl_zigbee_app_debug_print(" changing RED from %d ", red);
  red = red * brightness / rgbState.targetLevel;
  sl_zigbee_app_debug_print("to %d ", red);
  sl_zigbee_app_debug_print(" changing GREEN from %d ", green);
  green = green * brightness / rgbState.targetLevel;
  sl_zigbee_app_debug_print("to %d ", green);
  sl_zigbee_app_debug_print(" changing BLUE from %d ", blue);
  blue = blue * brightness / rgbState.targetLevel;
  sl_zigbee_app_debug_print("to %d ", blue);

  sl_led_set_rgb_color(RGB_LIGHT, red, green, blue);
  rgbState.targetLevel = MAX(brightness, 1);
  handle_sleep_requirements();
}

/**
 * @brief request proper maximum sleep levels, depending if PWM is being in use
 */
void handle_sleep_requirements()
{
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
  uint16_t red, green, blue;
  sl_led_get_rgb_color(RGB_LIGHT, &red, &green, &blue);
  bool need_em1 = ( 0 != red + green + blue)
                 || ( (red + green + blue) < (3*PWM_SLEEP_THRESHOLD) );
  if ( need_em1 ) {
    // request EM1
    if ( !(rgbState.isPowerManagementRequested) ) sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
  } else {
    // may allow EM2
    if ( rgbState.isPowerManagementRequested ) sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  }
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
}

/**
 * @brief Get the current brightness of the RGB LED
 */
uint8_t rgb_light_get_brightness()
{
    return rgbState.targetLevel;
}

// *****************************************************************************
// Static functions
// ---------------------
#if defined(SL_SIMPLE_RGB_ENABLE_PORT) && defined(SL_SIMPLE_RGB_ENABLE_PIN)
/**
 * @brief enable the RGB LED driver (applicable to Thunberboard Sense 2)
 */
void rgb_light_enable(void)
{
    GPIO_PinOutSet(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN);
}


/**
 * @brief disable the RGB LED driver (applicable to Thunberboard Sense 2)
 */
void rgb_light_disable(void)
{
    GPIO_PinOutClear(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN);
}

#endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN
