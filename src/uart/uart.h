/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart.h - declaration of UART driver routines for STM32F4XX or STM32F10X
 *---------------------------------------------------------------------------------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#if ! defined(STM32F10X)
#if defined(STM32L1XX_MD) || defined(STM32L1XX_MDP) || defined(STM32L1XX_HD)
#define STM32L1XX
#elif defined(STM32F10X_LD) || defined(STM32F10X_LD_VL) \
   || defined(STM32F10X_MD) || defined(STM32F10X_MD_VL) \
   || defined(STM32F10X_HD) || defined(STM32F10X_HD_VL) \
   || defined(STM32F10X_XL) || defined(STM32F10X_CL)
#define STM32F10X
#endif
#endif

#if defined (STM32F10X)
#  include "stm32f10x.h"
#  include "stm32f10x_gpio.h"
#  include "stm32f10x_usart.h"
#  include "stm32f10x_rcc.h"
#  include "misc.h"
#elif defined (STM32F30X)
#  include "stm32f30x.h"
#  include "stm32f30x_gpio.h"
#  include "stm32f30x_usart.h"
#  include "stm32f30x_rcc.h"
#  include "stm32f30x_tim.h"
#  include "stm32f30x_misc.h"
#elif defined (STM32F4XX)
#  include "stm32f4xx.h"
#  include "stm32f4xx_gpio.h"
#  include "stm32f4xx_usart.h"
#  include "stm32f4xx_rcc.h"
#  include "misc.h"
#else
#error unknown STM32
#endif

#include <stdarg.h>

#define _UART_CONCAT(a,b)                a##b
#define UART_CONCAT(a,b)                 _UART_CONCAT(a,b)

extern void             UART_CONCAT(UART_PREFIX, _init)            (uint32_t);
extern void             UART_CONCAT(UART_PREFIX, _putc)            (uint_fast8_t);
extern void             UART_CONCAT(UART_PREFIX, _puts)            (const char *);
extern int              UART_CONCAT(UART_PREFIX, _vprintf)         (const char *, va_list);
extern int              UART_CONCAT(UART_PREFIX, _printf)          (const char *, ...);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _getc)            (void);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _poll)            (uint_fast8_t *);
extern uint_fast8_t     UART_CONCAT(UART_PREFIX, _interrupted)     (void);
extern void             UART_CONCAT(UART_PREFIX, _rawmode)         (uint_fast8_t);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _rxsize)          (void);
extern void             UART_CONCAT(UART_PREFIX, _flush)           (void);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _read)            (char *, uint_fast16_t);
extern uint_fast16_t    UART_CONCAT(UART_PREFIX, _write)           (char *, uint_fast16_t);

