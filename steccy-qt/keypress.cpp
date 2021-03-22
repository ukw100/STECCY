/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keypress.c - map AT keyboard to ZX keyboard/joystick - handle key events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Cursor Joystick:
 * INKEY$ returns the following values:
 *    "7"
 * "5"   "8"
 *    "6"
 * Fire returns "0"
 *
 * Sinclair Interface 2:
 * INKEY$ returns the following values:
 * Player 1:        Player 2:
 *    "9"              "4"
 * "6"   "7"        "1"   "2"
 *    "8"              "3"
 * Fire: "0"        Fire: "5"
 *
 * Kempston Joystick:
 * IN 31 returns the following values:
 *  10  8  9
 *   2  0  1
 *   6  4  5
 * Pressing either fire buttons adds 16 to these numbers
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2019-2021 Frank Meyer - frank(at)fli4l.de
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

#ifdef QT_CORE_LIB                                                                  // QT version

#include "keypress.h"
#include "mywindow.h"

#include <QApplication>
#include <QKeyEvent>
#include "z80.h"
#include "zxscr.h"
#include "zxio.h"

KeyPress::KeyPress(QWidget * parent) : QWidget(parent)
{
    keypressLabel = new QLabel("Key pressed", parent);
    keypressLabel->setGeometry (200, 2 * ZX_SPECTRUM_DISPLAY_ROWS + 2 * ZX_SPECTRUM_BORDER_SIZE + 30, 100, 20);

    joystickLabel = new QLabel("Joystick", parent);
    joystickLabel->setGeometry (30, 2 * ZX_SPECTRUM_DISPLAY_ROWS + 2 * ZX_SPECTRUM_BORDER_SIZE + 30, 150, 20);

    joystickLabel->setText (jostick_names[joystick_type]);
}

