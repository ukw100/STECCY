/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii-gamepad.c - Wii Gamepad routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 Frank Meyer - frank(at)fli4l.de
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
#include <string.h>

#include "console.h"
#include "wii-gamepad.h"
#include "i2c1-dma.h"

#define WII_ADDR                    0xA4                            // I2C address 0x52 << 1

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Wii Gamepad addresses and control registers
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define WII_START_ADDR              0x00                            // start of data offset
#define WII_CAL_ADDR                0x20                            // start of calibration offset
#define WII_IDENT_NUMBER_ADDR       0xFA                            // Ident number offset
#define WII_NUNCHUK_IDENT_NUMBER    0xA4200000                      // Nunchuk Controller
#define WII_CLASSIC_IDENT_NUMBER    0xA4200101                      // Gamepad Classsic Controller
#define WII_BALANCE_IDENT_NUMBER    0xA4200402                      // Balance Controller

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Wii Gamepad data
 *
 *  Byte |   Bit7       Bit6       Bit5       Bit4       Bit3       Bit2       Bit1       Bit0
 *  -----+----------+----------+----------+----------+----------+----------+----------+----------+
 *  0 	 | RX<4:3> 	           | LX<5:0>                                                         |
 *  1 	 | RX<2:1> 	           | LY<5:0>                                                         |
 *  2 	 | RX<0>    | LT<4:3> 	          | RY<4:0>                                              |
 *  3 	 | LT<2:0> 	                      | RT<4:0>                                              |
 *  4 	 | BDR 	    | BDD 	   | BLT 	  | B– 	     | BH 	    | B+ 	    | BRT 	  | 1        |
 *  5 	 | BZL 	    | BB 	   | BY 	  | BA 	     | BX 	    | BZR 	    | BDL 	  | BDU      |
 *  -----+----------+----------+----------+----------+----------+----------+----------+----------+
 *
 * Wii Nunchuk data:
 *
 *  Byte |   Bit7       Bit6       Bit5       Bit4       Bit3       Bit2       Bit1       Bit0
 *  -----+----------+----------+----------+----------+----------+----------+----------+----------+
 *  0 	 | LX<7:0> 	                                                                             |
 *  1 	 | LY<7:0>                                                                               |
 *  2 	 | Accelerometer X-Axis Ax [9:2]                                                         |
 *  3 	 | Accelerometer Y-Axis Ay [9:2]                                                         |
 *  4 	 | Accelerometer Z-Axis Az [9:2]                                                         |
 *  5 	 | Az [1:0]	           | Ay [1:0] 	         | Ax [1:0] 	       | BC 	  | BZ       |
 *  -----+----------+----------+----------+----------+----------+----------+----------+----------+
 *
 * Wii Nunchuk Calibration data (16 bytes) beginning at 0x20:
 *
 *  0	0G value of X-axis [9:2]
 *  1	0G value of Y-axis [9:2]
 *  2	0G value of Z-axis [9:2]
 *  3	LSB of Zero value of X,Y,Z axes
 *  4	1G value of X-axis [9:2]
 *  5	1G value of Y-axis [9:2]
 *  6	1G value of Z-axis [9:2]
 *  7	LSB of 1G value of X,Y,Z axes
 *  8	Joystick X-axis maximum
 *  9	Joystick X-axis minimum
 * 10	Joystick X-axis center
 * 11	Joystick Y-axis maximum
 * 12	Joystick Y-axis minimum
 * 13	Joystick Y-axis center
 * 14	Checksum
 * 15	Checksum
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t        wii_type;
WII_DATA            wii_data;
uint_fast8_t        wii_state   = WII_STATE_IDLE;

