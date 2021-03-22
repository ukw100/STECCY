/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart-driver.h - UART driver routines for STM32F4XX or STM32F10X
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * Example of usage:
 *
 * console.h:
 *     #undef UART_PREFIX
 *     #define UART_PREFIX          console             // prefix for all USART functions, e.g. console_puts()
 *     #include "uart.h"
 *
 * console.c:
 *      #define UART_NUMBER         3                   // UART number on STM32F1xx (1-3 for STM32F1xx, 1-6 for STM32F4xx)
 *      #define UART_ALTERNATE      0                   // ALTERNATE pin number, see below for possible values
 *      #define UART_TXBUFLEN       64                  // ringbuffer size for UART TX
 *      #define UART_RXBUFLEN       64                  // ringbuffer size for UART RX
 *      #include "uart-driver.h"                        // at least include this file
 *
 * Possible UARTs of STM32F10X and STM32F30X:
 *
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F4xx Nucleo:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 6    | PC6  | PC7  || PA11 | PA12 ||      |      |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F407:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  | 4    | PA0  | PA1  || PC10 | PC11 ||      |      |
 *  | 5    | PC12 | PD2  ||      |      ||      |      |
 *  | 6    | PC6  | PC7  || PG14 | PG9  ||      |      |
 *  +--------------------------------------------------+
 *
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2015-2021 Frank Meyer - frank(at)fli4l.de
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (defined STM32F10X)
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

#elif (defined STM32F30X)
#include "stm32f30x.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_usart.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_misc.h"

#elif (defined STM32F4XX)
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

#endif

#include "uart.h"

#define STRBUF_SIZE                 256                                         // (v)printf buffer size

static volatile uint8_t             uart_txbuf[UART_TXBUFLEN];                  // tx ringbuffer
static volatile uint_fast16_t       uart_txsize = 0;                            // tx size
static volatile uint8_t             uart_rxbuf[UART_RXBUFLEN];                  // rx ringbuffer
static volatile uint_fast16_t       uart_rxsize = 0;                            // rx size

static uint_fast8_t                 uart_rxstart = 0;                           // head, not volatile

#define INTERRUPT_CHAR              0x03                                        // CTRL-C
static volatile uint_fast8_t        uart_rawmode = 1;                           // raw mode: no interrupts
static volatile uint_fast8_t        uart_interrupted;                           // flag: user pressed CTRL-C

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Possible UARTs of STM32F4xx:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  | 4    | PA0  | PA1  || PC10 | PC11 ||      |      |
 *  | 5    | PC12 | PD2  ||      |      ||      |      |
 *  | 6    | PC6  | PC7  ||      |      ||      |      |
 *  +--------------------------------------------------+
 *
 * Additional UARTs of STM32F407:
 *
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 6    |      |      || PG14 | PG9  ||      |      |
 *  +--------------------------------------------------+
 *
 * Additional UARTs of STM32F4xx Nucleo:
 *
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 6    |      |      || PA10 | PA11  ||      |      |
 *  +--------------------------------------------------+
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#if defined(STM32F4XX)

#if UART_NUMBER == 1

#if UART_ALTERNATE == 0                                                         // A9/A10
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        9
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        10
#elif UART_ALTERNATE == 1                                                       // B6/B7
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        6
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        7
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#if 0 // fm: STM32F407?
#define UART_GPIO_CLOCK_CMD         RCC_AHB2PeriphClockCmd
#define UART_GPIO                   RCC_AHB2Periph_GPIO
#else
#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO
#endif

#define UART_NAME                   USART1
#define UART_USART_CLOCK_CMD        RCC_APB2PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB2Periph_USART1
#define UART_GPIO_AF_UART           GPIO_AF_USART1
#define UART_IRQ_HANDLER            USART1_IRQHandler
#define UART_IRQ_CHANNEL            USART1_IRQn

#elif UART_NUMBER == 2

#if UART_ALTERNATE == 0                                                         // A2/A3
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        2
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        3
#elif UART_ALTERNATE == 1                                                       // D5/D6
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        5
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        6
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO

