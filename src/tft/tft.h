/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tft.h - definitions/declarations of TFT routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2018-2021 Frank Meyer - frank(at)fli4l.de
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
#ifndef TFT_H
#define TFT_H

#include <stdint.h>
#include "tft-config.h"

#if defined (SSD1963)
#  include "ssd1963.h"
#  define tft_write_data(x)     ssd1963_write_data(x)
#elif defined (ILI9341)
#  include "ili9341.h"
#  define tft_write_data(x)     ili9341_write_data(x)
#else
#  error Display driver not defined. Use -DSSD1963 or -DILI9341
#endif

#define BLACK565        0x0000
#define RED565          0xF800
#define GREEN565        0x07E0
#define BLUE565         0x001F
#define YELLOW565       (RED565   | GREEN565)
#define MAGENTA565      (RED565   | BLUE565)
#define CYAN565         (GREEN565 | BLUE565)
#define WHITE565        0xFFFF

extern void             tft_fadein_backlight (uint32_t);
extern void             tft_fadeout_backlight (uint32_t);
extern void             tft_backlight_on (void);
extern void             tft_backlight_off (void);

extern void             tft_set_area (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             tft_draw_pixel (uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             tft_draw_horizontal_line (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             tft_draw_vertical_line (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             tft_draw_rectangle (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t, uint16_t);
extern void             tft_fill_rectangle (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             tft_fill_screen (uint_fast16_t);
extern void             tft_draw_line (int, int, int, int, uint_fast16_t);
extern void             tft_draw_thick_line (int, int, int, int, uint_fast16_t);
extern void             tft_draw_circle (int, int, int, uint_fast16_t);
extern void             tft_draw_thick_circle (int, int, int, uint_fast16_t);
extern void             tft_draw_image (uint_fast16_t, uint_fast16_t, uint_fast16_t, uint_fast16_t, uint16_t *);
extern uint_fast16_t    tft_rgb64_to_color565 (uint_fast8_t, uint_fast8_t, uint_fast8_t);
extern uint_fast16_t    tft_rgb256_to_color565 (uint_fast8_t, uint_fast8_t, uint_fast8_t);
extern void             tft_init (void);

#endif // TFT_H
