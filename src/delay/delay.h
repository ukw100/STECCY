/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay.h - declaration of delay functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2014-2021 Frank Meyer - frank(at)fli4l.de
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
#ifndef DELAY_H
#define DELAY_H

#if defined (STM32F10X)
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#elif defined (STM32F4XX)
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#endif

// resolution of delay functions
#define DELAY_RESOLUTION_1_US             1
#define DELAY_RESOLUTION_5_US             5
#define DELAY_RESOLUTION_10_US           10
#define DELAY_RESOLUTION_100_US         100

#define DELAY_DEFAULT_RESOLUTION        DELAY_RESOLUTION_100_US

extern volatile uint32_t                delay_counter;              // counts down in units of resolution

extern void delay_usec (uint32_t);                                  // delay of n usec, only reasonable if resolution is 1us or 5us
extern void delay_msec (uint32_t);                                  // delay of n msec
extern void delay_sec  (uint32_t);                                  // delay of n sec
extern void delay_init (uint_fast8_t);                              // init delay functions

#endif
