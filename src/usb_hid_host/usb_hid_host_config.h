/*-------------------------------------------------------------------------------------------------------------------------------------------
 * usb_hid_host_config.h - USB HID HOST configuration
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
#ifndef USB_HID_HOST_CONFIG_H
#define USB_HID_HOST_CONFIG_H

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * enable if you want to use USB HS in FS mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
// #define USE_USB_OTG_HS                                                               // fm: only for STM32F429-Discovery

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * disable ID pin for USB HID HOST library
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define USB_HID_HOST_DISABLE_ID                                                         // fm: Disable ID pin, only Data+ and Data- pins

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * disable VBUS pin for USB HID HOST library
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define USB_HID_HOST_DISABLE_VBUS                                                       // fm: Disable VBUS, only Data+ and Data- pins

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Keyboard support for QWERTY mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define QWERTY_KEYBOARD

#endif // USB_HID_HOST_CONFIG_H
