/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxmapkey.c - map PC keys into ZX Spectrum keys
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020 Frank Meyer - frank(at)fli4l.de
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
#include "zxio.h"
#include "scancodes.h"
#include "lxjoystick.h"
#include "lxmapkey.h"

uint8_t                 lxmapkey_menu_enabled   = 0;
volatile uint32_t       lxmapkey_menu_scancode  = 0x000;

static uint8_t          shift_state             = 0;
static uint8_t          key[2];

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd_enable_menu - enable keyboard for menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxmapkey_enable_menu (void)
{
    lxmapkey_menu_enabled = 1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd_disable_menu - disable keyboard for menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxmapkey_disable_menu (void)
{
    lxmapkey_menu_enabled = 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxmapky() - map PC scancodes into ZX Spectrum matrix codes
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxmapkey (unsigned int scancode)
{
    int             kidx = 0;

    key[0] = 0xFF;
    key[1] = 0xFF;

    if (shift_state)
    {
        switch (scancode)
        {
            case SCANCODE_1:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_1_IDX;
                break;
            }
            case SCANCODE_2:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_P_IDX;
                break;
            }
            case SCANCODE_3:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_3_IDX;
                break;
            }
            case SCANCODE_4:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_4_IDX;
                break;
            }
            case SCANCODE_5:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case SCANCODE_6:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case SCANCODE_7:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_V_IDX;
                break;
            }
            case SCANCODE_8:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case SCANCODE_9:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_9_IDX;
                break;
            }
            case SCANCODE_0:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_L_IDX;
                break;
            }
            case SCANCODE_SHARP_S:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_C_IDX;
                break;
            }
            case SCANCODE_PLUS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_B_IDX;
                break;
            }
            case SCANCODE_HASH:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case SCANCODE_MINUS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case SCANCODE_DOT:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_Z_IDX;
                break;
            }
            case SCANCODE_COMMA:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_O_IDX;
                break;
            }
            case SCANCODE_LESS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_T_IDX;
                break;
            }
        }
    }

    if (kidx == 0)                                                          // key already found?
    {                                                                       // no...
        switch (scancode)
        {
            case SCANCODE_ESC:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_SPACE_IDX;
                break;
            }
            case SCANCODE_BACKSPACE:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case SCANCODE_L_ARROW:
            case SCANCODE_L_ARROW_EXT:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case SCANCODE_R_ARROW:
            case SCANCODE_R_ARROW_EXT:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case SCANCODE_D_ARROW:
            case SCANCODE_D_ARROW_EXT:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case SCANCODE_U_ARROW:
            case SCANCODE_U_ARROW_EXT:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case SCANCODE_KEYPAD_7:                                                     // Keypad 7: Joystick up + left
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_5_IDX;                                 // cursor joystick: "5" + "7"
                        key[kidx++] = MATRIX_KEY_7_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_6_IDX;                                 // sinclair joystick: "6" + "9"
                        key[kidx++] = MATRIX_KEY_9_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_1_IDX;                                 // sinclair joystick: "1" + "4"
                        key[kidx++] = MATRIX_KEY_4_IDX;
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_LEFT_IDX;                         // kempston joystick: 2 + 8
                        key[kidx++] = MATRIX_KEMPSTON_UP_IDX;
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_8:                                                     // Keypad 8: Joystick up
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_7_IDX;                                 // cursor joystick: "7"
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_9_IDX;                                 // sinclair joystick: "9"
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_4_IDX;                                 // sinclair joystick: "4"
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_UP_IDX;                           // kempston joystick: 8
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_9:                                                     // Keypad 9: Joystick up + right
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_8_IDX;                                 // cursor joystick: "8" + "7"
                        key[kidx++] = MATRIX_KEY_7_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_7_IDX;                                 // sinclair joystick: "7" + "9"
                        key[kidx++] = MATRIX_KEY_9_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_2_IDX;                                 // sinclair joystick: "2" + "4"
                        key[kidx++] = MATRIX_KEY_4_IDX;
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_RIGHT_IDX;                        // kempston joystick: 1 + 8
                        key[kidx++] = MATRIX_KEMPSTON_UP_IDX;
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_4:                                                     // Keypad 4: Joystick left
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_5_IDX;                                 // cursor joystick: "5"
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_6_IDX;                                 // sinclair joystick: "6"
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_1_IDX;                                 // sinclair joystick: "1"
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_LEFT_IDX;                         // kempston joystick: 2
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_6:                                                     // Keypad 6: Joystick right
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_8_IDX;                                 // cursor joystick: "8"
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_7_IDX;                                 // sinclair joystick: "7"
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_2_IDX;                                 // sinclair joystick: "2"
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_RIGHT_IDX;                        // kempston joystick: 1
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_1:                                                     // Keypad 1: Joystick down + left
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_5_IDX;                                 // cursor joystick: "5" + "6"
                        key[kidx++] = MATRIX_KEY_6_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_6_IDX;                                 // sinclair joystick: "6" + "8"
                        key[kidx++] = MATRIX_KEY_8_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_1_IDX;                                 // sinclair joystick: "1" + "3"
                        key[kidx++] = MATRIX_KEY_3_IDX;
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_LEFT_IDX;                         // kempston joystick: 2 + 4
                        key[kidx++] = MATRIX_KEMPSTON_DOWN_IDX;
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_2:                                                     // Keypad 2: Joystick down
            case SCANCODE_KEYPAD_5:                                                     // Keypad 5: Joystick down, too
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_6_IDX;                                 // cursor joystick: "6"
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_8_IDX;                                 // sinclair joystick: "8"
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_3_IDX;                                 // sinclair joystick: "3"
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_DOWN_IDX;                         // kempston joystick: 4
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_3:                                                     // Keypad 3: Joystick down + right
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_8_IDX;                                 // cursor joystick: "8" + "6"
                        key[kidx++] = MATRIX_KEY_6_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_7_IDX;                                 // sinclair joystick: "7" + "8"
                        key[kidx++] = MATRIX_KEY_8_IDX;
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_2_IDX;                                 // sinclair joystick: "2" + "3"
                        key[kidx++] = MATRIX_KEY_3_IDX;
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_RIGHT_IDX;                        // kempston joystick: 1 + 4
                        key[kidx++] = MATRIX_KEMPSTON_DOWN_IDX;
                        break;
                }
                break;
            }
            case SCANCODE_KEYPAD_0:                                                     // Keypad 0: Joystick fire
            {
                switch (joystick_type)
                {
                    case JOYSTICK_CURSOR:
                        key[kidx++] = MATRIX_KEY_0_IDX;                                 // cursor joystick: "0"
                        break;
                    case JOYSTICK_SINCLAIR_P1:
                        key[kidx++] = MATRIX_KEY_0_IDX;                                 // sinclair joystick: "0"
                        break;
                    case JOYSTICK_SINCLAIR_P2:
                        key[kidx++] = MATRIX_KEY_5_IDX;                                 // sinclair joystick: "5"
                        break;
                    case JOYSTICK_KEMPSTON:
                        key[kidx++] = MATRIX_KEMPSTON_FIRE_IDX;                         // kempston joystick: 16
                        break;
                }
                break;
            }
            case SCANCODE_LSHIFT:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                break;
            }
            case SCANCODE_Y:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Z_IDX;
                break;
            }
            case SCANCODE_X:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_X_IDX;
                break;
            }
            case SCANCODE_C:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_C_IDX;
                break;
            }
            case SCANCODE_V:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_V_IDX;
                break;
            }
            case SCANCODE_A:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_A_IDX;
                break;
            }
            case SCANCODE_S:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_S_IDX;
                break;
            }
            case SCANCODE_D:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_D_IDX;
                break;
            }
            case SCANCODE_F:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_F_IDX;
                break;
            }
            case SCANCODE_G:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_G_IDX;
                break;
            }
            case SCANCODE_Q:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Q_IDX;
                break;
            }
            case SCANCODE_W:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_W_IDX;
                break;
            }
            case SCANCODE_E:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_E_IDX;
                break;
            }
            case SCANCODE_R:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_R_IDX;
                break;
            }
            case SCANCODE_T:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_T_IDX;
                break;
            }
            case SCANCODE_1:
            {
                key[kidx++] = MATRIX_KEY_1_IDX;
                break;
            }
            case SCANCODE_2:
            {
                key[kidx++] = MATRIX_KEY_2_IDX;
                break;
            }
            case SCANCODE_3:
            {
                key[kidx++] = MATRIX_KEY_3_IDX;
                break;
            }
            case SCANCODE_4:
            {
                key[kidx++] = MATRIX_KEY_4_IDX;
                break;
            }
            case SCANCODE_5:
            {
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case SCANCODE_0:
            {
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case SCANCODE_9:
            {
                key[kidx++] = MATRIX_KEY_9_IDX;
                break;
            }
            case SCANCODE_8:
            {
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case SCANCODE_7:
            {
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case SCANCODE_6:
            {
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case SCANCODE_P:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_P_IDX;
                break;
            }
            case SCANCODE_O:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_O_IDX;
                break;
            }
            case SCANCODE_I:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_I_IDX;
                break;
            }
            case SCANCODE_U:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_U_IDX;
                break;
            }
            case SCANCODE_Z:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Y_IDX;
                break;
            }
            case SCANCODE_ENTER:
            {
                key[kidx++] = MATRIX_KEY_ENTER_IDX;
                break;
            }
            case SCANCODE_L:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_L_IDX;
                break;
            }
            case SCANCODE_K:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_K_IDX;
                break;
            }
            case SCANCODE_J:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_J_IDX;
                break;
            }
            case SCANCODE_H:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_H_IDX;
                break;
            }
            case SCANCODE_SPACE:
            {
                key[kidx++] = MATRIX_KEY_SPACE_IDX;
                break;
            }
            case SCANCODE_LCTRL:                                                    // Left Ctrl
            case SCANCODE_RCTRL:                                                    // Right Ctrl
            case SCANCODE_RCTRL_EXT:                                                // Right Ctrl
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                break;
            }
            case SCANCODE_M:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_M_IDX;
                break;
            }
            case SCANCODE_N:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_N_IDX;
                break;
            }
            case SCANCODE_B:
            {
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_B_IDX;
                break;
            }
            case SCANCODE_PLUS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_K_IDX;
                break;
            }
            case SCANCODE_HASH:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_3_IDX;
                break;
            }
            case SCANCODE_MINUS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_J_IDX;
                break;
            }
            case SCANCODE_DOT:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_M_IDX;
                break;
            }
            case SCANCODE_COMMA:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_N_IDX;
                break;
            }
            case SCANCODE_LESS:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_R_IDX;
                break;
            }
