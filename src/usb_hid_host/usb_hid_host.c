/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usb_hid_host.c - handle USB HID HOST events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * See also: https://wiki.osdev.org/USB_Human_Interface_Devices
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
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"

#include "board-led.h"
#include "delay.h"
#include "usb_hid_host_config.h"
#include "usb_hid_host.h"
#include "usbh_hid_keybd.h"
#include "usbh_usr.h"
#include "ps2key.h"
#include "keypress.h"

#undef DEBUG
// #define DEBUG                        // change here
#ifdef DEBUG
#define debug_printf(...)               do { printf(__VA_ARGS__); fflush (stdout); } while (0)
#define debug_putchar(c)                do { putchar(c); fflush (stdout); } while (0)
#else
#define debug_printf(...)
#define debug_putchar(c)
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * low level variables
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
extern USB_OTG_CORE_HANDLE              USB_OTG_Core;
extern USBH_HOST                        USB_Host;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * current status
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
USB_HID_HOST_RESULT                     usb_hid_host_result;

#if 0 // mice yet not used
USB_HID_HOST_MOUSE                      usb_hid_host_mouse;

uint_fast8_t
read_usb_mouse (USB_HID_HOST_MOUSE * mouse)
{
    uint_fast8_t rtc = 0;

    if (usb_hid_host_result == USB_HID_HOST_RESULT_MOUSE_CONNECTED)                 // mouse connected?
    {
        if (usb_hid_host_mouse.event)
        {
            mouse->absolute_x               = usb_hid_host_mouse.absolute_x;
            mouse->absolute_y               = usb_hid_host_mouse.absolute_y;
            mouse->diff_x                   = usb_hid_host_mouse.diff_x;
            mouse->diff_y                   = usb_hid_host_mouse.diff_y;
            mouse->left_button_pressed      = usb_hid_host_mouse.left_button_pressed;
            mouse->middle_button_pressed    = usb_hid_host_mouse.middle_button_pressed;
            mouse->right_button_pressed     = usb_hid_host_mouse.right_button_pressed;

            usb_hid_host_mouse.DiffX        = 0;
            usb_hid_host_mouse.DiffY        = 0;
            usb_hid_host_mouse.event        = 0;
            rtc = 1;
        }
    }

    return rtc;
}
#endif // 0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keyboard event queue
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define QUEUE_LENGTH        32

static uint16_t             queue[QUEUE_LENGTH];
static uint_fast8_t         queue_pos = 0;
static uint_fast8_t         queue_end = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * map USB keyboard scancodes into PS/2 scancodes
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_MODIFIER_KEYS   8
static uint16_t usb_to_ps2_modifiers[MAX_MODIFIER_KEYS] =
{
    SCANCODE_LCTRL,                                                             // bit 0: KBD_LEFT_CTRL
    SCANCODE_LSHFT,                                                             // bit 1: KBD_LEFT_SHIFT
    SCANCODE_LALT,                                                              // bit 2: KBD_LEFT_ALT
    SCANCODE_LWIN,                                                              // bit 3: KBD_LEFT_GUI
    SCANCODE_RCTRL,                                                             // bit 4: KBD_RIGHT_CTRL
    SCANCODE_RSHFT,                                                             // bit 5: KBD_RIGHT_SHIFT
    SCANCODE_RALT,                                                              // bit 6: KBD_RIGHT_ALT
    SCANCODE_RWIN,                                                              // bit 7: KBD_RIGHT_GUI
};