const char *
KeyPress::mapkey (unsigned int keycode)
{
    const char *    text = nullptr;
    int             kidx = 0;

    key[0] = 0xFF;
    key[1] = 0xFF;

    if (shift_state)
    {
        switch (keycode)
        {
            case 2:
            {
                text = "!";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_1_IDX;
                break;
            }
            case 3:
            {
                text = "\"";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_P_IDX;
                break;
            }
            case 5:
            {
                text = "$";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_4_IDX;
                break;
            }
            case 6:
            {
                text = "%";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case 7:
            {
                text = "&";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case 8:
            {
                text = "/";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_V_IDX;
                break;
            }
            case 9:
            {
                text = "(";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case 10:
            {
                text = ")";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_9_IDX;
                break;
            }
            case 11:
            {
                text = "=";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_L_IDX;
                break;
            }
            case 12:
            {
                text = "?";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_C_IDX;
                break;
            }
            case 27:
            {
                text = "*";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_B_IDX;
                break;
            }
            case 43:
            {
                text = "'";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case 53:
            {
                text = "_";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case 52:
            {
                text = ":";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_Z_IDX;
                break;
            }
            case 51:
            {
                text = ";";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_O_IDX;
                break;
            }
            case 86:
            {
                text = ">";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_T_IDX;
                break;
            }
        }
    }

    if (kidx == 0)                                                          // key already found?
    {                                                                       // no...
        switch (keycode)
        {
            case 1:
            {
                text = "Esc";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_SPACE_IDX;
                break;
            }
            case 14:
            {
                text = "BS";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case 331:
            {
                text = "Left";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case 333:
            {
                text = "Right";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case 336:
            {
                text = "Down";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case 328:
            {
                text = "Up";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case 71:                                                                    // Keypad 7: Joystick up + left
            {
                text = "Joy UpLeft";
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
            case 72:                                                                    // Keypad 8: Joystick up
            {
                text = "Joy Up";
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
            case 73:                                                                    // Keypad 9: Joystick up + right
            {
                text = "Joy UpRight";
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
            case 75:                                                                    // Keypad 4: Joystick left
            {
                text = "Joy Left";
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
            case 77:                                                                    // Keypad 6: Joystick right
            {
                text = "Joy Right";
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
            case 79:                                                                    // Keypad 1: Joystick down + left
            {
                text = "Joy DownLeft";
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
            case 80:                                                                    // Keypad 2: Joystick down
            case 76:                                                                    // Keypad 5: Joystick down, too
            {
                text = "Joy Down";
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
            case 81:                                                                    // Keypad 3: Joystick down + right
            {
                text = "Joy DownRight";
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
            case 82:                                                                    // Keypad 0: Joystick fire
            {
                text = "Joy Fire";
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
            case 42:
            {
                text = "LShift";
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                break;
            }
            case 44:
            {
                text = "Z";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Z_IDX;
                break;
            }
            case 45:
            {
                text = "X";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_X_IDX;
                break;
            }
            case 46:
            {
                text = "C";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_C_IDX;
                break;
            }
            case 47:
            {
                text = "V";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_V_IDX;
                break;
            }
            case 30:
            {
                text = "A";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_A_IDX;
                break;
            }
            case 31:
            {
                text = "S";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_S_IDX;
                break;
            }
            case 32:
            {
                text = "D";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_D_IDX;
                break;
            }
            case 33:
            {
                text = "F";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_F_IDX;
                break;
            }
            case 34:
            {
                text = "G";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_G_IDX;
                break;
            }
            case 16:
            {
                text = "Q";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Q_IDX;
                break;
            }
            case 17:
            {
                text = "W";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_W_IDX;
                break;
            }
            case 18:
            {
                text = "E";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_E_IDX;
                break;
            }
            case 19:
            {
                text = "R";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_R_IDX;
                break;
            }
            case 20:
            {
                text = "T";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_T_IDX;
                break;
            }
            case 2:
            {
                text = "1";
                key[kidx++] = MATRIX_KEY_1_IDX;
                break;
            }
            case 3:
            {
                text = "2";
                key[kidx++] = MATRIX_KEY_2_IDX;
                break;
            }
            case 4:
            {
                text = "3";
                key[kidx++] = MATRIX_KEY_3_IDX;
                break;
            }
            case 5:
            {
                text = "4";
                key[kidx++] = MATRIX_KEY_4_IDX;
                break;
            }
            case 6:
            {
                text = "5";
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case 11:
            {
                text = "0";
                key[kidx++] = MATRIX_KEY_0_IDX;
                break;
            }
            case 10:
            {
                text = "9";
                key[kidx++] = MATRIX_KEY_9_IDX;
                break;
            }
            case 9:
            {
                text = "8";
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case 8:
            {
                text = "7";
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
            case 7:
            {
                text = "6";
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case 25:
            {
                text = "P";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_P_IDX;
                break;
            }
            case 24:
            {
                text = "O";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_O_IDX;
                break;
            }
            case 23:
            {
                text = "I";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_I_IDX;
                break;
            }
            case 22:
            {
                text = "U";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_U_IDX;
                break;
            }
            case 21:
            {
                text = "Y";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_Y_IDX;
                break;
            }
            case 28:
            {
                text = "Enter";
                key[kidx++] = MATRIX_KEY_ENTER_IDX;
                break;
            }
            case 38:
            {
                text = "L";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_L_IDX;
                break;
            }
            case 37:
            {
                text = "K";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_K_IDX;
                break;
            }
            case 36:
            {
                text = "J";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_J_IDX;
                break;
            }
            case 35:
            {
                text = "H";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_H_IDX;
                break;
            }
            case 57:
            {
                text = "Space";
                key[kidx++] = MATRIX_KEY_SPACE_IDX;
                break;
            }
            case 29:                                                                // Left Ctrl
            case 285:                                                               // Right Ctrl
            {
                text = "Ctrl";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                break;
            }
            case 50:
            {
                text = "M";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_M_IDX;
                break;
            }
            case 49:
            {
                text = "N";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_N_IDX;
                break;
            }
            case 48:
            {
                text = "B";
                if (shift_state)
                {
                    key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                }
                key[kidx++] = MATRIX_KEY_B_IDX;
                break;
            }
            case 27:
            {
                text = "+";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_K_IDX;
                break;
            }
            case 43:
            {
                text = "#";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_3_IDX;
                break;
            }
            case 53:
            {
                text = "-";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_J_IDX;
                break;
            }
            case 52:
            {
                text = ".";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_M_IDX;
                break;
            }
            case 51:
            {
                text = ",";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_N_IDX;
                break;
            }
            case 86:
            {
                text = "<";
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                key[kidx++] = MATRIX_KEY_R_IDX;
                break;
            }
            case 69:
            {
                text = "Pause";
                z80_pause ();
                break;
            }
        }
    }
    return text;
}

void KeyPress::keyPressEvent(QKeyEvent *event)
{
    char            buf[64];
    const char *    text    = nullptr;
    unsigned int    keycode = event->nativeScanCode();

    if (keycode == 54)
    {
        text = "RShift";
        shift_state = 1;
    }
    else if (keycode == 15)                                                             // TAB: Select Joystick mode
    {
        text = "Tab";
        joystick_type++;

        if (joystick_type == N_JOYSTICKS)
        {
            joystick_type = 0;
        }

        joystickLabel->setText (jostick_names[joystick_type]);
    }
    else
    {
        text = mapkey (keycode);

        if (key[0] != 0xFF)
        {
            zxio_press_key (key[0]);

            if (key[1] != 0xFF)
            {
                zxio_press_key (key[1]);
            }
        }
    }

    if (text)
    {
        sprintf (buf, "Pressed %s %u", text, keycode);
    }
    else
    {
        sprintf (buf, "Pressed %u", keycode);
    }
    keypressLabel->setText(buf);
    // printf ("%s\n", buf); fflush (stdout);
}

void KeyPress::keyReleaseEvent(QKeyEvent *event)
{
    char            buf[64];
    const char *    text    = nullptr;
    unsigned int    keycode = event->nativeScanCode();

    if (keycode == 54)
    {
        text = "RShift";
        zxio_release_key (MATRIX_KEY_SHIFT_IDX);
        shift_state = 0;
    }
    else
    {
        text = mapkey (keycode);

        if (key[0] != 0xFF)
        {
            zxio_release_key (key[0]);

            if (key[1] != 0xFF)
            {
                zxio_release_key (key[1]);
            }
        }
    }

    if (text)
    {
        sprintf (buf, "Released %s %u", text, keycode);
    }
    else
    {
        sprintf (buf, "Released %u", keycode);
    }
    keypressLabel->setText(buf);
    // printf ("%s\n", buf); fflush (stdout);
}

#else // no QT

#include "keypress.h"
#include "ps2key.h"
#include "z80.h"
#include "zxio.h"
#include "zxscr.h"

static uint8_t  key[2];
static uint8_t  rshift_state;                                                       // right SHIFT pressed
uint8_t         joystick_type = JOYSTICK_KEMPSTON;
const char *    joystick_names[N_JOYSTICKS] =
{
    "Cursor Joystick",
    "Sinclair Joystick 1",
    "Sinclair Joystick 2",
    "Kempston Joystick",
};

#define MATRIX_KEY_NONE                     0xFF

#define KEYMAP_FLAG_NONE                    0x00
#define KEYMAP_FLAG_SHIFT                   0x01
#define KEYMAP_FLAG_SYM_SHIFT               0x02

typedef struct
{
    uint8_t     flags;
    uint8_t     matrixkey;
} KEYMAP;

static const KEYMAP keymap[128] =
{
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x00
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x01 F9
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x02
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x03 F5
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x04 F3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x05 F1
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x06 F2
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x07 F12
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x08
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x09 F10
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0A F8
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0B F6
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0C F4
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0D TAB
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x0F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x10
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x11 LALT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_SHIFT_IDX    },                              // 0x12 LSHFT -> Shift
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x13
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_SYM_IDX      },                              // 0x14 LCTRL -> SymShift
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_Q_IDX        },                              // 0x15 Q -> q
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_1_IDX        },                              // 0x16 1 -> 1
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x17
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x18
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x19
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_Z_IDX        },                              // 0x1A Z -> z
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_S_IDX        },                              // 0x1B S -> s
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_A_IDX        },                              // 0x1C A -> a
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_W_IDX        },                              // 0x1D W -> w
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_2_IDX        },                              // 0x1E 2 -> 2
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x1F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x20
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_C_IDX        },                              // 0x21 C -> c
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_X_IDX        },                              // 0x22 X -> x
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_D_IDX        },                              // 0x23 D -> d
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_E_IDX        },                              // 0x24 E -> e
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_4_IDX        },                              // 0x25 4 -> 4
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_3_IDX        },                              // 0x26 3 -> 3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x27
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x28
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_SPACE_IDX    },                              // 0x29 SPACE -> SPACE
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_V_IDX        },                              // 0x2A V -> v
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_F_IDX        },                              // 0x2B F -> f
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_T_IDX        },                              // 0x2C T -> t
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_R_IDX        },                              // 0x2D R -> r
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_5_IDX        },                              // 0x2E 5 -> 5
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x2F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x30
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_N_IDX        },                              // 0x31 N -> n
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_B_IDX        },                              // 0x32 B -> b
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_H_IDX        },                              // 0x33 H -> h
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_G_IDX        },                              // 0x34 G -> g
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_Y_IDX        },                              // 0x35 Y -> y
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_6_IDX        },                              // 0x36 6 -> 6
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x37
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x38
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x39
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_M_IDX        },                              // 0x3A M -> m
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_J_IDX        },                              // 0x3B J -> j
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_U_IDX        },                              // 0x3C U -> u
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_7_IDX        },                              // 0x3D 7 -> 7
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_8_IDX        },                              // 0x3E 8 -> 8
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x3F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x40
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_N_IDX        },                              // 0x41 COMMA -> ,
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_K_IDX        },                              // 0x42 K -> k
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_I_IDX        },                              // 0x43 I -> i
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_O_IDX        },                              // 0x44 O -> o
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_0_IDX        },                              // 0x45 0 -> 0
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_9_IDX        },                              // 0x46 9 -> 9
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x47
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x48
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_M_IDX        },                              // 0x49 DOT -> .
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_J_IDX        },                              // 0x4A MINUS -> -
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_L_IDX        },                              // 0x4B L -> l
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x4C O_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_P_IDX,       },                              // 0x4D P -> p
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x4E SHARP_S
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x4F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x50
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x51
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x52 A_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x53
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x54 U_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x55
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x56
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x57
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x58 CAPS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x59 RSHFT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_ENTER_IDX    },                              // 0x5A ENTER -> ENTER
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_K_IDX        },                              // 0x5B PLUS -> +
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x5C
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_3_IDX,       },                              // 0x5D HASH -> #
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x5E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x5F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x60
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_R_IDX        },                              // 0x61 LESS -> <
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x62
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x63
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x64
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x65
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_0_IDX        },                              // 0x66 BSP -> Shift 0
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x67
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x68
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x69 KP_1
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6A
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6B KP_4
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6C KP_7
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6D
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x6F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x70 KP_0
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x71 KP_COMMA
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x72 KP_2
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x73 KP_5
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x74 KP_6
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x75 KP_8
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_SPACE_IDX    },                              // 0x76 ESC -> Break (Shift Space)
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x77 NUM
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x78 F11
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x79 KP_PLUS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7A KP_3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7B KP_MINUS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7C KP_ASTERISK
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7D KP_9
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7E SCROLL
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // 0x7F
};

