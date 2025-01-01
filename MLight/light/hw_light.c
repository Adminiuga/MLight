#include "em_gpio.h"
#include "sl_component_catalog.h"
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
#include "sl_power_manager.h"
#if SL_POWER_MANAGER_DEBUG == 1
#include "sl_power_manager_debug.h"
#endif // SL_POWER_MANAGER_DEBUG == 1
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#include "hw_light.h"
#include "sl_zigbee_debug_print.h"
#include "sl_simple_rgb_pwm_led.h"
#include "sl_simple_rgb_pwm_led_rgb_led0_config.h"

#ifndef PWM_SLEEP_THRESHOLD
#define PWM_SLEEP_THRESHOLD (SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION - 2)
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
static void hw_light_enable(void);
static void hw_light_disable(void);
#else
#define hw_light_enable(...)
#define hw_light_disable(...)
#endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
static bool _needs_em1();
static void _request_em1(bool allow_em1_only);
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
static sl_led_pwm_t* _rgb_channel_to_context( const sl_simple_rgb_pwm_led_context_t *context, enum RGB_channel_name_t ch_name );

/**
 * @brief Initialize the RGB LED
 */
void hw_light_init(void)
{
    #if defined(SL_SIMPLE_RGB_ENABLE_PORT) && defined(SL_SIMPLE_RGB_ENABLE_PIN)
    GPIO_PinModeSet(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN, gpioModePushPull, 0);
    hw_light_disable();
    // for thunderboard sense 2 enable each of the RGB LEDs
    GPIO_PinModeSet(gpioPortI, 0, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 1, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 2, gpioModePushPull, 1);
    GPIO_PinModeSet(gpioPortI, 3, gpioModePushPull, 1);
    #endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN
    hw_light_set_rgbcolor(
        SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION-1,
        SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION-1,
        SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION >> 1
    );
}

static void print_led_state()
{
  uint16_t r, g, b;
  sl_led_get_rgb_color(RGB_LIGHT, &r, &g, &b);
  sl_zigbee_app_debug_println("Current RGB light state: %02x/%02x/%02x, On/Off: %02x",
        r, g, b, sl_led_get_state( (const sl_led_t*) RGB_LIGHT ) );
}

/**
 * @brief Turn on the RGB LED
 */
void hw_light_turnon()
{
  sl_zigbee_app_debug_println("Turning on RGB light");
  hw_light_enable();
  sl_led_turn_on((const sl_led_t*) RGB_LIGHT);
  handle_sleep_requirements();
  print_led_state();
}

/**
 * @brief Turn off the RGB LED
 */
void hw_light_turnoff()
{
  sl_zigbee_app_debug_println("Turning off RGB light");
  sl_led_turn_off((const sl_led_t*) RGB_LIGHT);
  handle_sleep_requirements();
  hw_light_disable();
  print_led_state();
}

/**
 * @brief Set the RGB color of the LED
 * @param red Red color value [0-255]
 * @param green Green color value [0-255]
 * @param blue Blue color value [0-255]
 */
void hw_light_set_rgbcolor(uint16_t red, uint16_t green, uint16_t blue)
{
    sl_led_set_rgb_color(RGB_LIGHT, red, green, blue);
    if ( SL_LED_CURRENT_STATE_OFF == sl_led_get_state( (const sl_led_t*) RGB_LIGHT ) ) {
      sl_led_turn_off( (const sl_led_t*) RGB_LIGHT );
    }
    handle_sleep_requirements();
}

/**
 * @brief Set the brightness of the RGB LED recalculated each of the channels
 * @param brightness Brightness value [0-255]
 */
sl_status_t hw_light_set_brightness(uint8_t brightness)
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

  hw_light_set_rgbcolor(red, green, blue);
  rgbState.targetLevel = MAX(brightness, 1);

  return SL_STATUS_OK;
}

sl_status_t hw_light_set_level_ch(enum RGB_channel_name_t ch_name, uint16_t level)
{
  sl_simple_rgb_pwm_led_context_t *context = RGB_LIGHT->led_common.context;
  sl_led_pwm_t *ch = _rgb_channel_to_context( context, ch_name );
  if ( NULL == ch ) return SL_STATUS_FAIL;

  if ( level == SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION - 2) level = SL_SIMPLE_RGB_PWM_LED_RGB_LED0_RESOLUTION - 1;
  sl_pwm_led_set_color( ch, level );
  if ( SL_LED_CURRENT_STATE_OFF == ch->state ) sl_pwm_led_stop( ch );
  handle_sleep_requirements();
  return SL_STATUS_OK;
}

/**
 * @brief Turn off specific channel of the RGB led
 * @param[in] ch -- channel name
 * @return    Status Code:
 *            - SL_STATUS_OK   Success
 *            - SL_STATUS_FAIL Error
 */
