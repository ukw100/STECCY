/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxram.h - ZX Spectrum emulator RAM functions
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

#define ZX_RAM_BEGIN                0x4000
#define STECCY_PAGE_SIZE            0x4000

#define STECCY_ROM_BEGIN            0x0000U
#define STECCY_ROM_SIZE             0x8000U                                                 // 2 ROM banks: 32K
#define STECCY_RAM_BEGIN            (STECCY_ROM_BEGIN + STECCY_ROM_SIZE)
#define STECCY_RAM_SIZE             0x20000U                                                // 8 RAM banks: 128K

/*------------------------------------------------------------------------------------------------------------------------
 * ZX ROM/RAM:
 *------------------------------------------------------------------------------------------------------------------------
*/
#define STECCY_CCRAMSIZE            0x10000                                                 // 64K
#define STECCY_RAMSIZE              0x18000                                                 // 96K

extern uint8_t *                    steccy_rombankptr[2];                                   // ptr to 2 ROM banks
extern uint8_t *                    steccy_rambankptr[8];                                   // ptr to 8 RAM banks

extern uint8_t *                    steccy_bankptr[4];                                      // 4 banks active
extern uint_fast8_t                 zx_ram_shadow_display;
extern uint_fast8_t                 zx_ram_memory_paging_disabled;

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_get_text () - get 8 bit program text from RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#define zx_ram_get_text(a)          (*(steccy_bankptr[(a) >> 14] + ((a) & 0x3FFF)))

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_get_8 () - get 8 bit data from RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef DEBUG
uint8_t                             zx_ram_get_8 (uint16_t addr);
#else
#define zx_ram_get_8(a)             (*(steccy_bankptr[(a) >> 14] + ((a) & 0x3FFF)))
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_get_screen_8 () - get 8 bit data from SCREEN RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#define zx_ram_get_screen_8(a)      (*((zx_ram_shadow_display ? steccy_rambankptr[7] : steccy_rambankptr[5]) + ((a) & 0x3FFF)))

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_get_16 () - get 16 bit data from RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#define zx_ram_get_16(addr)         (UINT16_T (zx_ram_get_8 (addr) + (zx_ram_get_8 ((addr) + 1) << 8)))

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_set_8 () - store 8 bit value into RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#if defined QT_CORE_LIB || defined DEBUG
extern void                         zx_ram_set_8 (uint16_t addr, uint8_t value);
#else
#define zx_ram_set_8(a,v)                                               \
do                                                                      \
{                                                                       \
    if ((a) >= ZX_RAM_BEGIN)                                            \
    {                                                                   \
        if ((a) < ZX_SPECTRUM_NON_VIDEO_RAM_START_ADDR)                 \
        {                                                               \
            video_ram_changed = 1;                                      \
        }                                                               \
                                                                        \
        (*(steccy_bankptr[(a) >> 14] + ((a) & 0x3FFF))) = (v);          \
    }                                                                   \
} while (0)
#endif


/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_set_16 () - store 16 bit value into RAM
 *------------------------------------------------------------------------------------------------------------------------
 */
#define zx_ram_set_16(a,v)          do { zx_ram_set_8 ((a), (v) & 0xFF); zx_ram_set_8 ((a) + 1, (v) >> 8); } while (0)

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_addr () - get address of ZX RAM address
 *------------------------------------------------------------------------------------------------------------------------
 */
#define zx_ram_addr(a)              (steccy_bankptr[(a) >> 14] + ((a) & 0x3FFF)])
#define zx_ram_screen_addr(a)       ((zx_ram_shadow_display ? steccy_rambankptr[7] : steccy_rambankptr[5]) + ((a) & 0x3FFF))

/*------------------------------------------------------------------------------------------------------------------------
 * zx_ram_init () - initialize ZX 128K RAM banking
 *------------------------------------------------------------------------------------------------------------------------
 */
extern void                         zx_ram_init (uint_fast16_t romsize);
