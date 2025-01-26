/***************************************************************************//**
 * @brief RAZ1 Button Press component.
 *\n*******************************************************************************
 * MIT License
 * 
 * Copyright (c) [2025] [Alexei Chetroi]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "af.h"
#include "sl_zigbee_debug_print.h"
#include "sl_sleeptimer.h"
#include "rz_button_press.h"
#include "rz_button_press_config.h"
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT
#ifdef SL_CATALOG_CLI_PRESENT
#include "sl_cli.h"
#define RZ_BUTTON_CLI_IDX_BUTTON_ID 0
#define RZ_BUTTON_CLI_IDX_STATUS    1
#endif // SL_CATALOG_CLI_PRESENT

#define BUTTON_EVENT_X(x) (&(_buttons[x].event))
#define BUTTON_EVENT_HANDLER_X(x) _event_handler_button_##x


// Button state
typedef struct {
  bool pressed_at_boot;
  rz_button_press_status_t status;
  uint32_t ts;
  sl_zigbee_event_t event;
} rz_button_state_t;

// Button states
static rz_button_state_t _buttons[SL_SIMPLE_BUTTON_COUNT];

/**
 * Callback declarations
 */
SL_WEAK void rz_button_press_cb(uint8_t button, rz_button_press_status_t status)
{
  (void)button;
  (void)status;
}

//******** Forward declarations */
static void _generic_button_event_handler(uint8_t button);

/**
 * @brief Button event handlers
 */
#if SL_SIMPLE_BUTTON_COUNT >= 1
static void _event_handler_button_0(sl_zigbee_event_t *event)
{
  (void)event;
  _generic_button_event_handler(0);
}
#endif // SL_SIMPLE_BUTTON_COUNT >= 1
#if SL_SIMPLE_BUTTON_COUNT >= 2
static void _event_handler_button_1(sl_zigbee_event_t *event)
{
  (void)event;
  _generic_button_event_handler(1);
}
#endif // SL_SIMPLE_BUTTON_COUNT >= 2
#if SL_SIMPLE_BUTTON_COUNT >= 3
static void _event_handler_button_2(sl_zigbee_event_t *event)
{
  (void)event;
  _generic_button_event_handler(2);
}
#endif // SL_SIMPLE_BUTTON_COUNT >= 3
#if SL_SIMPLE_BUTTON_COUNT >= 4
static void _event_handler_button_3(sl_zigbee_event_t *event)
{
  (void)event;
  _generic_button_event_handler(3);
}
#endif // SL_SIMPLE_BUTTON_COUNT >= 4
#if SL_SIMPLE_BUTTON_COUNT >= 5
static void _event_handler_button_4(sl_zigbee_event_t *event)
{
  (void)event;
  _generic_button_event_handler(4);
}
#endif // SL_SIMPLE_BUTTON_COUNT >= 5

/***************************************
 * Public functions
 ***************************************/

/**
 * @brief Initialize the button press component.
 */
void rz_button_press_init(void)
{
  // Iterate over buttons and clear timestamps and states
  for (uint8_t i = 0; i < SL_SIMPLE_BUTTON_COUNT; i++) {
    _buttons[i].pressed_at_boot = ( 
      SL_SIMPLE_BUTTON_PRESSED
      == sl_simple_button_get_state(SL_SIMPLE_BUTTON_INSTANCE(i))
    );
    _buttons[i].status = RZ_BUTTON_PRESS_BUTTON_IS_RELEASED;
    _buttons[i].ts = 0;
  }
  #if SL_SIMPLE_BUTTON_COUNT >= 1
  sl_zigbee_af_isr_event_init( BUTTON_EVENT_X(0), BUTTON_EVENT_HANDLER_X(0) );
  #endif
  #if SL_SIMPLE_BUTTON_COUNT >= 2
  sl_zigbee_af_isr_event_init( BUTTON_EVENT_X(1), BUTTON_EVENT_HANDLER_X(1) );
  #endif
  #if SL_SIMPLE_BUTTON_COUNT >= 3
  sl_zigbee_af_isr_event_init( BUTTON_EVENT_X(2), BUTTON_EVENT_HANDLER_X(2) );
  #endif
  #if SL_SIMPLE_BUTTON_COUNT >= 4
  sl_zigbee_af_isr_event_init( BUTTON_EVENT_X(3), BUTTON_EVENT_HANDLER_X(3) );
  #endif
  #if SL_SIMPLE_BUTTON_COUNT >= 5
  sl_zigbee_af_isr_event_init( BUTTON_EVENT_X(4), BUTTON_EVENT_HANDLER_X(4) );
  #endif
}