static uint16_t usb_to_ps2_code[256] =
{
    SCANCODE_NONE,                                                                          // 0x00 No key pressed
    SCANCODE_NONE,                                                                          // 0x01 Keyboard Error Roll Over
    SCANCODE_NONE,                                                                          // 0x02 Keyboard POST Fail
    SCANCODE_NONE,                                                                          // 0x03 Keyboard Error Undefined

    SCANCODE_A,                                                                             // 0x04 Keyboard a and A
    SCANCODE_B,                                                                             // 0x05 Keyboard b and B
    SCANCODE_C,                                                                             // 0x06 Keyboard c and C
    SCANCODE_D,                                                                             // 0x07 Keyboard d and D
    SCANCODE_E,                                                                             // 0x08 Keyboard e and E
    SCANCODE_F,                                                                             // 0x09 Keyboard f and F
    SCANCODE_G,                                                                             // 0x0a Keyboard g and G
    SCANCODE_H,                                                                             // 0x0b Keyboard h and H
    SCANCODE_I,                                                                             // 0x0c Keyboard i and I
    SCANCODE_J,                                                                             // 0x0d Keyboard j and J
    SCANCODE_K,                                                                             // 0x0e Keyboard k and K
    SCANCODE_L,                                                                             // 0x0f Keyboard l and L
    SCANCODE_M,                                                                             // 0x10 Keyboard m and M
    SCANCODE_N,                                                                             // 0x11 Keyboard n and N
    SCANCODE_O,                                                                             // 0x12 Keyboard o and O
    SCANCODE_P,                                                                             // 0x13 Keyboard p and P
    SCANCODE_Q,                                                                             // 0x14 Keyboard q and Q
    SCANCODE_R,                                                                             // 0x15 Keyboard r and R
    SCANCODE_S,                                                                             // 0x16 Keyboard s and S
    SCANCODE_T,                                                                             // 0x17 Keyboard t and T
    SCANCODE_U,                                                                             // 0x18 Keyboard u and U
    SCANCODE_V,                                                                             // 0x19 Keyboard v and V
    SCANCODE_W,                                                                             // 0x1a Keyboard w and W
    SCANCODE_X,                                                                             // 0x1b Keyboard x and X
    SCANCODE_Y,                                                                             // 0x1c Keyboard y and Y
    SCANCODE_Z,                                                                             // 0x1d Keyboard z and Z

    SCANCODE_1,                                                                             // 0x1e Keyboard 1 and !
    SCANCODE_2,                                                                             // 0x1f Keyboard 2 and @
    SCANCODE_3,                                                                             // 0x20 Keyboard 3 and #
    SCANCODE_4,                                                                             // 0x21 Keyboard 4 and $
    SCANCODE_5,                                                                             // 0x22 Keyboard 5 and %
    SCANCODE_6,                                                                             // 0x23 Keyboard 6 and ^
    SCANCODE_7,                                                                             // 0x24 Keyboard 7 and &
    SCANCODE_8,                                                                             // 0x25 Keyboard 8 and *
    SCANCODE_9,                                                                             // 0x26 Keyboard 9 and (
    SCANCODE_0,                                                                             // 0x27 Keyboard 0 and )

    SCANCODE_ENTER,                                                                         // 0x28 Keyboard Return (ENTER)
    SCANCODE_ESC,                                                                           // 0x29 Keyboard ESCAPE
    SCANCODE_BSP,                                                                           // 0x2a Keyboard DELETE (Backspace)
    SCANCODE_TAB,                                                                           // 0x2b Keyboard Tab
    SCANCODE_SPACE,                                                                         // 0x2c Keyboard Spacebar
    SCANCODE_SHARP_S,                                                                       // 0x2d Keyboard - and _
    SCANCODE_NONE,                                                                          // 0x2e Keyboard = and +
    SCANCODE_NONE,                                                                          // 0x2f Keyboard [ and {

    SCANCODE_PLUS,                                                                          // 0x30 Keyboard ] and }
    SCANCODE_NONE,                                                                          // 0x31 Keyboard \ and |
    SCANCODE_HASH,                                                                          // 0x32 Keyboard Non-US # and ~
    SCANCODE_NONE,                                                                          // 0x33 Keyboard ; and :
    SCANCODE_NONE,                                                                          // 0x34 Keyboard ' and "
    SCANCODE_NONE,                                                                          // 0x35 Keyboard ` and ~
    SCANCODE_COMMA,                                                                         // 0x36 Keyboard , and <
    SCANCODE_DOT,                                                                           // 0x37 Keyboard . and >
    SCANCODE_MINUS,                                                                         // 0x38 Keyboard / and ?
    SCANCODE_NONE,                                                                          // 0x39 Keyboard Caps Lock
    SCANCODE_F1,                                                                            // 0x3a Keyboard F1
    SCANCODE_F2,                                                                            // 0x3b Keyboard F2
    SCANCODE_F3,                                                                            // 0x3c Keyboard F3
    SCANCODE_F4,                                                                            // 0x3d Keyboard F4
    SCANCODE_F5,                                                                            // 0x3e Keyboard F5
    SCANCODE_F6,                                                                            // 0x3f Keyboard F6

    SCANCODE_F7,                                                                            // 0x40 Keyboard F7
    SCANCODE_F8,                                                                            // 0x41 Keyboard F8
    SCANCODE_F9,                                                                            // 0x42 Keyboard F9
    SCANCODE_F10,                                                                           // 0x43 Keyboard F10
    SCANCODE_F11,                                                                           // 0x44 Keyboard F11
    SCANCODE_F12,                                                                           // 0x45 Keyboard F12

    SCANCODE_NONE,                                                                          // 0x46 Keyboard Print Screen
    SCANCODE_SCROLL,                                                                        // 0x47 Keyboard Scroll Lock
    SCANCODE_NONE,                                                                          // 0x48 Keyboard Pause
    SCANCODE_INSERT,                                                                        // 0x49 Keyboard Insert
    SCANCODE_HOME,                                                                          // 0x4a Keyboard Home
    SCANCODE_NONE,                                                                          // 0x4b Keyboard Page Up
    SCANCODE_DELETE,                                                                        // 0x4c Keyboard Delete Forward
    SCANCODE_END,                                                                           // 0x4d Keyboard End
    SCANCODE_NONE,                                                                          // 0x4e Keyboard Page Down
    SCANCODE_R_ARROW,                                                                       // 0x4f Keyboard Right Arrow
    SCANCODE_L_ARROW,                                                                       // 0x50 Keyboard Left Arrow
    SCANCODE_D_ARROW,                                                                       // 0x51 Keyboard Down Arrow
    SCANCODE_U_ARROW,                                                                       // 0x52 Keyboard Up Arrow

    SCANCODE_NONE,                                                                          // 0x53 Keyboard Num Lock and Clear
    SCANCODE_KP_SLASH,                                                                      // 0x54 Keypad /
    SCANCODE_KP_ASTERISK,                                                                   // 0x55 Keypad *
    SCANCODE_KP_MINUS,                                                                      // 0x56 Keypad -
    SCANCODE_KP_PLUS,                                                                       // 0x57 Keypad +
    SCANCODE_KP_ENTER,                                                                      // 0x58 Keypad ENTER
    SCANCODE_KP_1,                                                                          // 0x59 Keypad 1 and End
    SCANCODE_KP_2,                                                                          // 0x5a Keypad 2 and Down Arrow
    SCANCODE_KP_3,                                                                          // 0x5b Keypad 3 and PageDn
    SCANCODE_KP_4,                                                                          // 0x5c Keypad 4 and Left Arrow
    SCANCODE_KP_5,                                                                          // 0x5d Keypad 5
    SCANCODE_KP_6,                                                                          // 0x5e Keypad 6 and Right Arrow
    SCANCODE_KP_7,                                                                          // 0x5f Keypad 7 and Home
    SCANCODE_KP_8,                                                                          // 0x60 Keypad 8 and Up Arrow
    SCANCODE_KP_9,                                                                          // 0x61 Keypad 9 and Page Up
    SCANCODE_KP_0,                                                                          // 0x62 Keypad 0 and Insert
    SCANCODE_KP_COMMA,                                                                      // 0x63 Keypad . and Delete

    SCANCODE_NONE,                                                                          // 0x64 Keyboard Non-US \ and |
    SCANCODE_NONE,                                                                          // 0x65 Keyboard Application
    SCANCODE_NONE,                                                                          // 0x66 Keyboard Power
    SCANCODE_NONE,                                                                          // 0x67 Keypad =

    SCANCODE_NONE,                                                                          // 0x68 Keyboard F13
    SCANCODE_NONE,                                                                          // 0x69 Keyboard F14
    SCANCODE_NONE,                                                                          // 0x6a Keyboard F15
    SCANCODE_NONE,                                                                          // 0x6b Keyboard F16
    SCANCODE_NONE,                                                                          // 0x6c Keyboard F17
    SCANCODE_NONE,                                                                          // 0x6d Keyboard F18
    SCANCODE_NONE,                                                                          // 0x6e Keyboard F19
    SCANCODE_NONE,                                                                          // 0x6f Keyboard F20
    SCANCODE_NONE,                                                                          // 0x70 Keyboard F21
    SCANCODE_NONE,                                                                          // 0x71 Keyboard F22
    SCANCODE_NONE,                                                                          // 0x72 Keyboard F23
    SCANCODE_NONE,                                                                          // 0x73 Keyboard F24

    SCANCODE_NONE,                                                                          // 0x74 Keyboard Execute
    SCANCODE_NONE,                                                                          // 0x75 Keyboard Help
    SCANCODE_NONE,                                                                          // 0x76 Keyboard Menu
    SCANCODE_NONE,                                                                          // 0x77 Keyboard Select
    SCANCODE_NONE,                                                                          // 0x78 Keyboard Stop
    SCANCODE_NONE,                                                                          // 0x79 Keyboard Again
    SCANCODE_NONE,                                                                          // 0x7a Keyboard Undo
    SCANCODE_NONE,                                                                          // 0x7b Keyboard Cut
    SCANCODE_NONE,                                                                          // 0x7c Keyboard Copy
    SCANCODE_NONE,                                                                          // 0x7d Keyboard Paste
    SCANCODE_NONE,                                                                          // 0x7e Keyboard Find
    SCANCODE_NONE,                                                                          // 0x7f Keyboard Mute
    SCANCODE_NONE,                                                                          // 0x80 Keyboard Volume Up
    SCANCODE_NONE,                                                                          // 0x81 Keyboard Volume Down
    SCANCODE_NONE,                                                                          // 0x82 Keyboard Locking Caps Lock
    SCANCODE_NONE,                                                                          // 0x83 Keyboard Locking Num Lock
    SCANCODE_NONE,                                                                          // 0x84 Keyboard Locking Scroll Lock
    SCANCODE_KP_COMMA,                                                                      // 0x85 Keypad Comma
    SCANCODE_NONE,                                                                          // 0x86 Keypad Equal Sign
    SCANCODE_NONE,                                                                          // 0x87 Keyboard International1
    SCANCODE_NONE,                                                                          // 0x88 Keyboard International2
    SCANCODE_NONE,                                                                          // 0x89 Keyboard International3
    SCANCODE_NONE,                                                                          // 0x8a Keyboard International4
    SCANCODE_NONE,                                                                          // 0x8b Keyboard International5
    SCANCODE_NONE,                                                                          // 0x8c Keyboard International6
    SCANCODE_NONE,                                                                          // 0x8d Keyboard International7
    SCANCODE_NONE,                                                                          // 0x8e Keyboard International8
    SCANCODE_NONE,                                                                          // 0x8f Keyboard International9
    SCANCODE_NONE   ,                                                                       // 0x90 Keyboard LANG1
    SCANCODE_NONE,                                                                          // 0x91 Keyboard LANG2
    SCANCODE_NONE,                                                                          // 0x92 Keyboard LANG3
    SCANCODE_NONE,                                                                          // 0x93 Keyboard LANG4
    SCANCODE_NONE,                                                                          // 0x94 Keyboard LANG5
    SCANCODE_NONE,                                                                          // 0x95 Keyboard LANG6
    SCANCODE_NONE,                                                                          // 0x96 Keyboard LANG7
    SCANCODE_NONE,                                                                          // 0x97 Keyboard LANG8
    SCANCODE_NONE,                                                                          // 0x98 Keyboard LANG9
    SCANCODE_NONE,                                                                          // 0x99 Keyboard Alternate Erase
    SCANCODE_NONE,                                                                          // 0x9a Keyboard SysReq/Attention
    SCANCODE_NONE,                                                                          // 0x9b Keyboard Cancel
    SCANCODE_NONE,                                                                          // 0x9c Keyboard Clear
    SCANCODE_NONE,                                                                          // 0x9d Keyboard Prior
    SCANCODE_NONE,                                                                          // 0x9e Keyboard Return
    SCANCODE_NONE,                                                                          // 0x9f Keyboard Separator
    SCANCODE_NONE,                                                                          // 0xa0 Keyboard Out
    SCANCODE_NONE,                                                                          // 0xa1 Keyboard Oper
    SCANCODE_NONE,                                                                          // 0xa2 Keyboard Clear/Again
    SCANCODE_NONE,                                                                          // 0xa3 Keyboard CrSel/Props
    SCANCODE_NONE,                                                                          // 0xa4 Keyboard ExSel

    SCANCODE_NONE,                                                                          // 0xa5 <unkown>
    SCANCODE_NONE,                                                                          // 0xa6 <unkown>
    SCANCODE_NONE,                                                                          // 0xa7 <unkown>
    SCANCODE_NONE,                                                                          // 0xa8 <unkown>
    SCANCODE_NONE,                                                                          // 0xa9 <unkown>
    SCANCODE_NONE,                                                                          // 0xaa <unkown>
    SCANCODE_NONE,                                                                          // 0xab <unkown>
    SCANCODE_NONE,                                                                          // 0xac <unkown>
    SCANCODE_NONE,                                                                          // 0xad <unkown>
    SCANCODE_NONE,                                                                          // 0xae <unkown>
    SCANCODE_NONE,                                                                          // 0xaf <unkown>

    SCANCODE_NONE,                                                                          // 0xb0 Keypad 00
    SCANCODE_NONE,                                                                          // 0xb1 Keypad 000
    SCANCODE_NONE,                                                                          // 0xb2 Thousands Separator
    SCANCODE_NONE,                                                                          // 0xb3 Decimal Separator
    SCANCODE_NONE,                                                                          // 0xb4 Currency Unit
    SCANCODE_NONE,                                                                          // 0xb5 Currency Sub-unit
    SCANCODE_NONE,                                                                          // 0xb6 Keypad (
    SCANCODE_NONE,                                                                          // 0xb7 Keypad )
    SCANCODE_NONE,                                                                          // 0xb8 Keypad {
    SCANCODE_NONE,                                                                          // 0xb9 Keypad }
    SCANCODE_NONE,                                                                          // 0xba Keypad Tab
    SCANCODE_NONE,                                                                          // 0xbb Keypad Backspace
    SCANCODE_NONE,                                                                          // 0xbc Keypad A
    SCANCODE_NONE,                                                                          // 0xbd Keypad B
    SCANCODE_NONE,                                                                          // 0xbe Keypad C
    SCANCODE_NONE,                                                                          // 0xbf Keypad D
    SCANCODE_NONE,                                                                          // 0xc0 Keypad E
    SCANCODE_NONE,                                                                          // 0xc1 Keypad F
    SCANCODE_NONE,                                                                          // 0xc2 Keypad XOR
    SCANCODE_NONE,                                                                          // 0xc3 Keypad ^
    SCANCODE_NONE,                                                                          // 0xc4 Keypad %
    SCANCODE_NONE,                                                                          // 0xc5 Keypad <
    SCANCODE_NONE,                                                                          // 0xc6 Keypad >
    SCANCODE_NONE,                                                                          // 0xc7 Keypad &
    SCANCODE_NONE,                                                                          // 0xc8 Keypad &&
    SCANCODE_NONE,                                                                          // 0xc9 Keypad |
    SCANCODE_NONE,                                                                          // 0xca Keypad ||
    SCANCODE_NONE,                                                                          // 0xcb Keypad :
    SCANCODE_NONE,                                                                          // 0xcc Keypad #
    SCANCODE_NONE,                                                                          // 0xcd Keypad Space
    SCANCODE_NONE,                                                                          // 0xce Keypad @
    SCANCODE_NONE,                                                                          // 0xcf Keypad !
    SCANCODE_NONE,                                                                          // 0xd0 Keypad Memory Store
    SCANCODE_NONE,                                                                          // 0xd1 Keypad Memory Recall
    SCANCODE_NONE,                                                                          // 0xd2 Keypad Memory Clear
    SCANCODE_NONE,                                                                          // 0xd3 Keypad Memory Add
    SCANCODE_NONE,                                                                          // 0xd4 Keypad Memory Subtract
    SCANCODE_NONE,                                                                          // 0xd5 Keypad Memory Multiply
    SCANCODE_NONE,                                                                          // 0xd6 Keypad Memory Divide
    SCANCODE_NONE,                                                                          // 0xd7 Keypad +/-
    SCANCODE_NONE,                                                                          // 0xd8 Keypad Clear
    SCANCODE_NONE,                                                                          // 0xd9 Keypad Clear Entry
    SCANCODE_NONE,                                                                          // 0xda Keypad Binary
    SCANCODE_NONE,                                                                          // 0xdb Keypad Octal
    SCANCODE_NONE,                                                                          // 0xdc Keypad Decimal
    SCANCODE_NONE,                                                                          // 0xdd Keypad Hexadecimal

    SCANCODE_NONE,                                                                          // 0xde <undefined>
    SCANCODE_NONE,                                                                          // 0xdf <undefined>

    SCANCODE_NONE,                                                                          // 0xe0 <undefined>
    SCANCODE_NONE,                                                                          // 0xe1 <undefined>
    SCANCODE_NONE,                                                                          // 0xe2 <undefined>
    SCANCODE_NONE,                                                                          // 0xe3 <undefined>
    SCANCODE_NONE,                                                                          // 0xe4 <undefined>
    SCANCODE_NONE,                                                                          // 0xe5 <undefined>
    SCANCODE_NONE,                                                                          // 0xe6 <undefined>
    SCANCODE_NONE,                                                                          // 0xe7 <undefined>
    SCANCODE_NONE,                                                                          // 0xe8 <undefined>
    SCANCODE_NONE,                                                                          // 0xe9 <undefined>
    SCANCODE_NONE,                                                                          // 0xea <undefined>
    SCANCODE_NONE,                                                                          // 0xeb <undefined>
    SCANCODE_NONE,                                                                          // 0xec <undefined>
    SCANCODE_NONE,                                                                          // 0xed <undefined>
    SCANCODE_NONE,                                                                          // 0xee <undefined>
    SCANCODE_NONE,                                                                          // 0xef <undefined>

    SCANCODE_NONE,                                                                          // 0xf0 <undefined>
    SCANCODE_NONE,                                                                          // 0xf1 <undefined>
    SCANCODE_NONE,                                                                          // 0xf2 <undefined>
    SCANCODE_NONE,                                                                          // 0xf3 <undefined>
    SCANCODE_NONE,                                                                          // 0xf4 <undefined>
    SCANCODE_NONE,                                                                          // 0xf5 <undefined>
    SCANCODE_NONE,                                                                          // 0xf6 <undefined>
    SCANCODE_NONE,                                                                          // 0xf7 <undefined>
    SCANCODE_NONE,                                                                          // 0xf8 <undefined>
    SCANCODE_NONE,                                                                          // 0xf9 <undefined>
    SCANCODE_NONE,                                                                          // 0xfa <undefined>
    SCANCODE_NONE,                                                                          // 0xfb <undefined>
    SCANCODE_NONE,                                                                          // 0xfc <undefined>
    SCANCODE_NONE,                                                                          // 0xfd <undefined>
    SCANCODE_NONE,                                                                          // 0xfe <undefined>
    SCANCODE_NONE,                                                                          // 0xff <undefined>
};

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * dequeue () - fetch scancode from event queue
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
dequeue (uint16_t * code)
{
    uint_fast8_t rtc = 0;

    if (queue_pos != queue_end)
    {
        *code = queue[queue_pos];
        queue_pos++;

        if (queue_pos == QUEUE_LENGTH)
        {
            queue_pos = 0;
        }
        rtc = 1;
    }
    else
    {
        rtc = 0;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * enqueue () - place scancode in event queue
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
enqueue (uint_fast8_t code)
{
    queue[queue_end] = code;
    queue_end++;

    if (queue_end == QUEUE_LENGTH)
    {
        queue_end = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usb_hid_host_keyboard_decode () - decode USB keyboard buffer and convert it into press/release events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
usb_hid_host_keyboard_decode (uint8_t * pbuf)
{
    static uint8_t          last_modifier_keys;
    static uint8_t          last_keys_pressed[KBR_MAX_NBR_PRESSED];
    static uint_fast8_t     last_pbuf_len = 0;
    uint_fast8_t            mask;
    uint_fast8_t            i;
    uint_fast8_t            j;

    debug_printf ("pbuf[0] = 0x%02X ", pbuf[0]);

    for (i = 0, mask = 0x01; i < MAX_MODIFIER_KEYS; i++)
    {
        if (pbuf[0] & mask)
        {
            if (! (last_modifier_keys & mask))
            {
                enqueue (usb_to_ps2_modifiers[i]);
            }
        }
        else
        {
            if (last_modifier_keys & mask)
            {
                enqueue (usb_to_ps2_modifiers[i] | PS2KEY_RELEASED_FLAG);
            }
        }
        mask <<= 1;
    }

    last_modifier_keys = pbuf[0];
    pbuf += 2;

    for (i = 0; i < KBR_MAX_NBR_PRESSED && pbuf[i] != 0x00; i++)
    {
        debug_printf ("pbuf[%d] = 0x%02X ", i + 2, pbuf[i]);

        for (j = 0; j < last_pbuf_len; j++)
        {
            if (pbuf[i] == last_keys_pressed[j])
            {
                last_keys_pressed[j] = 0x00;                                            // mark as processed
                break;
            }
        }
        debug_putchar ('\n');

        if (j == last_pbuf_len)
        {
            if  (usb_to_ps2_code[pbuf[i]])
            {
                debug_printf ("enqueue  press 0x%02X -> 0x%04X\n", pbuf[i], usb_to_ps2_code[pbuf[i]]);
                enqueue (usb_to_ps2_code[pbuf[i]]);
            }
            else
            {
                debug_printf ("ignore   press 0x%02X\n", pbuf[i]);
            }
        }
    }

    if (pbuf[0] == 0x00)
    {
        debug_putchar ('\n');
    }

    for (j = 0; j < last_pbuf_len; j++)
    {
        if (usb_to_ps2_code[last_keys_pressed[j]])
        {
            debug_printf ("enqueue release 0x%02X -> %04X\n", last_keys_pressed[j], usb_to_ps2_code[last_keys_pressed[j]] | PS2KEY_RELEASED_FLAG);
            enqueue (usb_to_ps2_code[last_keys_pressed[j]] | PS2KEY_RELEASED_FLAG);
        }
        else
        {
            debug_printf ("ignore  release 0x%02X\n", last_keys_pressed[j]);
        }
    }

    last_pbuf_len = i;

    memcpy (last_keys_pressed, pbuf, last_pbuf_len);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usb_hid_host_set_keyboard_leds () - set keyboard LEDs
 *
 *  D0:     NUM lock
 *  D1:     CAPS lock
 *  D2:     SCROLL lock
 *  D3:     Compose
 *  D4:     Kana
 *  D5-D7:  reserved
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if 0
static uint_fast8_t
usb_hid_host_set_keyboard_leds (uint_fast8_t ledmask)
{
    uint8_t         buffer[1];
    uint8_t         report_type;
    uint8_t         report_id;
    uint8_t         report_len;
    uint_fast8_t    cnt = 0;
    USBH_Status     res;
    uint_fast8_t    rtc;

    buffer[0]   = ledmask;
    report_type = 0x02;
    report_id   = 0x00;
    report_len  = 1;

    do
    {
        res = USBH_Set_Report (&USB_OTG_Core, &USB_Host, report_type, report_id, report_len, buffer);
        debug_printf ("cnt = %d: res = %d\n", cnt, res);
        cnt++;
    } while (cnt < 16 && res == USBH_BUSY);

    if (res == USBH_OK)
    {
        rtc = 1;
    }
    else
    {
        rtc = 0;
    }

    return rtc;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usbkey_process () - process USB
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
usb_hid_host_process (uint_fast8_t call_ps2key_setscancode)
{
    static uint_fast8_t board_led2_is_on = 0;
    uint16_t            scancode;

    USBH_Process (&USB_OTG_Core, &USB_Host);

    if (usb_hid_host_result == USB_HID_HOST_RESULT_KEYBOARD_CONNECTED)              // keyboard connected?
    {
        if (! board_led2_is_on)
        {
#if 0
            usb_hid_host_set_keyboard_leds (0x01);
#endif
            debug_printf ("connected: usb_hid_host_result=%d\n", usb_hid_host_result);
            board_led2_on ();
            board_led2_is_on = 1;
        }

        while (dequeue (&scancode))
        {
            if (call_ps2key_setscancode)
            {
                ps2key_setscancode (scancode);
            }
            else
            {
                keypress (scancode);
            }
        }
    }
    else
    {
        static USB_HID_HOST_RESULT last_usb_hid_host_result = 0xFF;

        if (last_usb_hid_host_result != usb_hid_host_result)
        {
            debug_printf ("not connected: usb_hid_host_result=%d\n", usb_hid_host_result);
            last_usb_hid_host_result = usb_hid_host_result;
        }

        if (board_led2_is_on)
        {
            board_led2_off ();
            board_led2_is_on = 0;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usbkey_init () - initialize USB
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
usb_hid_host_init ()
{
    uint_fast16_t i;
    uint_fast16_t tries;

    for (tries = 0; tries < 10; tries++)
    {
#ifdef USE_USB_OTG_FS
        USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &HID_cb, &USR_Callbacks);
        delay_msec (50);
#else
#error: fm: USE_USB_OTG_FS not defined!
        USBH_Init(&USB_OTG_Core, USB_OTG_HS_CORE_ID, &USB_Host, &HID_cb, &USR_Callbacks);
#endif

        for (i = 0; i < 148; i++)                       // should work at least at i = 132
        {
            USBH_Process (&USB_OTG_Core, &USB_Host);
            debug_printf ("tries = %d i = %d usb_hid_host_result=%d\r\n", tries, i, usb_hid_host_result);

            if (usb_hid_host_result == USB_HID_HOST_RESULT_KEYBOARD_CONNECTED)
            {
                break;
            }
            delay_msec (5);
        }

        if (usb_hid_host_result == USB_HID_HOST_RESULT_KEYBOARD_CONNECTED)
        {
            break;
        }
    }
}