#define UART_NAME                   USART2
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART2
#define UART_GPIO_AF_UART           GPIO_AF_USART2
#define UART_IRQ_HANDLER            USART2_IRQHandler
#define UART_IRQ_CHANNEL            USART2_IRQn

#elif UART_NUMBER == 3

#if UART_ALTERNATE == 0                                                         // B10/B11
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 1                                                       // D8/D9
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        8
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        9
#elif UART_ALTERNATE == 2                                                       // C10/C11
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       C
#  define UART_RX_PIN_NUMBER        11
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO

#define UART_NAME                   USART3
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART3
#define UART_GPIO_AF_UART           GPIO_AF_USART3
#define UART_IRQ_HANDLER            USART3_IRQHandler
#define UART_IRQ_CHANNEL            USART3_IRQn

#elif UART_NUMBER == 4

#if UART_ALTERNATE == 0                                                         // A0/A1
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        0
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        1
#elif UART_ALTERNATE == 1                                                       // C10/C11
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       C
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO

#define UART_NAME                   USART4
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART4
#define UART_GPIO_AF_UART           GPIO_AF_USART4
#define UART_IRQ_HANDLER            USART4_IRQHandler
#define UART_IRQ_CHANNEL            USART4_IRQn

#elif UART_NUMBER == 5

#if UART_ALTERNATE == 0                                                         // C12/D2
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        12
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        2
#elif UART_ALTERNATE == 1                                                       // not defined
#  error wrong UART_ALTERNATE value
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO

#define UART_NAME                   USART5
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART5
#define UART_GPIO_AF_UART           GPIO_AF_USART5
#define UART_IRQ_HANDLER            USART5_IRQHandler
#define UART_IRQ_CHANNEL            USART5_IRQn

#elif UART_NUMBER == 6

#if UART_ALTERNATE == 0                                                         // C6/C7
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        6
#  define UART_RX_PORT_LETTER       C
#  define UART_RX_PIN_NUMBER        7
#elif UART_ALTERNATE == 1
#  if defined(STM32F4XX)                                                        // STM32F4xx Nucleo
#    define UART_TX_PORT_LETTER     A                                           // A11/A12
#    define UART_TX_PIN_NUMBER      11
#    define UART_RX_PORT_LETTER     A
#    define UART_RX_PIN_NUMBER      12
#  else                                                                         // STM32F407
#    define UART_TX_PORT_LETTER     G                                           // G14/G9
#    define UART_TX_PIN_NUMBER      14
#    define UART_RX_PORT_LETTER     G
#    define UART_RX_PIN_NUMBER      9
#  endif
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#if 0 // fm: STM32F407?
#define UART_GPIO_CLOCK_CMD         RCC_AHB2PeriphClockCmd
#define UART_GPIO                   RCC_AHB2Periph_GPIO
#else
#define UART_GPIO_CLOCK_CMD         RCC_AHB1PeriphClockCmd
#define UART_GPIO                   RCC_AHB1Periph_GPIO
#endif

#define UART_NAME                   USART6
#define UART_USART_CLOCK_CMD        RCC_APB2PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB2Periph_USART6
#define UART_GPIO_AF_UART           GPIO_AF_USART6
#define UART_IRQ_HANDLER            USART6_IRQHandler
#define UART_IRQ_CHANNEL            USART6_IRQn

#else
#error wrong number for UART_NUMBER, choose 1-6
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Possible UARTs of STM32F30X:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#elif defined(STM32F30X)

#if UART_NUMBER == 1

#if UART_ALTERNATE == 0                                                         // A9/A10
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        9
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        10
#elif UART_ALTERNATE == 1                                                       // B6/B7
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        6
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        7
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHBPeriphClockCmd
#define UART_GPIO                   RCC_AHBPeriph_GPIO
#define UART_NAME                   USART1
#define UART_USART_CLOCK_CMD        RCC_APB2PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB2Periph_USART1
#define UART_GPIO_AF_UART           GPIO_AF_USART1
#define UART_IRQ_HANDLER            USART1_IRQHandler
#define UART_IRQ_CHANNEL            USART1_IRQn

#elif UART_NUMBER == 2

