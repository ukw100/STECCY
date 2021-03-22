/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxram.c - ZX Spectrum emulator RAM functions
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "z80.h"
#include "zxram.h"
#include "zxscr.h"

#if defined STM32F4XX
#include "tft.h"
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * Debugging Macros
 *------------------------------------------------------------------------------------------------------------------------
*/
#ifdef DEBUG
#define debug_printf(...)   do { printf(__VA_ARGS__); fflush (stdout); } while (0)
#define debug_putchar(c)    do { putchar(c); fflush (stdout); } while (0)
#else
#define debug_printf(...)
#define debug_putchar(c)
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * ZX-Spectrum 128K: 32K ROM + 128K RAM
 *
 * ZX Spectrum 128K Memory layout: https://worldofspectrum.org/faq/reference/128kreference.htm
 *
 *      0xffff +--------+--------+--------+--------+--------+--------+--------+--------+
 *             | Bank 0 | Bank 1 | Bank 2 | Bank 3 | Bank 4 | Bank 5 | Bank 6 | Bank 7 |
 *             |        |        |(also at|        |        |(also at|        |        |
 *             |        |        | 0x8000)|        |        | 0x4000)|        |        |
 *             |        |        |        |        |        | screen |        | screen |
 *      0xc000 +--------+--------+--------+--------+--------+--------+--------+--------+
 *             | Bank 2 |        Any one of these pages may be switched in.
 *             |        |
 *             |        |
 *             |        |
 *      0x8000 +--------+
 *             | Bank 5 |
 *             |        |
 *             |        |
 *             | screen |
 *      0x4000 +--------+--------+
 *             | ROM 0  | ROM 1  | Either ROM may be switched in.
 *             |        |        |
 *             |        |        |
 *             |        |        |
 *      0x0000 +--------+--------+
 *
 * STM32F407VE memory layout:
 *
 *      RAM     128K
 *      CCRAM    64K
 *      ------------
 *      Total   192K
 *
 * Layout sorted by CCRAM, RAM:
 *
 * CCRAM is used by the default Spectrum 48K pages:
 *
 *      CCRAM       64K
 *      ---------------
 *      0 ROM0      16K
 *      1 Bank5     16K
 *      2 Bank2     16K
 *      3 Bank0     16K
 *
 * RAM is used by the other pages used by Spectrum 128K:
 *
 *      RAM         96K
 *      ---------------
 *      0 ROM1      16K
 *      1 Bank1     16K
 *      2 Bank3     16K
 *      3 Bank4     16K
 *      4 Bank6     16K
 *      5 Bank7     16K
 *
 * Layout sorted by ROM + RAM Banks
 *
 *      ROM Banks           32K
 *      -----------------------
 *      CCRAM   0   ROM0    16K
 *      RAM     0   ROM1    16K
 *
 *      RAM Banks           32K
 *      -----------------------
 *      CCRAM   3   Bank0   16K
 *      RAM     1   Bank1   16K
 *      CCRAM   2   Bank2   16K
 *      RAM     2   Bank3   16K
 *      RAM     3   Bank4   16K
 *      CCRAM   1   Bank5   16K
 *      RAM     4   Bank6   16K
 *      RAM     5   Bank7   16K
 *
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef STM32F4XX
#define CCRAM           __attribute__((section(".ccram")))
#else
#define CCRAM
#endif

static uint8_t          steccy_ccram[STECCY_CCRAMSIZE] CCRAM;           // 64K
static uint8_t          steccy_ram[STECCY_RAMSIZE];                     // 96K

uint8_t *               steccy_rombankptr[2] =                          // ptr to 2 ROM banks
{
    steccy_ccram + 0 * STECCY_PAGE_SIZE,
    steccy_ram   + 0 * STECCY_PAGE_SIZE
};

uint8_t *               steccy_rambankptr[8] =                          // ptr to 8 RAM banks
{
    steccy_ccram + 3 * STECCY_PAGE_SIZE,
    steccy_ram   + 1 * STECCY_PAGE_SIZE,
    steccy_ccram + 2 * STECCY_PAGE_SIZE,
    steccy_ram   + 2 * STECCY_PAGE_SIZE,
    steccy_ram   + 3 * STECCY_PAGE_SIZE,
    steccy_ccram + 1 * STECCY_PAGE_SIZE,
    steccy_ram   + 4 * STECCY_PAGE_SIZE,
    steccy_ram   + 5 * STECCY_PAGE_SIZE
};

uint8_t *               steccy_bankptr[4];                              // 4 banks active

uint_fast8_t            zx_ram_shadow_display = 0;
uint_fast8_t            zx_ram_memory_paging_disabled;

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_get_8 () - get 8 bit data from RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef DEBUG
uint8_t
zx_ram_get_8 (uint16_t addr)
{
    uint8_t val;

    val = *(steccy_bankptr[addr >> 14] + (addr & 0x3FFF));

    debug_printf (" ; %04X[%02X]", addr, val);
    return val;
}
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_set_8 () - store 8 bit value into RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#if defined QT_CORE_LIB || defined DEBUG
void
zx_ram_set_8 (uint16_t addr, uint8_t value)
{
    debug_printf (" ; %04X[%02X] <- %02X", addr, ram[addr], value);

    if (addr >= ZX_RAM_BEGIN)
    {
#ifdef QT_CORE_LIB
        if (addr < ZX_SPECTRUM_ATTRIBUTES_START_ADDR)
        {
            while (video_ram_blocked_at == addr)
            {
                // wait
            }
            video_ram_changed = 1;
        }
        else if (addr < ZX_SPECTRUM_NON_VIDEO_RAM_START_ADDR)
        {
            while (attr_ram_blocked)
            {
                // wait
            }
            video_ram_changed = 1;
        }
#else
        if (addr < ZX_SPECTRUM_NON_VIDEO_RAM_START_ADDR)
        {
            video_ram_changed = 1;
        }
#endif

        uint8_t * ptr = steccy_bankptr[addr >> 14] + (addr & 0x3FFF);
        *ptr = value;
    }
#ifdef DEBUG
    else
    {
        static uint32_t called;

        if (! called)
        {
            if (reg_PC >= ZX_RAM_BEGIN || addr > 4)
            {                       // Sinclair Basic SKIP-CONS Routine writes 4 dummy bytes into ram at locations 0-4
                debug_printf ("\r\nWriting into ROM, pc = %04X addr = %04Xh, value = %02Xh\n", cur_PC, addr, value);
                called = 1;
            }
        }
    }
#endif
}
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_init () - init memory bank pointers
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zx_ram_init (uint_fast16_t romsize)
{
    steccy_bankptr[0]       = steccy_rombankptr[0];
    steccy_bankptr[1]       = steccy_rambankptr[5];
    steccy_bankptr[2]       = steccy_rambankptr[2];
    steccy_bankptr[3]       = steccy_rambankptr[0];

    zx_ram_shadow_display   = 0;

    if (romsize == 0x4000)
    {
        zx_ram_memory_paging_disabled   = 1;
    }
    else
    {
        zx_ram_memory_paging_disabled   = 0;
    }

    debug_printf ("zx_ram_init: zx_ram_memory_paging_disabled = %d\n", zx_ram_memory_paging_disabled);
}
