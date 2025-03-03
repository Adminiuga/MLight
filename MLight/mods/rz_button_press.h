/***************************************************************************//**
 * @brief RAZ1 Button Press component header.
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

#ifndef RZ_BUTTON_PRESS_H_
#define RZ_BUTTON_PRESS_H_

#include "sl_status.h"
#include "stdint.h"

/*******************************************************************************
 * Public definitions
 ******************************************************************************/
typedef enum uint8_t {
    RZ_BUTTON_PRESS_BUTTON_IS_RELEASED = 0,
    RZ_BUTTON_PRESS_BUTTON_IS_PRESSED,
    RZ_BUTTON_PRESS_STILL_PRESSED_SHORT,
    RZ_BUTTON_PRESS_STILL_PRESSED_MEDIUM,
    RZ_BUTTON_PRESS_STILL_PRESSED_LONG,
    RZ_BUTTON_PRESS_STILL_PRESSED_VERYLONG,
    RZ_BUTTON_PRESS_RELEASED_SHORT,
    RZ_BUTTON_PRESS_RELEASED_MEDIUM,
    RZ_BUTTON_PRESS_RELEASED_LONG,
    RZ_BUTTON_PRESS_RELEASED_VERYLONG,
} rz_button_press_status_t;

/**
 * @brief Initialize the button press component.
 */
void rz_button_press_init(void);

/**
 * @brief get current button status
 * @param[in] button - simple button number
 * @return button press status
 */
rz_button_press_status_t rz_button_press_get_status(uint8_t button);

/**
 * @brief check button press sstatus
 * @param[in] button - simple button number
 * @return true if the button is pressed
 */
bool rz_button_press_is_pressed(uint8_t button);

/**
 * @brief check whether the button was pressed at boot (during component initialization)
 * @param[in] button - simple button number
 * @return true if the button was pressed at boot/initialization
 */
bool rz_button_press_was_pressed_at_boot(uint8_t button);

#endif // RZ_BUTTON_PRESS_H_
