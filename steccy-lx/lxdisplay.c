/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxdisplay.c - ZX display emulation
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020 Frank Meyer - frank(at)fli4l.de
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
#include <string.h>
#include "z80.h"
#include "zxscr.h"
#include "zxram.h"
#if defined FRAMEBUFFER
#include "lxfb.h"
#elif defined X11
#include "lxx11.h"
#endif
#define FB_RGB(r,g,b)       ((r << 16) | (g << 8) | b)

unsigned int                zx_display_width;
unsigned int                zx_display_height;

#define ZOOM                2
#define TOP_OFFSET          ((zx_display_height - ZOOM * ZX_SPECTRUM_DISPLAY_ROWS - 2 * ZX_SPECTRUM_BORDER_SIZE) / 2)
#define LEFT_OFFSET         (((zx_display_width - 800) / 2) + 8)
static uint32_t             top_offset;
static uint32_t             left_offset;

/*------------------------------------------------------------------------------------------------------------------------
 * ZX Spectrum colors
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t rgbvalues[16][3] =
{
    { 0x00, 0x00, 0x00 },                            // black
    { 0x00, 0x00, 0xF0 },                            // blue
    { 0xF0, 0x00, 0x00 },                            // red
    { 0xF0, 0x00, 0xF0 },                            // magenta
    { 0x00, 0xF0, 0x00 },                            // green
    { 0x00, 0xF0, 0xF0 },                            // cyan
    { 0xF0, 0xF0, 0x00 },                            // yellow
    { 0xF0, 0xF0, 0xF0 },                            // white

    { 0x00, 0x00, 0x00 },                            // black
    { 0x00, 0x00, 0xFF },                            // blue
    { 0xFF, 0x00, 0x00 },                            // red
    { 0xFF, 0x00, 0xFF },                            // magenta
    { 0x00, 0xFF, 0x00 },                            // green
    { 0x00, 0xFF, 0xFF },                            // cyan
    { 0xFF, 0xFF, 0x00 },                            // yellow
    { 0xFF, 0xFF, 0xFF },                            // white
};

uint_fast8_t                 z80_display_cached = 0;

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_display - update Linux framebuffer or X11 window
 *
 * screen memory layout:
 * 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * 0  1  0  y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxscr_update_display (void)
{
    static uint8_t  shadow_display[6144];
    static uint8_t  shadow_attr[768];
    static uint8_t  counter;
    static uint8_t  inverse;
    static uint8_t  last_zx_border_color = 0xFF;
    uint16_t        addr;
    uint16_t        attr_addr;
    uint16_t        r;
    uint16_t        row;
    uint16_t        col;
    uint8_t         value;
    uint8_t         attr;
    uint8_t         ink;
    uint8_t         paper;
    uint8_t         video_ram_changed_copy;

    video_ram_changed_copy = video_ram_changed;                             // use a local value, because other task changes it
    video_ram_changed = 1;

    counter++;

    if (counter == 16)                                                      // change every 16/25 = 32/50 seconds bg and fg
    {
        inverse = inverse ? 0 : 1;
        counter = 0;
    }

    if (! z80_display_cached || last_zx_border_color != zx_border_color)
    {
        uint16_t    x1;
        uint16_t    x2;
        uint16_t    y1;
        uint16_t    y2;

        uint32_t    rgb = FB_RGB (rgbvalues[zx_border_color][0], rgbvalues[zx_border_color][1], rgbvalues[zx_border_color][2]);

        // upper border
        y1 = 0;
        y2 = ZX_SPECTRUM_BORDER_SIZE - 1;
        x1 = 0;
        x2 = ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZOOM * ZX_SPECTRUM_BORDER_SIZE - 1;

        fill_rectangle (x1 + left_offset, y1 + top_offset, x2 + left_offset, y2 + top_offset, rgb);

        // left border
        y1 = ZX_SPECTRUM_BORDER_SIZE;
        y2 = ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZX_SPECTRUM_BORDER_SIZE - 1;
        x1 = 0;
        x2 = ZX_SPECTRUM_BORDER_SIZE - 1;

        fill_rectangle (x1 + left_offset, y1 + top_offset, x2 + left_offset, y2 + top_offset, rgb);

        // right border
        y1 = ZX_SPECTRUM_BORDER_SIZE;
        y2 = ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZX_SPECTRUM_BORDER_SIZE - 1;
        x1 = ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZX_SPECTRUM_BORDER_SIZE;
        x2 = x1 + ZX_SPECTRUM_BORDER_SIZE - 1;

        fill_rectangle (x1 + left_offset, y1 + top_offset, x2 + left_offset, y2 + top_offset, rgb);

        // lower border
        y1 = ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZX_SPECTRUM_BORDER_SIZE;
        y2 = ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZOOM * ZX_SPECTRUM_BORDER_SIZE - 1;
        x1 = 0;
        x2 = ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZOOM * ZX_SPECTRUM_BORDER_SIZE - 1;

        fill_rectangle (x1 + left_offset, y1 + top_offset, x2 + left_offset, y2 + top_offset, rgb);

        last_zx_border_color = zx_border_color;
    }

    if (counter != 0 && ! video_ram_changed_copy)                                       // no flash inverting and no video ram content changed
    {
        return;
    }

    // attr_ram_blocked = 1;

    addr = ZX_SPECTRUM_DISPLAY_START_ADDRESS;

    for (r = 0; r < ZX_SPECTRUM_DISPLAY_ROWS; r++)
    {
        row         = ((addr & 0x0700) >> 8) | ((addr & 0x00E0) >> 2) | ((addr & 0x1800) >> 5);
        attr_addr   = UINT16_T (ZX_SPECTRUM_ATTRIBUTES_START_ADDR + (((row >> 3) << 5)));

        for (col = 0; col < ZX_SPECTRUM_DISPLAY_COLUMNS; col += 8, addr++, attr_addr++)
        {
            uint32_t    rgbset;
            uint32_t    rgbreset;
            uint16_t    mask = 0x80;
            uint16_t    idx;

            // video_ram_blocked_at = addr;

            value   = zx_ram_get_screen_8(addr);
            attr    = zx_ram_get_screen_8(attr_addr);

            if (z80_display_cached &&                                                       // ram initialized?
                value == shadow_display[addr - ZX_SPECTRUM_DISPLAY_START_ADDRESS] &&        // value not changed?
                attr  == shadow_attr[attr_addr - ZX_SPECTRUM_ATTRIBUTES_START_ADDR])        // attr not changed?
            {
                if (counter != 0 || ! (attr & FLASH_MASK))                                  // no flash inverting
                {                                                                           // and no flash attribute
                    continue;                                                               // skip update
                }
            }

            shadow_display[addr - ZX_SPECTRUM_DISPLAY_START_ADDRESS] = value;

            if (attr & FLASH_MASK)
            {
                if (inverse)
                {
                    ink     = (attr & PAPER_MASK) >> 3;
                    paper   = (attr & INK_MASK) >> 0;
                }
                else
                {
                    paper   = (attr & PAPER_MASK) >> 3;
                    ink     = (attr & INK_MASK) >> 0;
                }
            }
            else
            {
                paper   = (attr & PAPER_MASK) >> 3;
                ink     = (attr & INK_MASK) >> 0;
            }

            if (attr & BOLD_MASK)
            {
                paper += 8;
                ink += 8;
            }

            rgbset      = FB_RGB(rgbvalues[ink][0], rgbvalues[ink][1], rgbvalues[ink][2]);
            rgbreset    = FB_RGB(rgbvalues[paper][0], rgbvalues[paper][1], rgbvalues[paper][2]);

            for (idx = 0; idx < 8; idx++)
            {
                uint16_t    x1 = ZX_SPECTRUM_BORDER_SIZE + ZOOM * (col + idx);
                uint16_t    y1 = ZX_SPECTRUM_BORDER_SIZE + ZOOM * row;

                if (value & mask)
                {
                    fill_rectangle (x1 + left_offset, y1 + top_offset, x1 + left_offset + 1, y1 + top_offset + 1, rgbset);
                }
                else
                {
                    fill_rectangle (x1 + left_offset, y1 + top_offset, x1 + left_offset + 1, y1 + top_offset + 1, rgbreset);
                }

                mask >>= 1;
            }
        }
    }

#if defined X11
        x11_flush ();
#endif

    // video_ram_blocked_at = 0x000;
    memcpy (shadow_attr, zx_ram_screen_addr(ZX_SPECTRUM_ATTRIBUTES_START_ADDR), 768);

    // attr_ram_blocked = 0;
    z80_display_cached = 1;
}

void
lxdisplay_init (unsigned int width, unsigned int height)
{
    zx_display_width    = width;
    zx_display_height   = height;
    top_offset          = TOP_OFFSET;
    left_offset         = LEFT_OFFSET;
}
