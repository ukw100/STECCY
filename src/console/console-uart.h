/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * console-uart.c - definitions of uart driver routines
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Possible UARTs of STM32F10x:
 *           ALTERNATE=0    ALTERNATE=1    ALTERNATE=2
 *  +--------------------------------------------------+
 *  | UART | TX   | RX   || TX   | RX   || TX   | RX   |
 *  |======|======|======||======|======||======|======|
 *  | 1    | PA9  | PA10 || PB6  | PB7  ||      |      |
 *  | 2    | PA2  | PA3  || PD5  | PD6  ||      |      |
 *  | 3    | PB10 | PB11 || PC10 | PC11 || PD8  | PD9  |
 *  +--------------------------------------------------+
 *
 * Possible UARTs of STM32F401 / STM32F411:
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

#define UART_PREFIX             console                 // see also uart-driver.h

#define UART_NUMBER             2                       // UART number on STM32F407 (1-3 for UART)
#define UART_ALTERNATE          0                       // ALTERNATE number

#define UART_TXBUFLEN           254                     // ringbuffer size for UART TX
#define UART_RXBUFLEN           128                     // ringbuffer size for UART RX

#include "uart-driver.h"
