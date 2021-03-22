/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii-gamepad.h - delarations of Wii Gamepad routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 Frank Meyer - frank(at)fli4l.de
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
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef WII_GAMEPAD_H
#define WII_GAMEPAD_H

#if defined (STM32F10X)
#  include "stm32f10x.h"
#elif defined (STM32F4XX)
#  include "stm32f4xx.h"
#endif

#define WII_BUTTON_DIRECTION_RIGHT_MASK     0x8000                  // Button: Direction right          ---
#define WII_BUTTON_DIRECTION_DOWN_MASK      0x4000                  // Button: Direction down           ---
#define WII_BUTTON_TRIGGER_LEFT_MASK        0x2000                  // Button: Trigger left             ---
#define WII_BUTTON_MINUS_MASK               0x1000                  // Button: Minus                    ---
#define WII_BUTTON_HOME_MASK                0x0800                  // Button: Home                     ---
#define WII_BUTTON_PLUS_MASK                0x0400                  // Button: Plus                     ---
#define WII_BUTTON_TRIGGER_RIGHT_MASK       0x0200                  // Button: Trigger right            ---
#define WII_BUTTON_C_MASK                   0x0100                  // ---                              Button: C
#define WII_BUTTON_Z_LEFT_MASK              0x0080                  // Button: Z left                   Button: Z
#define WII_BUTTON_B_MASK                   0x0040                  // Button: B                        ---
#define WII_BUTTON_Y_MASK                   0x0020                  // Button: Y                        ---
#define WII_BUTTON_A_MASK                   0x0010                  // Button: A                        ---
#define WII_BUTTON_X_MASK                   0x0008                  // Button: X                        ---
#define WII_BUTTON_Z_RIGHT_MASK             0x0004                  // Button: Z right                  ---
#define WII_BUTTON_DIRECTION_LEFT_MASK      0x0002                  // Button: Direction left           ---
#define WII_BUTTON_DIRECTION_UP_MASK        0x0001                  // Button: Direction up             ---

#define WII_GAMEPAD_LX_MIN_VALUE            0
#define WII_GAMEPAD_LX_MAX_VALUE            63
#define WII_GAMEPAD_LX_CENTER_VALUE         32
#define WII_GAMEPAD_LX_LEFT_VALUE           (WII_GAMEPAD_LX_CENTER_VALUE - WII_GAMEPAD_LX_CENTER_VALUE / 4)
#define WII_GAMEPAD_LX_RIGHT_VALUE          (WII_GAMEPAD_LX_CENTER_VALUE + WII_GAMEPAD_LX_CENTER_VALUE / 4)

#define WII_GAMEPAD_LY_MIN_VALUE            0
#define WII_GAMEPAD_LY_MAX_VALUE            63
#define WII_GAMEPAD_LY_CENTER_VALUE         32
#define WII_GAMEPAD_LY_DOWN_VALUE           (WII_GAMEPAD_LY_CENTER_VALUE - WII_GAMEPAD_LY_CENTER_VALUE / 4)
#define WII_GAMEPAD_LY_UP_VALUE             (WII_GAMEPAD_LY_CENTER_VALUE + WII_GAMEPAD_LY_CENTER_VALUE / 4)

#define WII_GAMEPAD_RX_MIN_VALUE            0
#define WII_GAMEPAD_RX_MAX_VALUE            31
#define WII_GAMEPAD_RX_CENTER_VALUE         16
#define WII_GAMEPAD_RX_LEFT_VALUE           (WII_GAMEPAD_RX_CENTER_VALUE - WII_GAMEPAD_RX_CENTER_VALUE / 4)
#define WII_GAMEPAD_RX_RIGHT_VALUE          (WII_GAMEPAD_RX_CENTER_VALUE + WII_GAMEPAD_RX_CENTER_VALUE / 4)

#define WII_GAMEPAD_RY_MIN_VALUE            0
#define WII_GAMEPAD_RY_MAX_VALUE            31
#define WII_GAMEPAD_RY_CENTER_VALUE         16
#define WII_GAMEPAD_RY_DOWN_VALUE           (WII_GAMEPAD_RY_CENTER_VALUE - WII_GAMEPAD_RY_CENTER_VALUE / 4)
#define WII_GAMEPAD_RY_UP_VALUE             (WII_GAMEPAD_RY_CENTER_VALUE + WII_GAMEPAD_RY_CENTER_VALUE / 4)

#define WII_NUNCHUK_LX_MIN_VALUE            0
#define WII_NUNCHUK_LX_MAX_VALUE            255
#define WII_NUNCHUK_LX_CENTER_VALUE         128
#define WII_NUNCHUK_LX_LEFT_VALUE           (WII_NUNCHUK_LX_CENTER_VALUE - WII_NUNCHUK_LX_CENTER_VALUE / 4)
#define WII_NUNCHUK_LX_RIGHT_VALUE          (WII_NUNCHUK_LX_CENTER_VALUE + WII_NUNCHUK_LX_CENTER_VALUE / 4)

#define WII_NUNCHUK_LY_MIN_VALUE            0
#define WII_NUNCHUK_LY_MAX_VALUE            255
#define WII_NUNCHUK_LY_CENTER_VALUE         128
#define WII_NUNCHUK_LY_DOWN_VALUE           (WII_NUNCHUK_LY_CENTER_VALUE - WII_NUNCHUK_LY_CENTER_VALUE / 4)
#define WII_NUNCHUK_LY_UP_VALUE             (WII_NUNCHUK_LY_CENTER_VALUE + WII_NUNCHUK_LY_CENTER_VALUE / 4)

typedef struct
{
    uint8_t     lx;                                                 // left analog stick x (0-63)
    uint8_t     ly;                                                 // left analog stick y (0-63)
    uint8_t     rx;                                                 // right analog stick x (0-31)
    uint8_t     ry;                                                 // right analog stick y (0-31)
    uint8_t     lt;                                                 // left trigger (0-31)
    uint8_t     rt;                                                 // right trigger (0-31)
    uint16_t    buttons;                                            // buttons
} WII_DATA;

#define WII_STATE_IDLE                      0
#define WII_STATE_BUSY                      1
extern uint_fast8_t                         wii_state;

#define WII_TYPE_NONE                       0
#define WII_TYPE_NUNCHUK                    1
#define WII_TYPE_GAMEPAD                    2
extern uint_fast8_t                         wii_type;
extern WII_DATA                             wii_data;

extern uint_fast8_t                         wii_start_transmission (void);
extern uint_fast8_t                         wii_finished_transmission (void);
extern uint_fast8_t                         wii_init (void);

#endif // WII_GAMEPAD_H