#if UART_ALTERNATE == 0                                                         // A2/A3
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        2
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        3
#elif UART_ALTERNATE == 1                                                       // D5/D6
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        5
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        6
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHBPeriphClockCmd
#define UART_GPIO                   RCC_AHBPeriph_GPIO

#define UART_NAME                   USART2
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART2
#define UART_GPIO_AF_UART           GPIO_AF_USART2
#define UART_IRQ_HANDLER            USART2_IRQHandler
#define UART_IRQ_CHANNEL            USART2_IRQn

#elif UART_NUMBER == 3

#if UART_ALTERNATE == 0                                                         // B10/B11
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 1                                                       // C10/C11
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       C
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 2                                                       // D8/D9
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        8
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        9
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_AHBPeriphClockCmd
#define UART_GPIO                   RCC_AHBPeriph_GPIO

#define UART_NAME                   USART3
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART3
#define UART_GPIO_AF_UART           GPIO_AF_USART3
#define UART_IRQ_HANDLER            USART3_IRQHandler
#define UART_IRQ_CHANNEL            USART3_IRQn

#else
#error wrong number for UART_NUMBER, choose 1-3
#endif

#if UART_NUMBER == 3
#  if UART_ALTERNATE == 0
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_Remap_USART, UART_NUMBER)
#  elif UART_ALTERNATE == 1
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_PartialRemap_USART, UART_NUMBER)
#  elif UART_ALTERNATE == 2
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_FullRemap_USART, UART_NUMBER)
#  endif
#else
#  define UART_GPIO_REMAP           UART_CONCAT(GPIO_Remap_USART, UART_NUMBER)
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * Possible UARTs of STM32F10X:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#elif defined(STM32F10X)

#if UART_NUMBER == 1

#if UART_ALTERNATE == 0                                                         // A9/A10
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        9
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        10
#elif UART_ALTERNATE == 1                                                       // B6/B7
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        6
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        7
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_APB2PeriphClockCmd
#define UART_GPIO                   RCC_APB2Periph_GPIO
#define UART_NAME                   USART1
#define UART_USART_CLOCK_CMD        RCC_APB2PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB2Periph_USART1
#define UART_GPIO_AF_UART           GPIO_AF_USART1
#define UART_IRQ_HANDLER            USART1_IRQHandler
#define UART_IRQ_CHANNEL            USART1_IRQn

#elif UART_NUMBER == 2

#if UART_ALTERNATE == 0                                                         // A2/A3
#  define UART_TX_PORT_LETTER       A
#  define UART_TX_PIN_NUMBER        2
#  define UART_RX_PORT_LETTER       A
#  define UART_RX_PIN_NUMBER        3
#elif UART_ALTERNATE == 1                                                       // D5/D6
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        5
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        6
#elif UART_ALTERNATE == 2                                                       // not defined
#  error wrong UART_ALTERNATE value
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_APB2PeriphClockCmd
#define UART_GPIO                   RCC_APB2Periph_GPIO

#define UART_NAME                   USART2
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART2
#define UART_GPIO_AF_UART           GPIO_AF_USART2
#define UART_IRQ_HANDLER            USART2_IRQHandler
#define UART_IRQ_CHANNEL            USART2_IRQn

#elif UART_NUMBER == 3

#if UART_ALTERNATE == 0                                                         // B10/B11
#  define UART_TX_PORT_LETTER       B
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       B
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 1                                                       // C10/C11
#  define UART_TX_PORT_LETTER       C
#  define UART_TX_PIN_NUMBER        10
#  define UART_RX_PORT_LETTER       C
#  define UART_RX_PIN_NUMBER        11
#elif UART_ALTERNATE == 2                                                       // D8/D9
#  define UART_TX_PORT_LETTER       D
#  define UART_TX_PIN_NUMBER        8
#  define UART_RX_PORT_LETTER       D
#  define UART_RX_PIN_NUMBER        9
#else
#  error wrong UART_ALTERNATE value
#endif

#define UART_GPIO_CLOCK_CMD         RCC_APB2PeriphClockCmd
#define UART_GPIO                   RCC_APB2Periph_GPIO