sl_status_t hw_light_turn_on_ch(enum RGB_channel_name_t ch_name)
{
  return hw_light_turn_ch_onoff( ch_name, true );
}

/**
 * @brief Turn on specific channel of the RGB led
 * @param[in] ch_name -- channel name
 * @return    Status Code:
 *            - SL_STATUS_OK   Success
 *            - SL_STATUS_FAIL Error
 */
sl_status_t hw_light_turn_off_ch(enum RGB_channel_name_t ch_name)
{
  return hw_light_turn_ch_onoff( ch_name, false );
}

/**
 * @brief Turn on specific channel of the RGB led
 * @param[in] ch_name -- channel name
 * @param[in] turn_on -- bool, true to turn the channel on, false to turn off
 * @return    Status Code:
 *            - SL_STATUS_OK   Success
 *            - SL_STATUS_FAIL Error
 */
sl_status_t hw_light_turn_ch_onoff(enum RGB_channel_name_t ch_name, bool turn_on)
{
  sl_simple_rgb_pwm_led_context_t *context = RGB_LIGHT->led_common.context;
  sl_led_pwm_t *ch = _rgb_channel_to_context( context, ch_name );
  if ( NULL == ch ) return SL_STATUS_FAIL;

  if ( turn_on ) {
    sl_pwm_led_start( ch );
    ch->state = SL_LED_CURRENT_STATE_ON;
    context->state = SL_LED_CURRENT_STATE_ON;
  } else {
    sl_pwm_led_stop( ch );
    ch->state = SL_LED_CURRENT_STATE_OFF;
    context->state = SL_LED_CURRENT_STATE_OFF;
  }
  handle_sleep_requirements();
  return SL_STATUS_OK;
}

/**
 * @brief request proper maximum sleep levels, depending if PWM is being in use
 */
void handle_sleep_requirements()
{
#ifdef SL_CATALOG_POWER_MANAGER_PRESENT

  if ( _needs_em1() ) {
    // request EM1
    if ( !(rgbState.isPowerManagementRequested) ) _request_em1(true);
  } else {
    // may allow EM2
    if ( rgbState.isPowerManagementRequested ) _request_em1(false);
  }
#if (!defined(SL_CATALOG_POWER_MANAGER_NO_DEEPSLEEP_PRESENT) && (SL_POWER_MANAGER_DEBUG == 1))
  sl_power_manager_debug_print_em_requirements();
#endif // NO_DEEP_SLEEP && PM_DEBUG
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
}

/**
 * @brief Get the current brightness of the RGB LED
 */
uint8_t hw_light_get_brightness()
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
void hw_light_enable(void)
{
    GPIO_PinOutSet(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN);
}


/**
 * @brief disable the RGB LED driver (applicable to Thunberboard Sense 2)
 */
void hw_light_disable(void)
{
    GPIO_PinOutClear(SL_SIMPLE_RGB_ENABLE_PORT, SL_SIMPLE_RGB_ENABLE_PIN);
}

#endif // SL_SIMPLE_RGB_ENABLE_PORT && SL_SIMPLE_RGB_ENABLE_PIN

#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
static bool _needs_em1()
{
  sl_simple_rgb_pwm_led_context_t *ctx = RGB_LIGHT->led_common.context;
  uint16_t red, green, blue;
  sl_simple_rgb_pwm_led_get_color(ctx, &red, &green, &blue);
  
  return ( red && (red < PWM_SLEEP_THRESHOLD) && (SL_LED_CURRENT_STATE_ON == ctx->red->state) )
         || ( green && (green < PWM_SLEEP_THRESHOLD) && (SL_LED_CURRENT_STATE_ON == ctx->green->state) )
         || ( blue && (blue < PWM_SLEEP_THRESHOLD) && (SL_LED_CURRENT_STATE_ON == ctx->blue->state) );
}

static void _request_em1(bool allow_em1_only)
{
  if ( allow_em1_only ) {
    sl_zigbee_app_debug_println("Requesting EM1 only");
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
    rgbState.isPowerManagementRequested = true;
  } else {
    sl_zigbee_app_debug_println("Allowing EM2 sleep");
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
    rgbState.isPowerManagementRequested = false;
  }
}
#endif // SL_CATALOG_POWER_MANAGER_PRESENT

/**
 * @brief get PWM Led channel from RGB instance based on channel name
 * @param[in] context RWB PWM Context
 * @param[in] ch_name channel name
 * @return sl_led_pwm pointer
 */
static sl_led_pwm_t* _rgb_channel_to_context( const sl_simple_rgb_pwm_led_context_t *context, enum RGB_channel_name_t ch_name )
{
  switch ( ch_name ) {
    case CH_RED:
      return context->red;
      break;

    case CH_GREEN:
      return context->green;
      break;

    case CH_BLUE:
      return context->blue;
      break;

    default:
      return NULL;
  }
}