/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxscr.c - ZX Spectrum emulator screen functions
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "z80.h"
#include "zxram.h"
#include "zxscr.h"

#if defined QT_CORE_LIB
#include "mywindow.h"
static MyWidget *                   myw;
#elif defined STM32F4XX
#include "tft.h"
#include "font.h"
#include "wii-gamepad.h"
#endif

#ifdef QT_CORE_LIB
volatile uint16_t           video_ram_blocked_at    = 0x0000;               // address of video ram currently blocked
volatile uint8_t            attr_ram_blocked        = 0;                    // flag: attribute ram currently blocked
volatile uint8_t            video_ram_changed       = 1;                    // flag: video ram changed
#else
uint8_t                     video_ram_changed       = 1;                    // flag: video ram changed
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * Border Color - 3 Bits used
 *------------------------------------------------------------------------------------------------------------------------
 */
uint8_t                     zx_border_color;                                // current border color

/*------------------------------------------------------------------------------------------------------------------------
 * ZX Spectrum colors
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef QT_CORE_LIB
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

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_display - update display
 *
 * screen memory layout:
 * 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * 0  1  0  y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
 *------------------------------------------------------------------------------------------------------------------------
 */
#define ZXSCR_ZOOM        2

void
zxscr_init_display (void)
{
    myw = new MyWidget();
    myw->show ();
}

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
    static uint8_t  called;

    video_ram_changed_copy = video_ram_changed;                             // use a local value, because other task changes it
    video_ram_changed = 1;

    counter++;

    if (counter == 16)                                                      // change every 16/25 = 32/50 seconds bg and fg
    {
        inverse = inverse ? 0 : 1;
        counter = 0;
    }

    if (last_zx_border_color != zx_border_color)
    {
        uint16_t    y;
        uint16_t    x;
        QRgb        rgb = qRgb(rgbvalues[zx_border_color][0], rgbvalues[zx_border_color][1], rgbvalues[zx_border_color][2]);

        for (y = 0; y < ZX_SPECTRUM_BORDER_SIZE; y++)
        {
            for (x = 0; x < ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZXSCR_ZOOM * ZX_SPECTRUM_BORDER_SIZE; x++)
            {
                myw->image->setPixel(x, y, rgb);
            }
        }

        for (y = ZX_SPECTRUM_BORDER_SIZE; y < ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZX_SPECTRUM_BORDER_SIZE; y++)
        {
            for (x = 0; x < ZX_SPECTRUM_BORDER_SIZE; x++)
            {
                myw->image->setPixel(x, y, rgb);
                myw->image->setPixel(x + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZX_SPECTRUM_BORDER_SIZE, y, rgb);
            }
        }

        for (y = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZX_SPECTRUM_BORDER_SIZE;
             y < ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + ZXSCR_ZOOM * ZX_SPECTRUM_BORDER_SIZE;
             y++)
        {
            for (x = 0; x < ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + ZXSCR_ZOOM * ZX_SPECTRUM_BORDER_SIZE; x++)
            {
                myw->image->setPixel(x, y, rgb);
            }
        }

        last_zx_border_color = zx_border_color;
    }

    if (counter != 0 && ! video_ram_changed_copy)                                       // no flash inverting and no video ram content changed
    {
        return;
    }

    attr_ram_blocked = 1;

    addr = ZX_SPECTRUM_DISPLAY_START_ADDRESS;

    for (r = 0; r < ZX_SPECTRUM_DISPLAY_ROWS; r++)
    {
        row         = ((addr & 0x0700) >> 8) | ((addr & 0x00E0) >> 2) | ((addr & 0x1800) >> 5);
        attr_addr   = UINT16_T (ZX_SPECTRUM_ATTRIBUTES_START_ADDR + (((row >> 3) << 5)));

        for (col = 0; col < ZX_SPECTRUM_DISPLAY_COLUMNS; col += 8, addr++, attr_addr++)
        {
            QRgb        rgbset;
            QRgb        rgbreset;
            uint16_t    mask = 0x80;
            uint16_t    idx;

            video_ram_blocked_at = addr;

            value   = zx_ram_get_screen_8(addr);
            attr    = zx_ram_get_screen_8(attr_addr);

            if (called &&                                                                   // ram initialized?
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

            rgbset      = qRgb(rgbvalues[ink][0], rgbvalues[ink][1], rgbvalues[ink][2]);
            rgbreset    = qRgb(rgbvalues[paper][0], rgbvalues[paper][1], rgbvalues[paper][2]);

            for (idx = 0; idx < 8; idx++)
            {
                if (value & mask)
                {
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx),      ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row,     rgbset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx) + 1,  ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row,     rgbset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx),      ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row + 1, rgbset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx) + 1 , ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row + 1, rgbset);
                }
                else
                {
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx),      ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row,     rgbreset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx) + 1,  ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row,     rgbreset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx),      ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row + 1, rgbreset);
                    myw->image->setPixel(ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * (col + idx) + 1 , ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row + 1, rgbreset);
                }

                mask >>= 1;
            }
        }
    }

    video_ram_blocked_at = 0x000;
    memcpy (shadow_attr,  zx_ram_screen_addr(ZX_SPECTRUM_ATTRIBUTES_START_ADDR), 768);
    attr_ram_blocked = 0;
    called = 1;
}

