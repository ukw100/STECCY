/*-------------------------------------------------------------------------------------------------------------------------------------------
 * scancodes.h - scancodes of PC keyboard
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
#ifndef SCANCODES_H
#define SCANCODES_H

#define SCANCODE_EXTENDED_FLAG  0x0100
#define SCANCODE_EXTENDED       0xE0
#define SCANCODE_NONE           0x00
#define SCANCODE_REDRAW         0xFF                                                    // pseudo code for refreshing X11 window

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * row 0
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_ESC            0x01
#define SCANCODE_F1             0x3B
#define SCANCODE_F2             0x3C
#define SCANCODE_F3             0x3D
#define SCANCODE_F4             0x3E
#define SCANCODE_F5             0x3F
#define SCANCODE_F6             0x40
#define SCANCODE_F7             0x41
#define SCANCODE_F8             0x42
#define SCANCODE_F9             0x43
#define SCANCODE_F10            0x44
#define SCANCODE_F11            0x57
#define SCANCODE_F12            0x58

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * row #1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_CIRCUMFLEX     0x29
#define SCANCODE_1              0x02
#define SCANCODE_2              0x03
#define SCANCODE_3              0x04
#define SCANCODE_4              0x05
#define SCANCODE_5              0x06
#define SCANCODE_6              0x07
#define SCANCODE_7              0x08
#define SCANCODE_8              0x09
#define SCANCODE_9              0x0A
#define SCANCODE_0              0x0B
#define SCANCODE_SHARP_S        0x0C
#define SCANCODE_ACCENT         0x0D
#define SCANCODE_BACKSPACE      0x0E

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * row #2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_TAB            0x0F
#define SCANCODE_Q              0x10
#define SCANCODE_W              0x11
#define SCANCODE_E              0x12
#define SCANCODE_R              0x13
#define SCANCODE_T              0x14
#define SCANCODE_Z              0x15
#define SCANCODE_U              0x16
#define SCANCODE_I              0x17
#define SCANCODE_O              0x18
#define SCANCODE_P              0x19
#define SCANCODE_PLUS           0x1B
#define SCANCODE_ENTER          0x1C

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * row #3
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_A              0x1E
#define SCANCODE_S              0x1F
#define SCANCODE_D              0x20
#define SCANCODE_F              0x21
#define SCANCODE_G              0x22
#define SCANCODE_H              0x23
#define SCANCODE_J              0x24
#define SCANCODE_K              0x25
#define SCANCODE_L              0x26
#define SCANCODE_HASH           0x2B

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * row #4
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_LSHIFT         0x2A
#define SCANCODE_LESS           0x56
#define SCANCODE_Y              0x2C
#define SCANCODE_X              0x2D
#define SCANCODE_C              0x2E
#define SCANCODE_V              0x2F
#define SCANCODE_B              0x30
#define SCANCODE_N              0x31
#define SCANCODE_M              0x32
#define SCANCODE_COMMA          0x33
#define SCANCODE_DOT            0x34
#define SCANCODE_MINUS          0x35
#define SCANCODE_RSHIFT         0x36
#define SCANCODE_SPACE          0x39

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keypad
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_KEYPAD_PF1     0x45
#define SCANCODE_KEYPAD_PF2     (0x35 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_KEYPAD_PF3     0x37
#define SCANCODE_KEYPAD_PF4     0xaA
#define SCANCODE_KEYPAD_PLUS    0x4E
#define SCANCODE_KEYPAD_ENTER   (0x1C | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_KEYPAD_COMMA   0x53
#define SCANCODE_KEYPAD_7       0x47
#define SCANCODE_KEYPAD_8       0x48
#define SCANCODE_KEYPAD_9       0x49
#define SCANCODE_KEYPAD_4       0x4B
#define SCANCODE_KEYPAD_5       0x4C
#define SCANCODE_KEYPAD_6       0x4D
#define SCANCODE_KEYPAD_1       0x4F
#define SCANCODE_KEYPAD_2       0x50
#define SCANCODE_KEYPAD_3       0x51
#define SCANCODE_KEYPAD_0       0x52

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * misc
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SCANCODE_LCTRL          0x1D
#define SCANCODE_RCTRL          0x61
#define SCANCODE_RCTRL_EXT      (0x1D | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_LALT           0x38
#define SCANCODE_RALT           (0x38 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_LGUI           (0x5B | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_RGUI           (0x5C | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_MENU           (0x5D | SCANCODE_EXTENDED_FLAG)

#define SCANCODE_L_ARROW        0x69
#define SCANCODE_R_ARROW        0x6a
#define SCANCODE_D_ARROW        0x6c
#define SCANCODE_U_ARROW        0x67

#define SCANCODE_L_ARROW_EXT    (0x4B | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_R_ARROW_EXT    (0x4D | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_D_ARROW_EXT    (0x50 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_U_ARROW_EXT    (0x48 | SCANCODE_EXTENDED_FLAG)

#define SCANCODE_PRINT          (0x37 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_SCROLL         0x46
#define SCANCODE_PAUSE          0x7F                                        // sometimes returns 0x45 (KEY_PAD_PF1) !

#define SCANCODE_INSERT         (0x52 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_DELETE         (0x53 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_HOME           (0x47 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_END            (0x4F | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_PAGE_UP        (0x49 | SCANCODE_EXTENDED_FLAG)
#define SCANCODE_PAGE_DOWN      (0x51 | SCANCODE_EXTENDED_FLAG)

#endif // SCANCODES_H