static const KEYMAP rshift_keymap[128] =
{
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x00
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x01 F9
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x02
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x03 F5
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x04 F3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x05 F1
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x06 F2
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x07 F12
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x08
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x09 F10
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0A F8
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0B F6
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0C F4
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0D TAB
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x0F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x10
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x11 LALT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_SHIFT_IDX    },                              // RSHFT + 0x12 LSHFT -> Shift
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x13
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_SYM_IDX      },                              // RSHFT + 0x14 LCTRL -> Sym Shift -> E
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_Q_IDX        },                              // RSHFT + 0x15 Q -> Q
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_1_IDX        },                              // RSHFT + 0x16 1 -> !
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x17
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x18
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x19
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_Y_IDX        },                              // RSHFT + 0x1A Z -> Y
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_S_IDX        },                              // RSHFT + 0x1B S -> S
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_A_IDX        },                              // RSHFT + 0x1C A -> A
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_W_IDX        },                              // RSHFT + 0x1D W -> W
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_P_IDX        },                              // RSHFT + 0x1E 2 -> "
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x1F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x20
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_C_IDX        },                              // RSHFT + 0x21 C -> C
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_X_IDX        },                              // RSHFT + 0x22 X -> X
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_D_IDX        },                              // RSHFT + 0x23 D -> D
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_E_IDX        },                              // RSHFT + 0x24 E -> E
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_4_IDX        },                              // RSHFT + 0x25 4 -> $
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_3_IDX        },                              // RSHFT + 0x26 3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x27
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x28
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_SPACE_IDX    },                              // RSHFT + 0x29 SPACE -> Break
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_V_IDX        },                              // RSHFT + 0x2A V -> V
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_F_IDX        },                              // RSHFT + 0x2B F -> F
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_T_IDX        },                              // RSHFT + 0x2C T -> T
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_R_IDX        },                              // RSHFT + 0x2D R -> R
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_5_IDX        },                              // RSHFT + 0x2E 5 -> %
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x2F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x30
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_N_IDX        },                              // RSHFT + 0x31 N -> N
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_B_IDX        },                              // RSHFT + 0x32 B -> B
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_H_IDX        },                              // RSHFT + 0x33 H -> H
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_G_IDX        },                              // RSHFT + 0x34 G -> G
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_Z_IDX        },                              // RSHFT + 0x35 Y -> Z
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_6_IDX        },                              // RSHFT + 0x36 6 -> &
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x37
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x38
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x39
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_M_IDX        },                              // RSHFT + 0x3A M -> M
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_J_IDX        },                              // RSHFT + 0x3B J -> J
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_U_IDX        },                              // RSHFT + 0x3C U -> U
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_V_IDX        },                              // RSHFT + 0x3D 7 -> /
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_8_IDX        },                              // RSHFT + 0x3E 8 -> (
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x3F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x40
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_O_IDX        },                              // RSHFT + 0x41 COMMA -> ;
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_K_IDX        },                              // RSHFT + 0x42 K -> K
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_I_IDX        },                              // RSHFT + 0x43 I -> I
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_O_IDX        },                              // RSHFT + 0x44 O -> O
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_L_IDX        },                              // RSHFT + 0x45 0 -> =
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_9_IDX        },                              // RSHFT + 0x46 9 -> )
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x47
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x48
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_Z_IDX        },                              // RSHFT + 0x49 DOT -> :
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_0_IDX        },                              // RSHFT + 0x4A MINUS -> _
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_L_IDX        },                              // RSHFT + 0x4B L -> L
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x4C O_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_P_IDX,       },                              // RSHFT + 0x4D P -> P
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_C_IDX        },                              // RSHFT + 0x4E SHARP_S -> ?
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x4F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x50
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x51
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x52 A_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x53
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x54 U_UMLAUT
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x55
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x56
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x57
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x58 CAPS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x59 RSHFT (handled below)
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_ENTER_IDX,   },                              // RSHFT + 0x5A ENTER -> Enter
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_B_IDX        },                              // RSHFT + 0x5B PLUS -> *
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x5C
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_7_IDX        },                              // RSHFT + 0x5D HASH -> '
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x5E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x5F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x60
    { KEYMAP_FLAG_SYM_SHIFT,                    MATRIX_KEY_T_IDX        },                              // RSHFT + 0x61 LESS -> >
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x62
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x63
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x64
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x65
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_0_IDX        },                              // RSHFT + 0x66 BSP -> SHIFT 0 -> BSP
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x67
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x68
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x69 KP_1
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6A
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6B KP_4
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6C KP_7
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6D
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6E
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x6F
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x70 KP_0
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x71 KP_COMMA
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x72 KP_2
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x73 KP_5
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x74 KP_6
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x75 KP_8
    { KEYMAP_FLAG_SHIFT,                        MATRIX_KEY_SPACE_IDX    },                              // RSHFT + 0x76 ESC -> Break
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x77 NUM
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x78 F11
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x79 KP_PLUS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7A KP_3
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7B KP_MINUS
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7C KP_ASTERISK
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7D KP_9
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7E SCROLL
    { KEYMAP_FLAG_NONE,                         MATRIX_KEY_NONE         },                              // RSHFT + 0x7F
};

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * mapkey () - map AT key to ZX key
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
mapkey (uint16_t scancode)
{
    int     kidx = 0;

    key[0] = MATRIX_KEY_NONE;
    key[1] = MATRIX_KEY_NONE;

    if (scancode & PS2KEY_EXTENDED_FLAG)                                                        // extended key: check arrow keys and others
    {
        switch (scancode)
        {
            case SCANCODE_RCTRL:
            {
                key[kidx++] = MATRIX_KEY_SYM_IDX;
                break;
            }
            case SCANCODE_L_ARROW:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_5_IDX;
                break;
            }
            case SCANCODE_R_ARROW:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_8_IDX;
                break;
            }
            case SCANCODE_D_ARROW:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_6_IDX;
                break;
            }
            case SCANCODE_U_ARROW:
            {
                key[kidx++] = MATRIX_KEY_SHIFT_IDX;
                key[kidx++] = MATRIX_KEY_7_IDX;
                break;
            }
        }
    }
    else if (scancode < 128)                                                                    // no extended key and not F7 (0x83)
    {
        uint8_t flags;
        uint8_t matrixkey;

        if (rshift_state)
        {
            flags       = rshift_keymap[scancode].flags;
            matrixkey   = rshift_keymap[scancode].matrixkey;
        }
        else
        {
            flags       = keymap[scancode].flags;
            matrixkey   = keymap[scancode].matrixkey;
        }

        if (flags & KEYMAP_FLAG_SHIFT)
        {
            key[kidx++] = MATRIX_KEY_SHIFT_IDX;
        }
        else if (flags & KEYMAP_FLAG_SYM_SHIFT)
        {
            key[kidx++] = MATRIX_KEY_SYM_IDX;
        }

        if (matrixkey != MATRIX_KEY_NONE)
        {
            key[kidx++] = matrixkey;
        }

        if (kidx == 0)                                                                      // key found?
        {                                                                                   // no
            switch (scancode)
            {
                case SCANCODE_KP_7:                                                         // Keypad 7: Joystick up + left
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
                case SCANCODE_KP_8:                                                         // Keypad 8: Joystick up
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
                case SCANCODE_KP_9:                                                         // Keypad 9: Joystick up + right
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
                case SCANCODE_KP_4:                                                         // Keypad 4: Joystick left
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
                case SCANCODE_KP_6:                                                         // Keypad 6: Joystick right
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
                case SCANCODE_KP_1:                                                         // Keypad 1: Joystick down + left
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
                case SCANCODE_KP_2:                                                         // Keypad 2: Joystick down
                case SCANCODE_KP_5:                                                         // Keypad 5: Joystick down, too
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
                case SCANCODE_KP_3:                                                         // Keypad 3: Joystick down + right
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
                case SCANCODE_KP_0:                                                         // Keypad 0: Joystick fire
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
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keypressed () - handle key pressed
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
keypressed (uint16_t scancode)
{
    if (scancode == SCANCODE_RSHFT)
    {
        rshift_state = 1;
    }
    else if (scancode == SCANCODE_TAB)                                              // TAB: stop z80 and handle menu
    {
        ps2key_setmode (PS2KEY_GET_MODE);
        z80_leave_focus ();
    }
    else if (scancode == SCANCODE_F1)
    {
        zxscr_next_display_orientation ();
    }
    else if (scancode == SCANCODE_F2)
    {
        zxscr_next_display_rgb_order ();
    }
    else if (scancode == SCANCODE_F3)
    {
        z80_next_turbo_mode ();
    }
    else
    {
        mapkey (scancode);

        if (key[0] != MATRIX_KEY_NONE)
        {
            zxio_press_key (key[0]);

            if (key[1] != MATRIX_KEY_NONE)
            {
                zxio_press_key (key[1]);
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keyreleased () - handle key released
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
keyreleased (uint16_t scancode)
{
    if (scancode == SCANCODE_RSHFT)
    {
        zxio_release_key (MATRIX_KEY_SHIFT_IDX);
        rshift_state = 0;
    }
    else
    {
        mapkey (scancode);

        if (key[0] != MATRIX_KEY_NONE)
        {
            zxio_release_key (key[0]);

            if (key[1] != MATRIX_KEY_NONE)
            {
                zxio_release_key (key[1]);
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keypress () - handle key
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
keypress (uint16_t scancode)
{
    if (scancode & PS2KEY_RELEASED_FLAG)
    {
        scancode &= PS2KEY_SCANCODE_MASK;
        keyreleased (scancode);
    }
    else
    {
        keypressed (scancode);
    }
}

#endif // QT
