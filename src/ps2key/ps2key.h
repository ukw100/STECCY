/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key.h - handle PS/2 AT keyboard events
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
#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"

#define PS2KEY_CALLBACK_MODE    0
#define PS2KEY_GET_MODE         1

#define PS2KEY_SCANCODE_MASK    0x01FF                                                          // 512 possible key scancodes
#define PS2KEY_EXTENDED_FLAG    0x0100
#define PS2KEY_RELEASED_FLAG    0x0200

// PS/2 scancodes SET 2
#define SCANCODE_NONE           0x00

#define SCANCODE_A              0x1c
#define SCANCODE_B              0x32
#define SCANCODE_C              0x21
#define SCANCODE_D              0x23
#define SCANCODE_E              0x24
#define SCANCODE_F              0x2b
#define SCANCODE_G              0x34
#define SCANCODE_H              0x33
#define SCANCODE_I              0x43
#define SCANCODE_J              0x3b
#define SCANCODE_K              0x42
#define SCANCODE_L              0x4b
#define SCANCODE_M              0x3a
#define SCANCODE_N              0x31
#define SCANCODE_O              0x44
#define SCANCODE_P              0x4d
#define SCANCODE_Q              0x15
#define SCANCODE_R              0x2d
#define SCANCODE_S              0x1b
#define SCANCODE_T              0x2c
#define SCANCODE_U              0x3c
#define SCANCODE_V              0x2a
#define SCANCODE_W              0x1d
#define SCANCODE_X              0x22
#define SCANCODE_Y              0x35
#define SCANCODE_Z              0x1a
#define SCANCODE_0              0x45
#define SCANCODE_1              0x16
#define SCANCODE_2              0x1e
#define SCANCODE_3              0x26
#define SCANCODE_4              0x25
#define SCANCODE_5              0x2e
#define SCANCODE_6              0x36
#define SCANCODE_7              0x3d
#define SCANCODE_8              0x3e
#define SCANCODE_9              0x46
#define SCANCODE_BSP            0x66
#define SCANCODE_SPACE          0x29
#define SCANCODE_TAB            0x0D
#define SCANCODE_CAPS           0x58
#define SCANCODE_LSHFT          0x12
#define SCANCODE_RSHFT          0x59
#define SCANCODE_LCTRL          0x14
#define SCANCODE_RCTRL          (0x14 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_LALT           0x11
#define SCANCODE_RALT           (0x11 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_LWIN           (0x1F | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_RWIN           (0x27 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_MENU           (0x2F | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_ENTER          0x5A
#define SCANCODE_ESC            0x76
#define SCANCODE_F1             0x05
#define SCANCODE_F2             0x06
#define SCANCODE_F3             0x04
#define SCANCODE_F4             0x0C
#define SCANCODE_F5             0x03
#define SCANCODE_F6             0x0B
#define SCANCODE_F7             0x83
#define SCANCODE_F8             0x0A
#define SCANCODE_F9             0x01
#define SCANCODE_F10            0x09
#define SCANCODE_F11            0x78
#define SCANCODE_F12            0x07
#define SCANCODE_SCROLL         0x7E
#define SCANCODE_INSERT         (0x70 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_HOME           (0x6C | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_PG_UP          (0x7D | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_DELETE         (0x71 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_END            (0x69 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_PG_DN          (0x7A | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_U_ARROW        (0x75 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_L_ARROW        (0x6B | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_D_ARROW        (0x72 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_R_ARROW        (0x74 | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_NUM            0x77
#define SCANCODE_KP_SLASH       (0x4A | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_KP_ASTERISK    0x7C
#define SCANCODE_KP_MINUS       0x7B
#define SCANCODE_KP_PLUS        0x79
#define SCANCODE_KP_ENTER       (0x5A | PS2KEY_EXTENDED_FLAG)
#define SCANCODE_KP_COMMA       0x71
#define SCANCODE_KP_0           0x70
#define SCANCODE_KP_1           0x69
#define SCANCODE_KP_2           0x72
#define SCANCODE_KP_3           0x7A
#define SCANCODE_KP_4           0x6B
#define SCANCODE_KP_5           0x73
#define SCANCODE_KP_6           0x74
#define SCANCODE_KP_7           0x6C
#define SCANCODE_KP_8           0x75
#define SCANCODE_KP_9           0x7D

#define SCANCODE_SHARP_S        0x4E
#define SCANCODE_U_UMLAUT       0x54
#define SCANCODE_O_UMLAUT       0x4C
#define SCANCODE_A_UMLAUT       0x52
#define SCANCODE_PLUS           0x5B
#define SCANCODE_HASH           0x5D
#define SCANCODE_MINUS          0x4A
#define SCANCODE_DOT            0x49
#define SCANCODE_COMMA          0x41
#define SCANCODE_LESS           0x61

extern uint_fast16_t            ps2key_scancode;

extern void                     ps2key_init (void);
extern void                     ps2key_setmode (uint_fast8_t);
extern uint_fast16_t            ps2key_getscancode (void);
void                            ps2key_setscancode (uint16_t);
