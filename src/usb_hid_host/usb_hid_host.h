/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usb_hid_host.h - handle handle USB HID HOST events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020-2021 Frank Meyer - frank(at)fli4l.de
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

#ifndef USB_HID_HOST_H
#define USB_HID_HOST_H

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "usb_hid_host_config.h"

#include "usb_bsp.h"
#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_hid_core.h"

typedef enum
{
    USB_HID_HOST_RESULT_ERROR,                                                      // error occurred
    USB_HID_HOST_RESULT_KEYBOARD_CONNECTED,                                         // keyboard is connected and ready to use
    USB_HID_HOST_RESULT_MOUSE_CONNECTED,                                            // mouse is connected and ready to use
    USB_HID_HOST_RESULT_DISCONNECTED,                                               // device is not connected
    USB_HID_HOST_RESULT_DEVICE_NOT_SUPPORTED,                                       // device is not supported
} USB_HID_HOST_RESULT;

extern USB_HID_HOST_RESULT          usb_hid_host_result;

#if 0 // mice yet not used
typedef struct
{
        uint_fast8_t                event;                                          // flag: usb mouse event occured
        int16_t                     absolute_x;                                     // absolute cursor X position
        int16_t                     absolute_y;                                     // absolute cursor Y position
        int16_t                     diff_x;                                         // difference cursor X position from last check
        int16_t                     diff_y;                                         // difference cursor Y position from last check

        /* Buttons */
        uint_fast8_t                left_button_pressed;                            // indicates if left button is pressed or released
        uint_fast8_t                right_button_pressed;                           // indicates if right button is pressed or released
        uint_fast8_t                middle_button_pressed;                          // indicates if middle button is pressed or released
} USB_HID_HOST_MOUSE;

extern USB_HID_HOST_MOUSE           usb_hid_host_mouse;
#endif

extern void                         usb_hid_host_keyboard_decode (uint8_t *);
extern void                         usb_hid_host_process (uint_fast8_t);
extern void                         usb_hid_host_init (void);

#endif // USB_HID_HOST_H
