/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxscr.h - ZX Spectrum emulator screen functions
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
#include <stdint.h>

#define ZX_SPECTRUM_DISPLAY_START_ADDRESS       0x4000
#define ZX_SPECTRUM_ATTRIBUTES_START_ADDR       0x5800
#define ZX_SPECTRUM_NON_VIDEO_RAM_START_ADDR    0x5B00
#define ZX_SPECTRUM_DISPLAY_COLUMNS             256
#define ZX_SPECTRUM_DISPLAY_ROWS                192
#define ZX_SPECTRUM_DISPLAY_CHAR_COLUMNS        32
#define ZX_SPECTRUM_DISPLAY_CHAR_ROWS           24

/*------------------------------------------------------------------------------------------------------------------------
 * ZX Spectrum border
 *------------------------------------------------------------------------------------------------------------------------
 */
#if defined QT_CORE_LIB
#  define ZX_SPECTRUM_BORDER_SIZE               32              // 32 pixel (not doubled!)
#elif defined FRAMEBUFFER || defined X11
#  define ZX_SPECTRUM_BORDER_SIZE               32              // 32 pixel (not doubled!)
#elif defined STM32F4XX
#  if defined SSD1963
#    define ZX_SPECTRUM_BORDER_SIZE             32              // 32 pixel (not doubled!)
#  elif defined ILI9341
#    define ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE  25              // top/bottom: 24 pixel
#    define ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE  32              // left/right: 32 pixel
#  else
#    error only SSD1963 or ILI9341 supported
#  endif
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * ZX Spectrum attributes
 *------------------------------------------------------------------------------------------------------------------------
 */
#define FLASH_MASK              (1<<7)
#define BOLD_MASK               (1<<6)
#define PAPER_MASK              (0x38)
#define INK_MASK                (0x07)

#ifdef QT_CORE_LIB
extern volatile uint16_t        video_ram_blocked_at;                       // address of video ram currently blocked
extern volatile uint8_t         attr_ram_blocked;                           // flag: attribute ram currently blocked
extern volatile uint8_t         video_ram_changed;                          // flag: video ram changed
#else
extern uint8_t                  video_ram_changed;                          // flag: video ram changed
#ifdef ILI9341
extern uint_fast8_t             zxscr_display_cached;
#endif
#endif

extern uint8_t                  zx_border_color;                            // current border color - 3 bits used

#ifdef QT_CORE_LIB
extern void                     zxscr_init_display (void);
#endif

extern void                     zxscr_update_display (void);

#ifdef STM32F4XX
extern void                     zxscr_set_display_flags (void);
extern void                     zxscr_next_display_orientation (void);
extern void                     zxscr_next_display_rgb_order (void);
#endif