#define UART_NAME                   USART3
#define UART_USART_CLOCK_CMD        RCC_APB1PeriphClockCmd
#define UART_USART_CLOCK            RCC_APB1Periph_USART3
#define UART_GPIO_AF_UART           GPIO_AF_USART3
#define UART_IRQ_HANDLER            USART3_IRQHandler
#define UART_IRQ_CHANNEL            USART3_IRQn

#else
#error wrong number for UART_NUMBER, choose 1-3
#endif

#if UART_NUMBER == 3
#  if UART_ALTERNATE == 0
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_Remap_USART, UART_NUMBER)
#  elif UART_ALTERNATE == 1
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_PartialRemap_USART, UART_NUMBER)
#  elif UART_ALTERNATE == 2
#    define UART_GPIO_REMAP         UART_CONCAT(GPIO_FullRemap_USART, UART_NUMBER)
#  endif
#else
#  define UART_GPIO_REMAP           UART_CONCAT(GPIO_Remap_USART, UART_NUMBER)
#endif

#endif

#define UART_TX_PORT                UART_CONCAT(GPIO, UART_TX_PORT_LETTER)
#define UART_TX_GPIO_CLOCK          UART_CONCAT(UART_GPIO, UART_TX_PORT_LETTER)
#define UART_TX_PIN                 UART_CONCAT(GPIO_Pin_, UART_TX_PIN_NUMBER)
#define UART_TX_PINSOURCE           UART_CONCAT(GPIO_PinSource,  UART_TX_PIN_NUMBER)
#define UART_RX_PORT                UART_CONCAT(GPIO, UART_RX_PORT_LETTER)
#define UART_RX_GPIO_CLOCK          UART_CONCAT(UART_GPIO, UART_RX_PORT_LETTER)
#define UART_RX_PIN                 UART_CONCAT(GPIO_Pin_, UART_RX_PIN_NUMBER)
#define UART_RX_PINSOURCE           UART_CONCAT(GPIO_PinSource, UART_RX_PIN_NUMBER)

