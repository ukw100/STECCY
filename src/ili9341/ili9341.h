/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341.h - definitions/declarations of ILI9341 routines
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
#ifndef ILI9341_H
#define ILI9341_H

#ifdef unix
#include <stdio.h>
#endif

#include <stdint.h>
#include "ili9341-config.h"

#ifdef STM32F10X
#  include "stm32f10x.h"
#  include "stm32f10x_gpio.h"
#  include "stm32f10x_rcc.h"
#elif defined STM32F4XX
#  include "stm32f4xx.h"
#  include "stm32f4xx_gpio.h"
#  include "stm32f4xx_rcc.h"
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ILI9341 API:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define TFT_REG                                 (*((volatile uint16_t *) 0x60000000))
#define TFT_RAM                                 (*((volatile uint16_t *) 0x60080000))   // fm: 0x60020000 ?

#define ili9341_write_command(cmd)              do { TFT_REG = (cmd); } while (0)
#define ili9341_write_data(val)                 do { TFT_RAM = (val); } while (0)
#define ili9341_read_data()                     (TFT_RAM)

#define ILI9341_GLOBAL_FLAGS_RGB_ORDER          0x01
#define ILI9341_GLOBAL_FLAGS_FLIP_HORIZONTAL    0x02
#define ILI9341_GLOBAL_FLAGS_FLIP_VERTICAL      0x04
#define ILI9341_GLOBAL_FLAGS_ROW_COL_EXCHANGE   0x08

#define ILI9341_GLOBAL_FLAGS_MASK               0x0F

typedef struct
{
    uint_fast8_t         flags;
} ILI9341_GLOBALS;

extern ILI9341_GLOBALS  ili9341;

extern void             ili9341_soft_reset (void);
extern void             ili9341_set_flags(uint_fast8_t);
extern void             ili9341_set_column_address (uint_fast16_t, uint_fast16_t);
extern void             ili9341_set_page_address (uint_fast16_t, uint_fast16_t);
extern void             ili9341_write_memory_start (void);
extern void             ili9341_read_memory_start (void);
extern void             ili9341_init (void);

#endif // ILI9341_H
