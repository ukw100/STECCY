/*-------------------------------------------------------------------------------------------------------------------------------------------
 * joystick.c - joystick routines
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
#include "z80.h"
#include "console.h"
#include "zxio.h"
#include "i2c1-dma.h"
#include "ps2key.h"
#include "keypress.h"
#include "wii-gamepad.h"
#include "joystick.h"

#define JOYSTICK_UP     0x01
#define JOYSTICK_DOWN   0x02
#define JOYSTICK_LEFT   0x04
#define JOYSTICK_RIGHT  0x08
#define JOYSTICK_FIRE   0x10

uint_fast8_t            joystick_state      = JOYSTICK_STATE_IDLE;
uint_fast8_t            joystick_is_online  = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * gamepad_decode_buttons() - decode gamepad button data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
gamepad_decode_buttons (void)
{
    uint_fast8_t buttons = 0x00;

    if (wii_data.lx > WII_GAMEPAD_LX_RIGHT_VALUE)                       // left stick right ---> joystick right
    {
        buttons |= JOYSTICK_RIGHT;
    }
    else if (wii_data.lx < WII_GAMEPAD_LX_LEFT_VALUE)                   // left stick left ---> joystick left
    {
        buttons |= JOYSTICK_LEFT;
    }

    if (wii_data.ly > WII_GAMEPAD_LY_UP_VALUE)                          // left stick up ---> joystick up
    {
        buttons |= JOYSTICK_UP;
    }
    else if (wii_data.ly < WII_GAMEPAD_LY_DOWN_VALUE)                   // left stick down ---> joystick down
    {
        buttons |= JOYSTICK_DOWN;
    }

    if (buttons == 0x00)                                                // check right stick only if left stick not used
    {
        if (wii_data.rx > WII_GAMEPAD_RX_RIGHT_VALUE)                   // right stick right ---> joystick right
        {
            buttons |= JOYSTICK_RIGHT;
        }
        else if (wii_data.rx < WII_GAMEPAD_RX_LEFT_VALUE)               // right stick left ---> joystick left
        {
            buttons |= JOYSTICK_LEFT;
        }

        if (wii_data.ry > WII_GAMEPAD_RY_UP_VALUE)                      // right stick up ---> joystick up
        {
            buttons |= JOYSTICK_UP;
        }
        else if (wii_data.ry < WII_GAMEPAD_RY_DOWN_VALUE)               // right stick down ---> joystick down
        {
            buttons |= JOYSTICK_DOWN;
        }
    }

    if (buttons == 0x00)                                                // check direction buttons only if left & right stick not used
    {
        if (wii_data.buttons & WII_BUTTON_DIRECTION_LEFT_MASK)          // direction left -> joystick left
        {
            buttons |= JOYSTICK_LEFT;
        }
        else if (wii_data.buttons & WII_BUTTON_DIRECTION_RIGHT_MASK)    // direction right --> joystick right
        {
            buttons |= JOYSTICK_RIGHT;
        }

        if (wii_data.buttons & WII_BUTTON_DIRECTION_UP_MASK)            // direction up ---> joystick up
        {
            buttons |= JOYSTICK_UP;
        }
        else if (wii_data.buttons & WII_BUTTON_DIRECTION_DOWN_MASK)     // direction down ---> joystick down
        {
            buttons |= JOYSTICK_DOWN;
        }
    }

    if ((wii_data.buttons & WII_BUTTON_Z_LEFT_MASK)        ||           // button Z left        -> Fire
        (wii_data.buttons & WII_BUTTON_Z_RIGHT_MASK)       ||           // button Z right       -> Fire
        (wii_data.buttons & WII_BUTTON_TRIGGER_LEFT_MASK)  ||           // button Trigger Left  -> Fire
        (wii_data.buttons & WII_BUTTON_TRIGGER_RIGHT_MASK) ||           // button Trigger Left  -> Fire
        (wii_data.buttons & WII_BUTTON_A_MASK)             ||           // button A             -> Fire
        (wii_data.buttons & WII_BUTTON_B_MASK)             ||           // button B             -> Fire
        (wii_data.buttons & WII_BUTTON_X_MASK)             ||           // button X             -> Fire
        (wii_data.buttons & WII_BUTTON_Y_MASK))                         // button Y             -> Fire
    {
        buttons |= JOYSTICK_FIRE;
    }

    if (wii_data.buttons & WII_BUTTON_MINUS_MASK)                       // Select (-) ---> change turbo speed
    {
        z80_next_turbo_mode ();
    }
    if (wii_data.buttons & WII_BUTTON_HOME_MASK)                        // Home ---> Menu
    {
        ps2key_setmode (PS2KEY_GET_MODE);
        z80_leave_focus ();
    }
    else if (wii_data.buttons & WII_BUTTON_PLUS_MASK)                  // Start (+) ---> Z80 reset
    {
        z80_reset ();
    }

    return buttons;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * gamepad_to_scancode () - convert gamepad button data to PS/2 scancode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast16_t
gamepad_to_scancode (void)
{
    uint_fast16_t           scancode = SCANCODE_NONE;

static uint_fast8_t x = 0;
x++;
if (x < 3)
{
    console_printf ("lx=%d\r\n", wii_data.lx);
    console_printf ("ly=%d\r\n", wii_data.ly);
    console_printf ("rx=%d\r\n", wii_data.rx);
    console_printf ("ry=%d\r\n", wii_data.ry);
    console_printf ("btn=%04X\r\n", wii_data.buttons);
}
    if (wii_data.lx > WII_GAMEPAD_LX_RIGHT_VALUE)
    {
        scancode = SCANCODE_R_ARROW;
    }
    else if (wii_data.lx < WII_GAMEPAD_LX_LEFT_VALUE)
    {
        scancode = SCANCODE_L_ARROW;
    }
    else if (wii_data.ly > WII_GAMEPAD_LY_UP_VALUE)
    {
        scancode = SCANCODE_U_ARROW;
    }
    else if (wii_data.ly < WII_GAMEPAD_LY_DOWN_VALUE)
    {
        scancode = SCANCODE_D_ARROW;
    }

    if (wii_data.rx > WII_GAMEPAD_RX_RIGHT_VALUE)
    {
        scancode = SCANCODE_R_ARROW;
    }
    else if (wii_data.rx < WII_GAMEPAD_RX_LEFT_VALUE)
    {
        scancode = SCANCODE_L_ARROW;
    }
    else if (wii_data.ry > WII_GAMEPAD_RY_UP_VALUE)
    {
        scancode = SCANCODE_U_ARROW;
    }
    else if (wii_data.ry < WII_GAMEPAD_RY_DOWN_VALUE)
    {
        scancode = SCANCODE_D_ARROW;
    }

    else if (wii_data.buttons & WII_BUTTON_DIRECTION_LEFT_MASK)             // direction left
    {
        scancode = SCANCODE_L_ARROW;
    }
    else if (wii_data.buttons & WII_BUTTON_DIRECTION_RIGHT_MASK)            // direction right
    {
        scancode = SCANCODE_R_ARROW;
    }
    else if (wii_data.buttons & WII_BUTTON_DIRECTION_UP_MASK)               // direction up
    {
        scancode = SCANCODE_U_ARROW;
    }
    else if (wii_data.buttons & WII_BUTTON_DIRECTION_DOWN_MASK)             // direction down
    {
        scancode = SCANCODE_D_ARROW;
    }

    else if ((wii_data.buttons & WII_BUTTON_Z_LEFT_MASK)        ||          // button Z left        -> Enter
             (wii_data.buttons & WII_BUTTON_Z_RIGHT_MASK)       ||          // button Z right       -> Enter
             (wii_data.buttons & WII_BUTTON_TRIGGER_LEFT_MASK)  ||          // button Trigger Left  -> Enter
             (wii_data.buttons & WII_BUTTON_TRIGGER_RIGHT_MASK) ||          // button Trigger Left  -> Enter
             (wii_data.buttons & WII_BUTTON_A_MASK)             ||          // button A             -> Enter
             (wii_data.buttons & WII_BUTTON_B_MASK)             ||          // button B             -> Enter
             (wii_data.buttons & WII_BUTTON_X_MASK)             ||          // button X             -> Enter
             (wii_data.buttons & WII_BUTTON_Y_MASK))                        // button Y             -> Enter
    {
        scancode = SCANCODE_ENTER;
    }

    else if ((wii_data.buttons & WII_BUTTON_MINUS_MASK) ||                  // button Select (-)    -> Escape
             (wii_data.buttons & WII_BUTTON_HOME_MASK)  ||                  // button Home          -> Escape
             (wii_data.buttons & WII_BUTTON_PLUS_MASK))                     // button Start (+)     -> Escape
    {
        scancode = SCANCODE_ESC;
    }


    return scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * nunchuk_decode_buttons() - decode nunchuk button data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
nunchuk_decode_buttons (void)
{
    uint_fast8_t buttons = 0x00;

    if (wii_data.lx > WII_NUNCHUK_LX_RIGHT_VALUE)
    {
        buttons |= JOYSTICK_RIGHT;
    }
    else if (wii_data.lx < WII_NUNCHUK_LX_LEFT_VALUE)
    {
        buttons |= JOYSTICK_LEFT;
    }

    if (wii_data.ly > WII_NUNCHUK_LY_UP_VALUE)
    {
        buttons |= JOYSTICK_UP;
    }
    else if (wii_data.ly < WII_NUNCHUK_LY_DOWN_VALUE)
    {
        buttons |= JOYSTICK_DOWN;
    }

    if (wii_data.buttons & WII_BUTTON_Z_LEFT_MASK)
    {
        buttons |= JOYSTICK_FIRE;
    }

    if (wii_data.buttons & WII_BUTTON_C_MASK)
    {
        ps2key_setmode (PS2KEY_GET_MODE);
        z80_leave_focus ();
    }

    return buttons;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * nunchuk_to_scancode () - convert nunchuk button data to PS/2 scancode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast16_t
nunchuk_to_scancode (void)
{
    uint_fast16_t           scancode = SCANCODE_NONE;

    if (wii_data.lx > WII_NUNCHUK_LX_RIGHT_VALUE)
    {
        scancode = SCANCODE_R_ARROW;
    }
    else if (wii_data.lx < WII_NUNCHUK_LX_LEFT_VALUE)
    {
        scancode = SCANCODE_L_ARROW;
    }
    else if (wii_data.ly > WII_NUNCHUK_LY_UP_VALUE)
    {
        scancode = SCANCODE_U_ARROW;
    }
    else if (wii_data.ly < WII_NUNCHUK_LY_DOWN_VALUE)
    {
        scancode = SCANCODE_D_ARROW;
    }
    else if (wii_data.buttons & WII_BUTTON_Z_LEFT_MASK)
    {
        scancode = SCANCODE_ENTER;
    }
    else if (wii_data.buttons & WII_BUTTON_C_MASK)
    {
        scancode = SCANCODE_ESC;
    }

    return scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * joystick_to_scancode () - convert wii to scancode - used by STECCY menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
joystick_to_scancode (void)
{
    uint_fast16_t   scancode = SCANCODE_NONE;

    if (wii_type == WII_TYPE_GAMEPAD)
    {
        scancode = gamepad_to_scancode ();
    }
    else
    {
        scancode = nunchuk_to_scancode ();
    }
    return scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * joystick_decode_data() - decode joystick data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
joystick_decode_data (void)
{
    static uint_fast8_t     last_buttons = 0;
    uint_fast8_t            buttons;

    if (wii_type == WII_TYPE_GAMEPAD)
    {
        buttons = gamepad_decode_buttons ();
    }
    else
    {
        buttons = nunchuk_decode_buttons ();
    }

    if (last_buttons != buttons)
    {
        uint_fast8_t changed_buttons = last_buttons ^ buttons;

        if (changed_buttons & JOYSTICK_LEFT)                                                            // changed left
        {
            if (buttons & JOYSTICK_LEFT)                                                                // pressed left
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_press_key (MATRIX_KEY_5_IDX);              break;  // cursor joystick:   "5"
                    case JOYSTICK_SINCLAIR_P1:  zxio_press_key (MATRIX_KEY_6_IDX);              break;  // sinclair joystick: "6"
                    case JOYSTICK_SINCLAIR_P2:  zxio_press_key (MATRIX_KEY_1_IDX);              break;  // sinclair joystick: "1"
                    case JOYSTICK_KEMPSTON:     zxio_press_key (MATRIX_KEMPSTON_LEFT_IDX);      break;  // kempston joystick:  2
                }
            }
            else                                                                                        // released left
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_release_key (MATRIX_KEY_5_IDX);            break;  // cursor joystick:   "5"
                    case JOYSTICK_SINCLAIR_P1:  zxio_release_key (MATRIX_KEY_6_IDX);            break;  // sinclair joystick: "6"
                    case JOYSTICK_SINCLAIR_P2:  zxio_release_key (MATRIX_KEY_1_IDX);            break;  // sinclair joystick: "1"
                    case JOYSTICK_KEMPSTON:     zxio_release_key (MATRIX_KEMPSTON_LEFT_IDX);    break;  // kempston joystick:  2
                }
            }
        }

        if (changed_buttons & JOYSTICK_RIGHT)                                                           // changed right
        {
            if (buttons & JOYSTICK_RIGHT)                                                               // pressed right
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_press_key (MATRIX_KEY_8_IDX);              break;  // cursor joystick:   "8"
                    case JOYSTICK_SINCLAIR_P1:  zxio_press_key (MATRIX_KEY_7_IDX);              break;  // sinclair joystick: "7"
                    case JOYSTICK_SINCLAIR_P2:  zxio_press_key (MATRIX_KEY_2_IDX);              break;  // sinclair joystick: "2"
                    case JOYSTICK_KEMPSTON:     zxio_press_key (MATRIX_KEMPSTON_RIGHT_IDX);     break;  // kempston joystick:  1
                }
            }
            else                                                        // released right
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_release_key (MATRIX_KEY_8_IDX);            break;  // cursor joystick:   "8"
                    case JOYSTICK_SINCLAIR_P1:  zxio_release_key (MATRIX_KEY_7_IDX);            break;  // sinclair joystick: "7"
                    case JOYSTICK_SINCLAIR_P2:  zxio_release_key (MATRIX_KEY_2_IDX);            break;  // sinclair joystick: "2"
                    case JOYSTICK_KEMPSTON:     zxio_release_key (MATRIX_KEMPSTON_RIGHT_IDX);   break;  // kempston joystick:  1
                }
            }
        }

        if (changed_buttons & JOYSTICK_UP)                                                              // changed up
        {
            if (buttons & JOYSTICK_UP)                                                                  // pressed up
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_press_key (MATRIX_KEY_7_IDX);              break;  // cursor joystick:   "7"
                    case JOYSTICK_SINCLAIR_P1:  zxio_press_key (MATRIX_KEY_9_IDX);              break;  // sinclair joystick: "9"
                    case JOYSTICK_SINCLAIR_P2:  zxio_press_key (MATRIX_KEY_4_IDX);              break;  // sinclair joystick: "4"
                    case JOYSTICK_KEMPSTON:     zxio_press_key (MATRIX_KEMPSTON_UP_IDX);        break;  // kempston joystick:  8
                }
            }
            else                                                                                        // released up
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_release_key (MATRIX_KEY_7_IDX);            break;  // cursor joystick:   "7"
                    case JOYSTICK_SINCLAIR_P1:  zxio_release_key (MATRIX_KEY_9_IDX);            break;  // sinclair joystick: "9"
                    case JOYSTICK_SINCLAIR_P2:  zxio_release_key (MATRIX_KEY_4_IDX);            break;  // sinclair joystick: "4"
                    case JOYSTICK_KEMPSTON:     zxio_release_key (MATRIX_KEMPSTON_UP_IDX);      break;  // kempston joystick:  8
                }
            }
        }

        if (changed_buttons & JOYSTICK_DOWN)                                                            // changed down
        {
            if (buttons & JOYSTICK_DOWN)                                                                // pressed down
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_press_key (MATRIX_KEY_6_IDX);              break;  // cursor joystick:   "6"
                    case JOYSTICK_SINCLAIR_P1:  zxio_press_key (MATRIX_KEY_8_IDX);              break;  // sinclair joystick: "8"
                    case JOYSTICK_SINCLAIR_P2:  zxio_press_key (MATRIX_KEY_3_IDX);              break;  // sinclair joystick: "3"
                    case JOYSTICK_KEMPSTON:     zxio_press_key (MATRIX_KEMPSTON_DOWN_IDX);      break;  // kempston joystick:  4
                }
            }
            else                                                                                        // released down
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_release_key (MATRIX_KEY_6_IDX);            break;  // cursor joystick:   "6"
                    case JOYSTICK_SINCLAIR_P1:  zxio_release_key (MATRIX_KEY_8_IDX);            break;  // sinclair joystick: "8"
                    case JOYSTICK_SINCLAIR_P2:  zxio_release_key (MATRIX_KEY_3_IDX);            break;  // sinclair joystick: "3"
                    case JOYSTICK_KEMPSTON:     zxio_release_key (MATRIX_KEMPSTON_DOWN_IDX);    break;  // kempston joystick:  4
                }
            }
        }

        if (changed_buttons & JOYSTICK_FIRE)                                                            // changed fire
        {
            if (buttons & JOYSTICK_FIRE)                                                                // pressed fire
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_press_key (MATRIX_KEY_0_IDX);              break;  // cursor joystick:   "0"
                    case JOYSTICK_SINCLAIR_P1:  zxio_press_key (MATRIX_KEY_0_IDX);              break;  // sinclair joystick: "0"
                    case JOYSTICK_SINCLAIR_P2:  zxio_press_key (MATRIX_KEY_5_IDX);              break;  // sinclair joystick: "5"
                    case JOYSTICK_KEMPSTON:     zxio_press_key (MATRIX_KEMPSTON_FIRE_IDX);      break;  // kempston joystick: 16
                }
            }
            else                                                                                        // released fire
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:       zxio_release_key (MATRIX_KEY_0_IDX);            break;  // cursor joystick:   "0"
                    case JOYSTICK_SINCLAIR_P1:  zxio_release_key (MATRIX_KEY_0_IDX);            break;  // sinclair joystick: "0"
                    case JOYSTICK_SINCLAIR_P2:  zxio_release_key (MATRIX_KEY_5_IDX);            break;  // sinclair joystick: "5"
                    case JOYSTICK_KEMPSTON:     zxio_release_key (MATRIX_KEMPSTON_FIRE_IDX);    break;  // kempston joystick: 16
                }
            }
        }

        last_buttons = buttons;
    }

}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * joystick_finished_transmission() - check if transmission finished, then decode data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
joystick_finished_transmission (uint_fast8_t do_decode)
{
    uint_fast8_t    rtc = 0;

    if (joystick_state == JOYSTICK_STATE_BUSY)
    {
        if (wii_finished_transmission ())
        {
            if (do_decode)
            {
                joystick_decode_data ();
            }

            joystick_state = JOYSTICK_STATE_IDLE;
            rtc = 1;
        }
    }
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * joystick_start_transmission() - start transmission
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
joystick_start_transmission (void)
{
    uint_fast8_t    rtc = 0;

    if (joystick_state == JOYSTICK_STATE_IDLE)
    {
        if (wii_start_transmission ())
        {
            joystick_state = JOYSTICK_STATE_BUSY;
            rtc = 1;
        }
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize joystick/gamepad
 *
 * Return values:
 *  0   Failed
 *  1   Successful
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
joystick_init (void)
{
    uint_fast8_t    rtc;

    rtc = wii_init ();

    if (rtc)
    {
        joystick_is_online = 1;
    }
    return rtc;
}
