/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxio.c - ZX-Spectrum I/O emulator I/O functions
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
#include "zxio.h"
#include "zxkbd.h"

#if defined (STM32F4XX)
#include "board-led.h"
#include "speaker.h"
#if 0
#include "i2c.h"
#endif
#include "zxkbd.h"
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * Keyboard matrix: 8 lines, 5 columns
 *
 *               D0 D1 D2 D3 D4   D4 D3 D2 D1 D0
 *  row 3 - A11  1  2  3  4  5    6  7  8  9  0   A12 - row 4
 *  row 2 - A10  Q  W  E  R  T    Y  U  I  O  P   A13 - row 5
 *  row 1 -  A9  A  S  D  F  G    H  J  K  L  CR  A14 - row 6
 *  row 0 -  A8  CS Z  X  C  V    B  N  M  SS SP  A15 - row 7
 *------------------------------------------------------------------------------------------------------------------------
 */
#define ZX_KEYBOARD_PORT    0xFE                                            // lower keyboard port

static uint8_t              kmatrix[ZX_KBD_ROWS] =                          // keyboard matrix
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/*------------------------------------------------------------------------------------------------------------------------
 * Reserved Ports:
 *
 *  Output:
 *  Hi  Lo
 *  --  FE    254       ---- ---- ---- ---0     ULA Border of screen, speaker & mic
 *  --  FD    253       ---- ---- 1111 1101     AY-3-8912 Soundchip of Spectrum 128K, not used
 *  --  FB    251       ---- ---- 1111 1011     ZX Printer, not used
 *  xx  7F    127       0xxx xxxx 0111 1111     I2C on STM32, where xx is the 7 bit address of I2C device
 *  FF  7F  65407       1111 1111 0111 1111     Board LEDs on STM32
 *  xx  7F              1xxx xxxx 0111 1111     reserved for future use, except FF7F for Board LEDs
 *
 *  Input:
 *  --  1F     31       ---- ---- 0001 1111     Kempston Joystick - 5 Bits used
 *  xx  7F    127       0xxx xxxx 0111 1111     I2C on STM32, where xx is the 7 bit address of I2C device
 *  FF  7F  65407       1111 1111 0111 1111     Board LEDs on STM32
 *  xx  7F              1xxx xxxx 0111 1111     reserved for future use, except FF7F for Board LEDs
 *
 *  STM32 I/O: Lo = 0x7F
 *
 *  Examples for STM32 IO:
 *
 *  Hi Lo     Dez       Bin
 *  20 7F    8139       PCF8574 at base address 0x20
 *  38 7F   14463       PCF8574A at base address 0x38
 *  48 7F   18559       LM75 at base address 0x48
 *  FF 7F   65407       STM32 BlackBoard LEDs
 *------------------------------------------------------------------------------------------------------------------------
 */
#define KEMPSTON_PORT       0x1F                                            // Port 31
#define ZX_OUTPUT_PORT      0xFE                                            // lower border port
#define STM32_IO_PORT       0x7F                                            // LEDs D2 & D3 on STM32 Blackboard

/*------------------------------------------------------------------------------------------------------------------------
 * Masks for ZX_OUTPUT_PORT:
 *------------------------------------------------------------------------------------------------------------------------
 */
#define ZX_BORDER_MASK      0x07                                            // ZX Spectrum border:               bits 0-2
#define ZX_MICRO_MASK       0x08                                            // ZX Spectrum microphone:           bit 3
#define ZX_SPEAKER_MASK     0x10                                            // ZX Spectrum ear output & speaker: bit 5

/*------------------------------------------------------------------------------------------------------------------------
 * Kempston Joystick - 5 Bits used
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t              kempston_value;                                 // state of Kempston Joystick

/*------------------------------------------------------------------------------------------------------------------------
 * STM32 Black Board LEDs - 2 Bits used
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t              led_state = 0x03;

/*------------------------------------------------------------------------------------------------------------------------
 * Value of I/O port 7FFD - used for ZX memory paging
 *------------------------------------------------------------------------------------------------------------------------
 */
uint8_t                     zxio_7ffd_value;