#define UART_PREFIX_INIT            UART_CONCAT(UART_PREFIX, _init)
#define UART_PREFIX_PUTC            UART_CONCAT(UART_PREFIX, _putc)
#define UART_PREFIX_PUTS            UART_CONCAT(UART_PREFIX, _puts)
#define UART_PREFIX_VPRINTF         UART_CONCAT(UART_PREFIX, _vprintf)
#define UART_PREFIX_PRINTF          UART_CONCAT(UART_PREFIX, _printf)
#define UART_PREFIX_GETC            UART_CONCAT(UART_PREFIX, _getc)
#define UART_PREFIX_RAWMODE         UART_CONCAT(UART_PREFIX, _rawmode)
#define UART_PREFIX_INTERRUPTED     UART_CONCAT(UART_PREFIX, _interrupted)
#define UART_PREFIX_POLL            UART_CONCAT(UART_PREFIX, _poll)
#define UART_PREFIX_RXSIZE          UART_CONCAT(UART_PREFIX, _rxsize)
#define UART_PREFIX_FLUSH           UART_CONCAT(UART_PREFIX, _flush)

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_init (uint32_t baudrate)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UART_PREFIX_INIT (uint32_t baudrate)
{
    static uint32_t last_baudrate = 0;

    if (last_baudrate != baudrate)
    {
        last_baudrate = baudrate;

        GPIO_InitTypeDef    gpio;
        USART_InitTypeDef   uart;
        NVIC_InitTypeDef    nvic;

        GPIO_StructInit (&gpio);
        USART_StructInit (&uart);

        UART_GPIO_CLOCK_CMD (UART_TX_GPIO_CLOCK, ENABLE);
        UART_GPIO_CLOCK_CMD (UART_RX_GPIO_CLOCK, ENABLE);

        UART_USART_CLOCK_CMD (UART_USART_CLOCK, ENABLE);

        // connect UART functions with IO-Pins

#if defined (STM32F4XX)

        GPIO_PinAFConfig (UART_TX_PORT, UART_TX_PINSOURCE, UART_GPIO_AF_UART);      // TX
        GPIO_PinAFConfig (UART_RX_PORT, UART_RX_PINSOURCE, UART_GPIO_AF_UART);      // RX

        // UART as alternate function with PushPull
        gpio.GPIO_Mode  = GPIO_Mode_AF;
        gpio.GPIO_Speed = GPIO_Speed_100MHz;
        gpio.GPIO_OType = GPIO_OType_PP;
        gpio.GPIO_PuPd  = GPIO_PuPd_UP;                                                             // fm: perhaps better: GPIO_PuPd_NOPULL

        gpio.GPIO_Pin = UART_TX_PIN;
        GPIO_Init(UART_TX_PORT, &gpio);

        gpio.GPIO_Pin = UART_RX_PIN;
        GPIO_Init(UART_RX_PORT, &gpio);

#elif defined (STM32F10X)

#if UART_ALTERNATE == 0
        // GPIO_PinRemapConfig(UART_GPIO_REMAP, DISABLE);                                           // fm: that's the default?!?
#else
        GPIO_PinRemapConfig(UART_GPIO_REMAP, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
#endif

        /* TX Pin */
        gpio.GPIO_Pin = UART_TX_PIN;
        gpio.GPIO_Mode = GPIO_Mode_AF_PP;
        gpio.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(UART_TX_PORT, &gpio);

        /* RX Pin */
        gpio.GPIO_Pin = UART_RX_PIN;
        gpio.GPIO_Mode = GPIO_Mode_IPU;
        gpio.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(UART_RX_PORT, &gpio);

#endif

        USART_OverSampling8Cmd(UART_NAME, ENABLE);

        // 8 bits, 1 stop bit, no parity, no RTS+CTS
        uart.USART_BaudRate             = baudrate;
        uart.USART_WordLength           = USART_WordLength_8b;
        uart.USART_StopBits             = USART_StopBits_1;
        uart.USART_Parity               = USART_Parity_No;
        uart.USART_HardwareFlowControl  = USART_HardwareFlowControl_None;
        uart.USART_Mode                 = USART_Mode_Rx | USART_Mode_Tx;

        USART_Init(UART_NAME, &uart);

        // UART enable
        USART_Cmd(UART_NAME, ENABLE);

        // RX-Interrupt enable
        USART_ITConfig(UART_NAME, USART_IT_RXNE, ENABLE);

        // enable UART Interrupt-Vector
        nvic.NVIC_IRQChannel                    = UART_IRQ_CHANNEL;
        nvic.NVIC_IRQChannelPreemptionPriority  = 0;
        nvic.NVIC_IRQChannelSubPriority         = 0;
        nvic.NVIC_IRQChannelCmd                 = ENABLE;
        NVIC_Init (&nvic);
    }
    return;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_putc ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UART_PREFIX_PUTC (uint_fast8_t ch)
{
    static uint_fast8_t uart_txstop  = 0;                                       // tail

    while (uart_txsize >= UART_TXBUFLEN)                                        // buffer full?
    {                                                                           // yes
        ;                                                                       // wait
    }

    uart_txbuf[uart_txstop++] = ch;                                             // store character

    if (uart_txstop >= UART_TXBUFLEN)                                           // at end of ringbuffer?
    {                                                                           // yes
        uart_txstop = 0;                                                        // reset to beginning
    }

    __disable_irq();
    uart_txsize++;                                                              // increment used size
    __enable_irq();

    USART_ITConfig(UART_NAME, USART_IT_TXE, ENABLE);                           // enable TXE interrupt
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_puts ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UART_PREFIX_PUTS (const char * s)
{
    uint_fast8_t ch;

    while ((ch = (uint_fast8_t) *s) != '\0')
    {
        UART_PREFIX_PUTC (ch);
        s++;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * uart_vprintf () - print a formatted message (by va_list)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
UART_PREFIX_VPRINTF (const char * fmt, va_list ap)
{
    static char str_buf[STRBUF_SIZE];
    int         len;

    (void) vsnprintf ((char *) str_buf, STRBUF_SIZE, fmt, ap);
    len = strlen (str_buf);
    UART_PREFIX_PUTS (str_buf);
    return len;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * uart_printf () - print a formatted message (varargs)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
UART_PREFIX_PRINTF (const char * fmt, ...)
{
    int     len;
    va_list ap;

    va_start (ap, fmt);
    len = UART_PREFIX_VPRINTF (fmt, ap);
    va_end (ap);
    return len;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_getc ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
UART_PREFIX_GETC (void)
{
    uint_fast8_t         ch;

    while (uart_rxsize == 0)                                                    // rx buffer empty?
    {                                                                           // yes, wait
        ;
    }

    ch = uart_rxbuf[uart_rxstart++];                                            // get character from ringbuffer

    if (uart_rxstart == UART_RXBUFLEN)                                          // at end of rx buffer?
    {                                                                           // yes
        uart_rxstart = 0;                                                       // reset to beginning
    }

    __disable_irq();
    uart_rxsize--;                                                              // decrement size
    __enable_irq();

    return (ch);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_rawmode ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UART_PREFIX_RAWMODE (uint_fast8_t rawmode)
{
    uart_rawmode = rawmode;

    if (rawmode)
    {
        uart_interrupted = 0;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_interrupted ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
UART_PREFIX_INTERRUPTED (void)
{
    uint_fast8_t rtc;

    if (uart_interrupted)
    {
        rtc = 1;
        uart_interrupted = 0;
    }
    else
    {
        rtc = 0;
    }

    return rtc;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_poll()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
UART_PREFIX_POLL (uint_fast8_t * chp)
{
    uint_fast8_t        ch;

    if (uart_rxsize == 0)                                                       // rx buffer empty?
    {                                                                           // yes, return 0
        return 0;
    }

    ch = uart_rxbuf[uart_rxstart++];                                            // get character from ringbuffer

    if (uart_rxstart == UART_RXBUFLEN)                                          // at end of rx buffer?
    {                                                                           // yes
        uart_rxstart = 0;                                                       // reset to beginning
    }

    __disable_irq();
    uart_rxsize--;                                                              // decrement size
    __enable_irq();

    *chp = ch;
    return 1;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_rxsize()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
UART_PREFIX_RXSIZE (void)
{
    return uart_rxsize;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * uart_flush ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
UART_PREFIX_FLUSH (void)
{
    while (uart_txsize > 0)                                                     // tx buffer empty?
    {
        ;                                                                       // no, wait
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * UART_IRQ_HANDLER ()
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void UART_IRQ_HANDLER (void);

void UART_IRQ_HANDLER (void)
{
    static uint_fast8_t     uart_rxstop  = 0;                                   // tail
    uint16_t                value;
    uint_fast8_t            ch;

    if (USART_GetITStatus (UART_NAME, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit (UART_NAME, USART_IT_RXNE);
        value = USART_ReceiveData (UART_NAME);

        ch = value & 0xFF;

        if (! uart_rawmode && ch == INTERRUPT_CHAR)                             // no raw mode & user pressed CTRL-C
        {
            uart_interrupted = 1;
        }

        if (uart_rxsize < UART_RXBUFLEN)                                        // buffer full?
        {                                                                       // no
            uart_rxbuf[uart_rxstop++] = ch;                                     // store character

            if (uart_rxstop >= UART_RXBUFLEN)                                   // at end of ringbuffer?
            {                                                                   // yes
                uart_rxstop = 0;                                                // reset to beginning
            }

            uart_rxsize++;                                                      // increment used size
        }
    }

    if (USART_GetITStatus (UART_NAME, USART_IT_TXE) != RESET)
    {
        static uint_fast8_t  uart_txstart = 0;                                  // head

        USART_ClearITPendingBit (UART_NAME, USART_IT_TXE);

        if (uart_txsize > 0)                                                    // tx buffer empty?
        {                                                                       // no
            ch = uart_txbuf[uart_txstart++];                                    // get character to send, increment offset

            if (uart_txstart == UART_TXBUFLEN)                                  // at end of tx buffer?
            {                                                                   // yes
                uart_txstart = 0;                                               // reset to beginning
            }

            uart_txsize--;                                                      // decrement size

            USART_SendData(UART_NAME, ch);
        }
        else
        {
            USART_ITConfig(UART_NAME, USART_IT_TXE, DISABLE);                   // disable TXE interrupt
        }
    }
    else
    {
        ;
    }
}
