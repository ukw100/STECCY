/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxfont.h - font functions
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
#ifndef LXFONT_H
#define LXFONT_H

#include <stdint.h>

#if 1 // we need only one font, not 14

#define FONT_08x12           5
#define N_FONTS              1

#else

#define FONT_05x08           0
#define FONT_05x12           1
#define FONT_06x08           2
#define FONT_06x10           3
#define FONT_08x08           4
#define FONT_08x12           5
#define FONT_08x14           6
#define FONT_10x16           7
#define FONT_12x16           8
#define FONT_12x20           9
#define FONT_16x26          10
#define FONT_22x36          11
#define FONT_24x40          12
#define FONT_32x53          13
#define N_FONTS             14

#endif
#ifdef unix
extern void     tft_show_screen (void);
#endif

extern int      number_of_fonts (void);
extern void     set_font (int);
extern int      font_width (void);
extern int      font_height (void);
extern void     draw_letter (unsigned char, unsigned int, unsigned int, uint32_t, uint32_t);
extern void     draw_string (unsigned char *, unsigned int, unsigned int, uint32_t, uint32_t);

#endif