/*------------------------------------------------------------------------------------------------------------------------
 * OUT port
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
set_leds (void)
{
#ifdef STM32F4XX
    if (led_state & 0x01)
    {
        board_led2_off();
    }
    else
    {
        board_led2_on();
    }

    if (led_state & 0x02)
    {
        board_led3_off();
    }
    else
    {
        board_led3_on();
    }
#endif
}

void
zxio_reset (void)
{
    led_state               = 0x03;
    set_leds();

    z80_reset ();
}

/*------------------------------------------------------------------------------------------------------------------------
 * OUT port
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxio_out_port (uint8_t hi, uint8_t lo, uint8_t value)                            // here we ignore upper byte of address
{
    if (lo == ZX_OUTPUT_PORT)
    {
        zx_border_color = value & ZX_BORDER_MASK;

#ifdef STM32F4XX
        static uint_fast8_t  last_speaker_value = 0;

        if (value & ZX_SPEAKER_MASK)
        {
            if (! last_speaker_value)
            {
                speaker_high ();
                last_speaker_value = 1;
            }
        }
        else
        {
            if (last_speaker_value)
            {
                speaker_low ();
                last_speaker_value = 0;
            }
        }
#endif
    }
    else if (lo == STM32_IO_PORT)                                           // lo = 0111 1111
    {
#ifdef STM32F4XX
        if (! (hi & 0x80))                                                  // hi = 0bbb bbbb
        {
#if 0
            uint_fast8_t     i2c_addr;

            i2c_addr = hi << 1;                                             // 8 bit I2C address is: bbbb bbb0
            i2c_write_buf (i2c_addr, &value, 1);
#endif
        }
        else if (hi == 0xFF)                                                // hi = 1111 1111
        {
            led_state = value & 0x03;
            set_leds ();
        }
#else
        printf ("STM32 OUT 0x%02X%02X,%02X\n", hi, lo, value);
        fflush (stdout);
#endif
    }
    else if (hi == 0x7F && lo == 0xFD)
    {
        if (! zx_ram_memory_paging_disabled)
        {
            zxio_7ffd_value = value;
#ifdef DEBUG
            if (steccy_bankptr[3] != steccy_rambankptr[value & 0x07])
            {
                printf ("RAM Bank %d\n", value & 0x07);
            }
#endif
            steccy_bankptr[3] = steccy_rambankptr[value & 0x07];

            if (value & 0x08)
            {
#ifdef DEBUG
                if (! zx_ram_shadow_display)
                {
                    printf ("shadow screen enabled\n");
                }
#endif
                zx_ram_shadow_display = 1;
            }
            else
            {
#ifdef DEBUG
                if (zx_ram_shadow_display)
                {
                    printf ("shadow screen disabled\n");
                }
#endif
                zx_ram_shadow_display = 0;
            }

            if (value & 0x10)
            {
#ifdef DEBUG
                if (steccy_bankptr[0] != steccy_rombankptr[1])
                {
                    printf ("ROM Bank 1\n");
                }
#endif
                steccy_bankptr[0] = steccy_rombankptr[1];                   // ROM 1
            }
            else
            {
#ifdef DEBUG
                if (steccy_bankptr[0] != steccy_rombankptr[0])
                {
                    printf ("ROM Bank 0\n");
                }
#endif
                steccy_bankptr[0] = steccy_rombankptr[0];                   // ROM 0
            }

            if (value & 0x20)
            {
#ifdef DEBUG
                printf ("memory paging disabled\n");
#endif
                zx_ram_memory_paging_disabled = 1;                          // disable memory paging until reset
            }
        }

    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * IN port
 *------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
zxio_in_port (uint8_t hi, uint8_t lo)
{
    uint8_t rtc = 0xFF;

    if (lo == ZX_KEYBOARD_PORT)
    {
        hi = ~hi;

#ifdef STM32F4XX
        if (z80_settings.keyboard & KEYBOARD_ZX)                            // get row value of ZX keyboard
        {
            if (hi & 0x01) { rtc &= zxkbd_matrix[0]; }                      // code unrolled for more speed
            if (hi & 0x02) { rtc &= zxkbd_matrix[1]; }
            if (hi & 0x04) { rtc &= zxkbd_matrix[2]; }
            if (hi & 0x08) { rtc &= zxkbd_matrix[3]; }
            if (hi & 0x10) { rtc &= zxkbd_matrix[4]; }
            if (hi & 0x20) { rtc &= zxkbd_matrix[5]; }
            if (hi & 0x40) { rtc &= zxkbd_matrix[6]; }
            if (hi & 0x80) { rtc &= zxkbd_matrix[7]; }
        }
#endif

        if (hi & 0x01) { rtc &= kmatrix[0]; }                               // get row value of USB/PS2 keyboard (inverted ORed)
        if (hi & 0x02) { rtc &= kmatrix[1]; }                               // code unrolled for more speed
        if (hi & 0x04) { rtc &= kmatrix[2]; }
        if (hi & 0x08) { rtc &= kmatrix[3]; }
        if (hi & 0x10) { rtc &= kmatrix[4]; }
        if (hi & 0x20) { rtc &= kmatrix[5]; }
        if (hi & 0x40) { rtc &= kmatrix[6]; }
        if (hi & 0x80) { rtc &= kmatrix[7]; }
    }
    else if (lo == KEMPSTON_PORT)
    {
        rtc = kempston_value;
    }
    else if (lo == STM32_IO_PORT)                                           // lo = 0111 1111
    {
#ifdef STM32F4XX
#if 0
        uint_fast8_t    i2c_addr;
#endif
        uint8_t         value = 0xFF;

        if (! (hi & 0x80))                                                  // hi = 0bbb bbbb
        {
#if 0
            i2c_addr = hi << 1;                                             // 8 bit I2C address is: bbbb bbb0
            i2c_read_buf (i2c_addr, &value, 1);
#endif
            rtc = value;
        }
        else if (hi == 0xFF)                                                // hi = 1111 1111
        {
            rtc = led_state;
        }
#else
        printf ("STM32 IN 0x%02X%02X\n", hi, lo);
        fflush (stdout);
#endif
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * zxio_press_key() - press virtual key
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxio_press_key (uint8_t kb_idx)
{
    uint8_t kb_line     = kb_idx >> 4;
    uint8_t kb_column   = kb_idx & 0x0F;

    if (kb_line & 0x08)                                                                     // Kempston
    {
        kempston_value |= 1 << kb_column;                                                   // active high
    }
    else                                                                                    // Keyboard
    {
        kmatrix[kb_line] &= ~(1 << kb_column);                                              // active low
    }
}

/*------------------------------------------------------------------------------------------------------------------------
 * zxio_release_key() - release virtual key
 *------------------------------------------------------------------------------------------------------------------------
 */
void
zxio_release_key (uint8_t kb_idx)
{
    uint8_t kb_line     = kb_idx >> 4;
    uint8_t kb_column   = kb_idx & 0x0F;

    if (kb_line & 0x08)                                                                     // Kempston
    {
        kempston_value &= ~(1 << kb_column);                                                // active high
    }
    else                                                                                    // Keyboard
    {
        kmatrix[kb_line] |= (1 << kb_column);                                               // active low
    }
}