#elif defined STM32F4XX

static uint16_t rgb565values[16] =
{
    // dark colors
    0x0000,                                         // black
    0x001E,                                         // blue
    0xF000,                                         // red
    0xF01E,                                         // magenta
    0x0780,                                         // green
    0x079E,                                         // cyan
    0xF780,                                         // yellow
    0xF79E,                                         // white

    // light colors
    0x0000,                                         // black
    0x001F,                                         // blue
    0xF800,                                         // red
    0xF81F,                                         // magenta
    0x07E0,                                         // green
    0x07FF,                                         // cyan
    0xFFE0,                                         // yellow
    0xFFFF,                                         // white
};

uint_fast8_t    zxscr_force_update = 1;

#if TFT_WIDTH == 800

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_display - update display
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

    counter++;

    if (counter == 16)                                                                      // change every 16/25 = 32/50 seconds bg and fg
    {
        inverse = inverse ? 0 : 1;
        counter = 0;
    }

    if (zxscr_force_update || last_zx_border_color != zx_border_color)
    {
        uint16_t        x;
        uint16_t        y;
        uint16_t        w;
        uint16_t        h;
        uint_fast16_t   rgb565 = rgb565values[zx_border_color];

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET;
        w = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZX_SPECTRUM_BORDER_SIZE;
        h = ZX_SPECTRUM_BORDER_SIZE;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                            // upper border

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE;
        w = ZX_SPECTRUM_BORDER_SIZE;
        h = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                            // left border

        x = ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE;
        w = ZX_SPECTRUM_BORDER_SIZE;
        h = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                            // right border

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        w = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZX_SPECTRUM_BORDER_SIZE;
        h = ZX_SPECTRUM_BORDER_SIZE;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                            // lower border

        last_zx_border_color = zx_border_color;
    }

    if (! zxscr_force_update && counter != 0 && ! video_ram_changed)                        // no flash inverting and no video ram content changed
    {
        return;
    }

    addr = ZX_SPECTRUM_DISPLAY_START_ADDRESS;

    for (r = 0; r < ZX_SPECTRUM_DISPLAY_ROWS; r++)
    {
        row         = ((addr & 0x0700) >> 8) | ((addr & 0x00E0) >> 2) | ((addr & 0x1800) >> 5);
        attr_addr   = UINT16_T (ZX_SPECTRUM_ATTRIBUTES_START_ADDR + (((row >> 3) << 5)));
        uint16_t y  = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * row;
        uint16_t x  = ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_BORDER_SIZE;

        for (col = 0; col < ZX_SPECTRUM_DISPLAY_COLUMNS; col += 8, addr++, attr_addr++)
        {
            uint16_t        mask = 0x80;
            uint16_t        idx;

            value   = zx_ram_get_screen_8(addr);
            attr    = zx_ram_get_screen_8(attr_addr);

            if (! zxscr_force_update &&                                                     // ram initialized?
                value == shadow_display[addr - ZX_SPECTRUM_DISPLAY_START_ADDRESS] &&        // value not changed?
                attr  == shadow_attr[attr_addr - ZX_SPECTRUM_ATTRIBUTES_START_ADDR])        // attr not changed?
            {
                if (counter != 0 || ! (attr & FLASH_MASK))                                  // no flash inverting
                {                                                                           // and no flash attribute
                    x += 8 * ZXSCR_ZOOM;
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

            if (value == 0x00)                                                              // optimize: ZXSCR_ZOOM * 8 pixels with paper color
            {
                tft_fill_rectangle (x, y, x + (8 * ZXSCR_ZOOM - 1), y + 1, rgb565values[paper]);
                x += 8 * ZXSCR_ZOOM;
            }
            else if (value == 0xFF)                                                         // optimize: ZXSCR_ZOOM * 8 pixels with ink color
            {
                tft_fill_rectangle (x, y, x + (8 * ZXSCR_ZOOM - 1), y + 1, rgb565values[ink]);
                x += 8 * ZXSCR_ZOOM;
            }
            else
            {
                for (idx = 0; idx < 8; idx++)
                {
                    if (value & mask)
                    {
                        tft_fill_rectangle (x, y, x + 1, y + 1, rgb565values[ink]);
                    }
                    else
                    {
                        tft_fill_rectangle (x, y, x + 1, y + 1, rgb565values[paper]);
                    }

                    x += ZXSCR_ZOOM;
                    mask >>= 1;
                }
            }
        }
    }

    memcpy (shadow_attr,  zx_ram_screen_addr(ZX_SPECTRUM_ATTRIBUTES_START_ADDR), 768);
    zxscr_force_update = 0;
    video_ram_changed = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_status () - update lower status line on SSD1963
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxscr_update_status (void)
{
    uint_fast16_t           x;
    uint_fast16_t           y;

    y = TFT_HEIGHT - font_height () - 2;

    if (wii_type != WII_TYPE_NONE)
    {
        x = TFT_WIDTH - 25 * font_width() - 2;                                          // 7 characters to print at (-28)

        if (wii_type == WII_TYPE_NUNCHUK)
        {
            draw_string ((unsigned char *) "Nunchuk", y, x, RED565, BLACK565);
        }
        else // if (wii_type == WII_TYPE_GAMEPAD)
        {
            draw_string ((unsigned char *) "Gamepad", y, x, RED565, BLACK565);
        }
    }

    x = TFT_WIDTH - 17 * font_width() - 2;                                              // 5 characters to print

    if (z80_get_turbo_mode ())
    {
        draw_string ((unsigned char *) "Turbo", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "     ", y, x, RED565, BLACK565);
    }

    x = TFT_WIDTH - 11 * font_width() - 2;                                              // 7 characters to print

    if (z80_get_rom_hooks ())
    {
        draw_string ((unsigned char *) "Hooks", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "     ", y, x, RED565, BLACK565);
    }

    x = TFT_WIDTH - 5 * font_width() - 2;                                              // 4 characters to print

    if (z80_romsize == 0x4000)
    {
        draw_string ((unsigned char *) " 48K", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "128K", y, x, RED565, BLACK565);
    }
}

#elif TFT_WIDTH == 320

uint_fast8_t                        zxscr_display_cached = 0;

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_display - update display - ILI9341 version
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

    if (zxscr_force_update)
    {
        zxscr_display_cached = 0;
    }

    counter++;

    if (counter == 16)                                                                      // change every 16/25 = 32/50 seconds bg and fg
    {
        inverse = inverse ? 0 : 1;
        counter = 0;
    }

    if (! zxscr_display_cached || last_zx_border_color != zx_border_color)
    {
        uint16_t        x;
        uint16_t        y;
        uint16_t        w;
        uint16_t        h;
        uint_fast16_t   rgb565 = rgb565values[zx_border_color];

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET;
        w = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZXSCR_ZOOM * ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE;
        h = ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                // upper border

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE;
        w = ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE;
        h = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                // left border

        x = ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE;
        w = ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE;
        h = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                // right border

        x = ZXSCR_LEFT_OFFSET;
        y = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;
        w = ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZXSCR_ZOOM * ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE;
        h = ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE;
        tft_fill_rectangle (x, y, x + w - 1, y + h - 1, rgb565);                // lower border

        last_zx_border_color = zx_border_color;
    }

    if (! zxscr_force_update && counter != 0 && ! video_ram_changed)            // no flash inverting and no video ram content changed
    {
        return;
    }

    addr = ZX_SPECTRUM_DISPLAY_START_ADDRESS;

    for (r = 0; r < ZX_SPECTRUM_DISPLAY_ROWS; r++)
    {
        row         = ((addr & 0x0700) >> 8) | ((addr & 0x00E0) >> 2) | ((addr & 0x1800) >> 5);
        attr_addr   = UINT16_T (ZX_SPECTRUM_ATTRIBUTES_START_ADDR + (((row >> 3) << 5)));
        uint16_t y  = ZXSCR_TOP_OFFSET + ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE + ZXSCR_ZOOM * row;
        uint16_t x  = ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE;

        for (col = 0; col < ZX_SPECTRUM_DISPLAY_COLUMNS; col += 8, addr++, attr_addr++)
        {
            uint16_t        mask = 0x80;
            uint16_t        idx;

            value   = zx_ram_get_screen_8(addr);
            attr    = zx_ram_get_screen_8(attr_addr);

            if (zxscr_display_cached &&                                                     // ram initialized?
                value == shadow_display[addr - ZX_SPECTRUM_DISPLAY_START_ADDRESS] &&        // value not changed?
                attr  == shadow_attr[attr_addr - ZX_SPECTRUM_ATTRIBUTES_START_ADDR])        // attr not changed?
            {
                if (counter != 0 || ! (attr & FLASH_MASK))                                  // no flash inverting
                {                                                                           // and no flash attribute
                    x += 8 * ZXSCR_ZOOM;
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

            tft_set_area (x, x + (ZXSCR_ZOOM * 8 - 1), y, y);                                 // set area here: 8 Pixels following

            for (idx = 0; idx < 8; idx++)
            {
                if (value & mask)
                {
                    tft_write_data (rgb565values[ink]);
                }
                else
                {
                    tft_write_data (rgb565values[paper]);
                }

                x += ZXSCR_ZOOM;
                mask >>= 1;
            }
        }
    }

    memcpy (shadow_attr,  zx_ram_screen_addr(ZX_SPECTRUM_ATTRIBUTES_START_ADDR), 768);
    zxscr_display_cached = 1;
    zxscr_force_update = 0;
    video_ram_changed = 0;
}

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_update_status () - update lower status line on ILI9341
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxscr_update_status (void)
{
    uint_fast16_t           x;
    uint_fast16_t           y;

    x = 0;
    y = ZXSCR_TOP_OFFSET + 2 * ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS;

    tft_fill_rectangle (x, y, TFT_WIDTH - 1, TFT_HEIGHT - 1, BLACK565);

    y = ZXSCR_TOP_OFFSET + 2 * ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + 2;

    if (wii_type != WII_TYPE_NONE)
    {
        x = 2;

        if (wii_type == WII_TYPE_NUNCHUK)
        {
            draw_string ((unsigned char *) "Nunchuk", y, x, RED565, BLACK565);
        }
        else // if (wii_type == WII_TYPE_GAMEPAD)
        {
            draw_string ((unsigned char *) "Gamepad", y, x, RED565, BLACK565);
        }
    }

    x = TFT_WIDTH - 21 * font_width() - 2;                                              // 5 characters to print at (-21)

    if (z80_get_turbo_mode ())
    {
        draw_string ((unsigned char *) "TURBO", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "     ", y, x, RED565, BLACK565);
    }

    x = TFT_WIDTH - 14 * font_width() - 2;                                              // 7 characters to print at (-14)

    if (z80_get_rom_hooks ())
    {
        draw_string ((unsigned char *) "FASTROM", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "       ", y, x, RED565, BLACK565);
    }

    x = TFT_WIDTH - 4 * font_width() - 2;                                              // 4 characters to print

    if (z80_romsize == 0x4000)
    {
        draw_string ((unsigned char *) " 48K", y, x, RED565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "128K", y, x, RED565, BLACK565);
    }
}

#else
#  error only TFT_WIDTH == 320 or TFT_WIDTH == 800 supported
#endif

#endif // QT_CORE_LIB

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_set_display_flags () - set display rgb order and orientation
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef STM32F4XX
void
zxscr_set_display_flags (void)
{
#if defined (SSD1963)
    uint_fast8_t    flags;

    switch (z80_settings.rgb_order)
    {
        case 0:     flags = SSD1963_GLOBAL_FLAGS_RGB_ORDER;     break;                      // default is RGB
        default:    flags = 0;
    }

    switch (z80_settings.orientation)
    {
        case 1:
            flags |= SSD1963_GLOBAL_FLAGS_FLIP_VERTICAL;
            break;
        case 2:
            flags |= SSD1963_GLOBAL_FLAGS_FLIP_HORIZONTAL;
            break;
        case 3:
            flags |= SSD1963_GLOBAL_FLAGS_FLIP_VERTICAL | SSD1963_GLOBAL_FLAGS_FLIP_HORIZONTAL;
            break;
        default: // 0
            z80_settings.orientation = 0;
            break;
    }
    ssd1963_set_flags (flags);
#elif defined (ILI9341)
    uint_fast8_t    flags;

    switch (z80_settings.rgb_order)
    {
        case 0:     flags = 0;                                      break;                      // default is GRB
        default:    flags = ILI9341_GLOBAL_FLAGS_RGB_ORDER;         break;
    }

    switch (z80_settings.orientation)
    {
        case 1:
            flags |= ILI9341_GLOBAL_FLAGS_FLIP_VERTICAL;
            break;
        case 2:
            flags |= ILI9341_GLOBAL_FLAGS_FLIP_HORIZONTAL;
            break;
        case 3:
            flags |= ILI9341_GLOBAL_FLAGS_FLIP_VERTICAL | ILI9341_GLOBAL_FLAGS_FLIP_HORIZONTAL;
            break;
        default: // 0
            z80_settings.orientation = 0;
            break;
    }
    ili9341_set_flags (flags);
    zxscr_display_cached = 0;                                              // redraw screen content, ILI9341 does it not by itself
#else
#  error Display driver not defined. Use -DSSD1963 or -DILI9341
#endif
}
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_next_display_orientation () - set next display orientation
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef STM32F4XX
void
zxscr_next_display_orientation (void)
{
    z80_settings.orientation++;

    if (z80_settings.orientation == 4)
    {
        z80_settings.orientation = 0;
    }

    zxscr_set_display_flags ();
    zxscr_update_status ();
}
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * zxscr_next_display_rgb_order () - set next display rgb
 *------------------------------------------------------------------------------------------------------------------------
 */
#ifdef STM32F4XX
void
zxscr_next_display_rgb_order (void)
{
    z80_settings.rgb_order++;

    if (z80_settings.rgb_order == 2)
    {
        z80_settings.rgb_order = 0;
    }

    zxscr_set_display_flags ();
}
#endif