static uint8_t      tx_buffer[8];
static uint8_t      rx_buffer[8];

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii_decode_data() - decode data of wii device
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
wii_decode_data (void)
{
    if (wii_type == WII_TYPE_GAMEPAD)
    {
        wii_data.lx         = (rx_buffer[0] & 0x3F);
        wii_data.ly         = (rx_buffer[1] & 0x3F);
        wii_data.rx         = ((rx_buffer[0] & 0xC0) >> 3) | ((rx_buffer[1] & 0xC0) >> 5) | ((rx_buffer[2] & 0x80) >> 7);
        wii_data.ry         = (rx_buffer[2] & 0x1F);
        wii_data.lt         = ((rx_buffer[2] & 0x60) >> 2) | (rx_buffer[3] & 0xE0 >> 5);
        wii_data.buttons    = ~((rx_buffer[4] << 8) | rx_buffer[5]);    // invert: 1 = pressed, 0 = not pressed
    }
    else
    {
        wii_data.lx         = rx_buffer[0];
        wii_data.ly         = rx_buffer[1];

        if (rx_buffer[5] & 0x02)                                        // Bit 2 set -> button C released
        {
            wii_data.buttons &= ~WII_BUTTON_C_MASK;
        }
        else                                                            // Bit 2 not set -> button C pressed
        {
            wii_data.buttons |= WII_BUTTON_C_MASK;
        }

        if (rx_buffer[5] & 0x01)                                        // Bit 2 set -> button Z released
        {
            wii_data.buttons &= ~WII_BUTTON_Z_LEFT_MASK;
        }
        else                                                            // Bit 2 not set -> button Z pressed
        {
            wii_data.buttons |= WII_BUTTON_Z_LEFT_MASK;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii_finished_transmission() - check if transmission finished, then decode data
 *
 * Return values:
 *  0   Failed
 *  1   Successful
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
wii_finished_transmission (void)
{
    uint_fast8_t    rtc = 0;

    if (wii_state == WII_STATE_BUSY)
    {
        if (i2c1_dma_state == I2C1_DMA_STATE_IDLE)
        {
            wii_decode_data ();
            wii_state = WII_STATE_IDLE;
            rtc = 1;
        }
    }
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii_start_transmission() - start wii transmission
 *
 * Return values:
 *  0   Failed
 *  1   Successful
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
wii_start_transmission (void)
{
    uint_fast8_t    rtc = 0;

    if (wii_state == WII_STATE_IDLE)
    {
        tx_buffer[0] = WII_START_ADDR;
        rtc = i2c1_dma_start (WII_ADDR, 1, 6);

        if (rtc)
        {
            wii_state = WII_STATE_BUSY;
        }
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wii_init () - initialize wii gamepad/nunchuk
 *
 * Return values:
 *  0   Failed
 *  1   Successful
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
wii_init (void)
{
    uint8_t         ident_buffer_nunchuk[6]         = { 0x00, 0x00, 0xA4, 0x20, 0x00, 0x00 };   // ident number of nunchuk
    uint8_t         ident_buffer_gamepad[6]         = { 0x00, 0x00, 0xA4, 0x20, 0x01, 0x01 };   // ident number of gamepad classic
    uint8_t         ident_buffer_gamepad_pro[6]     = { 0x01, 0x00, 0xA4, 0x20, 0x01, 0x01 };   // ident number of gamepad classic pro
    uint8_t         buffer[6];
    int_fast16_t    i2c_rtc;
    uint8_t         rtc = 0;

    i2c1_init (400000);

    // initialize nunchuk/gamepad etc. without encryption
    buffer[0] = 0xF0;
    buffer[1] = 0x55;

    i2c_rtc = i2c1_write (WII_ADDR, buffer, 2);

    if (i2c_rtc == I2C_OK)
    {
        buffer[0] = 0xFB;
        buffer[1] = 0x00;

        i2c_rtc = i2c1_write (WII_ADDR, buffer, 2);

        if (i2c_rtc == I2C_OK)
        {
            buffer[0] = WII_IDENT_NUMBER_ADDR;

            i2c_rtc = i2c1_write (WII_ADDR, buffer, 1);

            if (i2c_rtc == I2C_OK)
            {
                i2c_rtc = i2c1_read (WII_ADDR, buffer, 6);

                if (i2c_rtc == I2C_OK)
                {
                    if (memcmp (ident_buffer_nunchuk, buffer, 6) == 0)
                    {
                        wii_type = WII_TYPE_NUNCHUK;
                        rtc = 1;
                    }
                    else if (memcmp (ident_buffer_gamepad, buffer, 6) == 0)
                    {
                        wii_type = WII_TYPE_GAMEPAD;
                        rtc = 1;
                    }
                    else if (memcmp (ident_buffer_gamepad_pro, buffer, 6) == 0)
                    {
                        wii_type = WII_TYPE_GAMEPAD;
                        rtc = 1;
                    }
                    else
                    {
                        uint8_t idx;

                        console_puts ("unknown wii device id: ");

                        for (idx = 0; idx < 6; idx++)
                        {
                            console_printf ("%02X", buffer[idx]);
                        }
                        console_puts ("\r\n");
                    }

                    if (rtc == 1)
                    {
                        i2c1_dma_init (tx_buffer, rx_buffer);
                    }
                }
            }
        }
    }

    return rtc;
}