#if 0 // TODO
            case SCANCODE_PAUSE:
            {
                z80_pause ();
                break;
            }
#endif
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkeypress() - handle key press events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxkeypress (unsigned int scancode)
{
    if (scancode == SCANCODE_RSHIFT)
    {
        shift_state = 1;
    }
    else if (scancode == SCANCODE_TAB)                                                  // enter steccy menu
    {
        lxmapkey_enable_menu ();
        z80_leave_focus ();
    }
    else if (scancode == SCANCODE_F3)                                                   // turbo mode
    {
        z80_next_turbo_mode ();
    }
    else
    {
        lxmapkey (scancode);

        if (key[0] != 0xFF)
        {
            zxio_press_key (key[0]);

            if (key[1] != 0xFF)
            {
                zxio_press_key (key[1]);
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkeyrelease() - handle key release events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxkeyrelease (unsigned int scancode)
{
    if (scancode == SCANCODE_RSHIFT)
    {
        zxio_release_key (MATRIX_KEY_SHIFT_IDX);
        shift_state = 0;
    }
    else
    {
        lxmapkey (scancode);

        if (key[0] != 0xFF)
        {
            zxio_release_key (key[0]);

            if (key[1] != 0xFF)
            {
                zxio_release_key (key[1]);
            }
        }
    }
}