/***************************************************************************//**
 * This is a callback function that is invoked each time a GPIO interrupt
 * in one of the pushbutton inputs occurs.
 *
 * @param[in] handle Pointer to button instance
 *
 * @note This function is called from ISR context and therefore it is
 *       not possible to call any BGAPI functions directly. The button state
 *       of the instance is updated based on the state change. The state is
 *       updated only after button release and it depends on how long the
 *       button was pressed. The button state is handled by the main loop.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  // Iterate over buttons
  for (uint8_t i = 0; i < SL_SIMPLE_BUTTON_COUNT; i++) {
    // If the handle is applicable
    if (SL_SIMPLE_BUTTON_INSTANCE(i) == handle) {
      // If button is pressed, update ticks
      if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
        // button is pressed
        _buttons[i].ts = sl_sleeptimer_get_tick_count();
        _buttons[i].status = RZ_BUTTON_PRESS_BUTTON_IS_PRESSED;
      } else if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
        // button is released
        _buttons[i].status = RZ_BUTTON_PRESS_BUTTON_IS_RELEASED;
      }
      sl_zigbee_common_rtos_wakeup_stack_task();
      sl_zigbee_event_set_active( BUTTON_EVENT_X(i) );
      break;
    }
  }
}

// ---------------------
// Internal functions
void _generic_button_event_handler(uint8_t button)
{
  uint32_t t_diff, nextUpdate;
  sl_zigbee_event_set_inactive( BUTTON_EVENT_X(button) );
  rz_button_state_t *state = &_buttons[button];

  t_diff = sl_sleeptimer_tick_to_ms( sl_sleeptimer_get_tick_count() - state->ts );
  switch ( state->status ) {
    case RZ_BUTTON_PRESS_BUTTON_IS_RELEASED:
      state->status = RZ_BUTTON_PRESS_BUTTON_IS_RELEASED;
      rz_button_press_cb(button, RZ_BUTTON_PRESS_BUTTON_IS_RELEASED);
      if ( t_diff < RZ_BUTTON_PRESS_DURATION_SHORT_MS ) {
        rz_button_press_cb(button, RZ_BUTTON_PRESS_RELEASED_SHORT);
      } else if ( t_diff < RZ_BUTTON_PRESS_DURATION_MEDIUM_MS ) {
        rz_button_press_cb(button, RZ_BUTTON_PRESS_RELEASED_MEDIUM);
      } else if ( t_diff < RZ_BUTTON_PRESS_DURATION_LONG_MS ) {
        rz_button_press_cb(button, RZ_BUTTON_PRESS_RELEASED_LONG);
      } else {
        rz_button_press_cb(button, RZ_BUTTON_PRESS_RELEASED_VERYLONG);
      }
      break;

    case RZ_BUTTON_PRESS_BUTTON_IS_PRESSED:
      state->status = RZ_BUTTON_PRESS_STILL_PRESSED_SHORT;
      sl_zigbee_event_set_delay_ms( &(state->event), RZ_BUTTON_PRESS_DURATION_SHORT_MS );
      rz_button_press_cb(button, RZ_BUTTON_PRESS_BUTTON_IS_PRESSED);
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_SHORT:
      state->status = RZ_BUTTON_PRESS_STILL_PRESSED_MEDIUM;
      nextUpdate = RZ_BUTTON_PRESS_DURATION_LONG_MS
          - RZ_BUTTON_PRESS_DURATION_MEDIUM_MS;
      sl_zigbee_event_set_delay_ms( &(state->event), nextUpdate );
      rz_button_press_cb(button, RZ_BUTTON_PRESS_STILL_PRESSED_SHORT);
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_MEDIUM:
      state->status = RZ_BUTTON_PRESS_STILL_PRESSED_LONG;
      nextUpdate = RZ_BUTTON_PRESS_DURATION_LONG_MS
          - RZ_BUTTON_PRESS_DURATION_MEDIUM_MS
          - RZ_BUTTON_PRESS_DURATION_SHORT_MS;
      sl_zigbee_event_set_delay_ms( &(state->event), nextUpdate );
      rz_button_press_cb(button, RZ_BUTTON_PRESS_STILL_PRESSED_MEDIUM);
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_LONG:
      state->status = RZ_BUTTON_PRESS_STILL_PRESSED_VERYLONG;
      sl_zigbee_event_set_delay_ms( &(state->event), RZ_BUTTON_PRESS_DURATION_LONG_MS );
      rz_button_press_cb(button, RZ_BUTTON_PRESS_STILL_PRESSED_LONG);
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_VERYLONG:
      rz_button_press_cb(button, RZ_BUTTON_PRESS_STILL_PRESSED_VERYLONG);
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
// CLI related functions

#ifdef SL_CATALOG_CLI_PRESENT
/***************************************************************************//**
 * Command Line Interface callback implementation
 *
 * @param[in] arguments command line argument list
 ******************************************************************************/
void button_press_from_cli(sl_cli_command_arg_t *arguments)
{
  uint8_t button_id;
  rz_button_press_status_t status;
  button_id = sl_cli_get_argument_uint8(arguments, RZ_BUTTON_CLI_IDX_BUTTON_ID);
  status = sl_cli_get_argument_uint8(arguments, RZ_BUTTON_CLI_IDX_STATUS);
  if ( status > RZ_BUTTON_PRESS_RELEASED_VERYLONG) {
    status  = RZ_BUTTON_PRESS_RELEASED_VERYLONG;
  }
  if (button_id < SL_SIMPLE_BUTTON_COUNT) {
    rz_button_press_cb(button_id, status);
  } else {
    sl_zigbee_app_debug_println("Invalid button id, there are only %d buttons", SL_SIMPLE_BUTTON_COUNT);
  }
}
#endif // SL_CATALOG_CLI_PRESENT

