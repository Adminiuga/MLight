#include "em_gpio.h"
#include "rgb_light.h"
#include "sl_zigbee_debug_print.h"
#include "sl_simple_rgb_pwm_led.h"
#include "sl_simple_rgb_pwm_led_rgb_led0_config.h"

#define MAX(a, b) (a > b) ? a : b

extern sl_led_rgb_pwm_t sl_simple_rgb_pwm_led_rgb_led0;
static uint8_t targetLevel = 254;

    
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
}

/**
 * @brief Turn on the RGB LED
 */
void rgb_light_turnon()
{
  rgb_light_enable();
  //sl_simple_rgb_pwm_led_turn_on(sl_simple_rgb_pwm_led_rgb_led0.led_common.context);
  GPIO_PinOutSet(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PIN);
  GPIO_PinOutSet(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PIN);
  GPIO_PinOutSet(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PIN);
}

/**
 * @brief Turn off the RGB LED
 */
void rgb_light_turnoff()
{
  GPIO_PinOutClear(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RED_PIN);
  GPIO_PinOutClear(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_GREEN_PIN);
  GPIO_PinOutClear(SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PORT, SL_SIMPLE_RGB_PWM_LED_RGB_LED0_BLUE_PIN);
  //sl_simple_rgb_pwm_led_turn_off(sl_simple_rgb_pwm_led_rgb_led0.led_common.context);
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
    sl_led_set_rgb_color(&sl_simple_rgb_pwm_led_rgb_led0, red, green, blue);
}

/**
 * @brief Set the brightness of the RGB LED recalculated each of the channels
 * @param brightness Brightness value [0-255]
 */
void rgb_light_set_brightness(uint8_t brightness)
{
  uint16_t red, green, blue;
  sl_zigbee_app_debug_print("Setting brightness from %d to %d", targetLevel, brightness);
  sl_led_get_rgb_color(&sl_simple_rgb_pwm_led_rgb_led0, &red, &green, &blue);
  red = MAX(red, 1);
  green = MAX(green, 1);
  blue = MAX(blue, 1);

  sl_zigbee_app_debug_print(" changing RED from %d ", red);
  red = red * brightness / targetLevel;
  sl_zigbee_app_debug_print("to %d ", red);
  sl_zigbee_app_debug_print(" changing GREEN from %d ", green);
  green = green * brightness / targetLevel;
  sl_zigbee_app_debug_print("to %d ", green);
  sl_zigbee_app_debug_print(" changing BLUE from %d ", blue);
  blue = blue * brightness / targetLevel;
  sl_zigbee_app_debug_print("to %d ", blue);


  sl_led_set_rgb_color(&sl_simple_rgb_pwm_led_rgb_led0, red, green, blue);
  targetLevel = MAX(brightness, 1);
}

/**
 * @brief Get the current brightness of the RGB LED
 */
uint8_t rgb_light_get_brightness()
{
    return targetLevel;
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
