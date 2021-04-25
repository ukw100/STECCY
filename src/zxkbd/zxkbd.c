/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd.c - ZX Spectrum emulator ZX keyboard functions
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
#include <string.h>
#include "stm32f4xx_gpio.h"
#include "zxkbd.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "io.h"
#include "z80.h"
#include "zxkbd.h"
#include "ps2key.h"
#include "usb_hid_host.h"
#include "delay.h"

/*------------------------------------------------------------------------------------------------------------------------
 * Keyboard matrix: 8 lines, 5 columns
 *
 *               D0 D1 D2 D3 D4   D4 D3 D2 D1 D0
 *  row 3 - A11  1  2  3  4  5    6  7  8  9  0   A12 - row 4
 *  row 2 - A10  Q  W  E  R  T    Y  U  I  O  P   A13 - row 5
 *  row 1 -  A9  A  S  D  F  G    H  J  K  L  CR  A14 - row 6
 *  row 0 -  A8  CS Z  X  C  V    B  N  M  SS SP  A15 - row 7
 *
 *  Keyboard connector on original Spectrum board:
 *
 *              ---  ---  ---  ---  ---            ---  ---  ---  ---  ---  ---  ---  ---
 *  Z80         D0   D1   D2   D3   D4             A11  A10  A9   A12  A13  A8   A14  A15
 *  STM32       PE0  PE1  PE2  PE3  PE4            PC3  PC2  PC1  PC4  PC5  PC0  PC6  PC7
 *------------------------------------------------------------------------------------------------------------------------
 */
#define ROW_SHIFT           0                                                   // Caps Shift "CS"
#define MASK_SHIFT          (1 << 0)

#define ROW_SYM_SHIFT       7                                                   // Symbol Shift "SS"
#define MASK_SYM_SHIFT      (1 << 1)

#define ROW_D_ARROW         4                                                   // CS + 6 = Down Arrow
#define MASK_D_ARROW        (1 << 4)

#define ROW_U_ARROW         4                                                   // CS + 7 = Up Arrow
#define MASK_U_ARROW        (1 << 3)

#define ROW_BSP             4                                                   // CS + 0 = Backspace
#define MASK_BSP            (1 << 0)

#define ROW_ESC             7                                                   // CS + Space = Break = Esc
#define MASK_ESC            (1 << 0)

#define ROW_MINUS           6                                                   // SS + J = Minus
#define MASK_MINUS          (1 << 3)

static const uint16_t       scancodes[ZX_KBD_ROWS][ZX_KBD_COLS]=
{   //      D0                  D1              D2              D3              D4
    {   SCANCODE_LSHFT,     SCANCODE_Z,     SCANCODE_X,     SCANCODE_C,     SCANCODE_V  },
    {   SCANCODE_A,         SCANCODE_S,     SCANCODE_D,     SCANCODE_F,     SCANCODE_G  },
    {   SCANCODE_Q,         SCANCODE_W,     SCANCODE_E,     SCANCODE_R,     SCANCODE_T  },
    {   SCANCODE_1,         SCANCODE_2,     SCANCODE_3,     SCANCODE_4,     SCANCODE_5  },
    {   SCANCODE_0,         SCANCODE_9,     SCANCODE_8,     SCANCODE_7,     SCANCODE_6  },
    {   SCANCODE_P,         SCANCODE_O,     SCANCODE_I,     SCANCODE_U,     SCANCODE_Y  },
    {   SCANCODE_ENTER,     SCANCODE_L,     SCANCODE_K,     SCANCODE_J,     SCANCODE_H  },
    {   SCANCODE_SPACE,     0,              SCANCODE_M,     SCANCODE_N,     SCANCODE_B  }
};

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keyboard for STECCY
 *
 * 0 = pressed, 1 = released
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t                     zxkbd_matrix[ZX_KBD_ROWS];                          // ZX keyboard matrix: 0 = pressed, 1 = released

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * internal keyboard matrix for STECCY Menu - values are inverted!
 *
 * 1 = pressed, 0 = released
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t              zxkbd_inv_matrix[ZX_KBD_ROWS];                      // ZX inverted keyboard matrix
static uint8_t              last_zxkbd_inv_matrix[ZX_KBD_ROWS];                 // last state of inverted keyboard matrix

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd_init() - initialize ZX keyboard matrix
 *
 *    ZX keyboard row 0       PC0
 *    ZX keyboard row 1       PC1
 *    ZX keyboard row 2       PC2
 *    ZX keyboard row 3       PC3
 *    ZX keyboard row 4       PC4
 *    ZX keyboard row 5       PC5
 *    ZX keyboard row 6       PC6
 *    ZX keyboard row 7       PC7
 *
 *    ZX keyboard column 0    PE0
 *    ZX keyboard column 1    PE1
 *    ZX keyboard column 2    PE2
 *    ZX keyboard column 3    PE3
 *    ZX keyboard column 4    PE4
 *    ZX extra    column 5    PE5
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
zxkbd_init (void)
{
  GPIO_InitTypeDef gpio = {0};

  /* GPIO Ports Clock Enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

  /* Configure GPIO pin Output Level */
  GPIO_SET_BIT(GPIOC, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

  /* Configure GPIO pins : PE0 PE1 PE2 PE3 PE4 PE5 */
  GPIO_SET_MODE_IN_UP(gpio, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5, GPIO_Speed_2MHz);
  GPIO_Init (GPIOE, &gpio);

  /* Configure GPIO pins : PC0 PC1 PC2 PC3 PC4 PC5 PC6 PC7 */
  GPIO_SET_MODE_OUT_OD(gpio, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7, GPIO_Speed_2MHz);
  GPIO_Init (GPIOC, &gpio);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd_set_row () - set keyboard address row
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
zxkbd_set_row (uint8_t addr)
{
    GPIOC->BSRRH = 0x00FF;                                  // reset lower 8 bits
    GPIOC->BSRRL = addr;                                    // set lower 8 bits corresponding to addr
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd_get_col () - get keyboard data column
 *
 * This function should be called after zxkbd_set_row() with a delay of minimum 15usec
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
zxkbd_get_col (void)
{
    uint8_t key;

    key = GPIOE->IDR;                                       // read lower 8 bits of port E
    return key & ZX_KBD_EXT_COLMASK;                        // return lower 6 bits (5 cols + 1 extra col)
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd_poll () - poll zx keyboard
 *
 * This function is designed as a state machine and is only used in Z80 emulator - not in STECCY menu.
 * Pause between two calls should be minimal 15usec to get stable signals.
 *
 * See also: z80.c
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ZXKBD_STATE_SET_ROW     0
#define ZXKBD_STATE_GET_COL     1
void
zxkbd_poll (void)
{
    static uint_fast8_t state   = ZXKBD_STATE_SET_ROW;
    static uint_fast8_t row     = 0;
    uint8_t             addr;
    uint8_t             col;

    switch (state)
    {
        case ZXKBD_STATE_SET_ROW:
        {
            addr = ~(1 << row);
            zxkbd_set_row (addr);
            state = ZXKBD_STATE_GET_COL;
            break;
        }
        case ZXKBD_STATE_GET_COL:
        {
            col = zxkbd_get_col ();

            if (! (col & 0x20))                                     // bit 5 active (low)?
            {
                z80_leave_focus ();                                 // menu key: PCx connected with PE5
            }

            col |= ~(ZX_KBD_COLMASK);                               // set upper 3 bits
            zxkbd_matrix[row] = col;

            row++;

            if (row == ZX_KBD_ROWS)
            {
                row = 0;
            }

            state = ZXKBD_STATE_SET_ROW;
            break;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkey_changed () - check if key changed
 *
 * This function is used only by STECCY menu system - not in Z80 emulator
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
zxkey_changed (uint_fast8_t row, uint_fast8_t colmask)
{
    uint_fast8_t    changed = 0;

    if ((zxkbd_inv_matrix[row] & colmask) != (last_zxkbd_inv_matrix[row] & colmask))
    {
        changed = 1;
    }

    return changed;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * key_pressed () - check if key pressed
 *
 * This function is used only by STECCY menu system - not in Z80 emulator
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
zxkey_pressed (uint_fast8_t row, uint_fast8_t colmask)
{
    uint_fast8_t    pressed = 0;

    if (zxkbd_inv_matrix[row] & colmask)
    {
        pressed = 1;
    }

    return pressed;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd_emulate_ps2keys () - emulate PS/2 keyboard scancodes
 *
 * This function is used only by STECCY menu system - not in Z80 emulator
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
zxkbd_emulate_ps2keys (void)
{
    uint_fast16_t   scancode = 0;
    uint_fast8_t    row;
    uint_fast8_t    col;
    uint8_t         addr;
    uint_fast8_t    cnt;

    for (row = 0; row < ZX_KBD_ROWS; row++)
    {
        addr = ~(1 << row);
        zxkbd_set_row (addr);                                                                   // set address row
        delay_usec (15);                                                                        // wait until signal is stable
        zxkbd_inv_matrix[row] = (~zxkbd_get_col ()) & ZX_KBD_COLMASK;                           // inverse: 1 = pressed, 0 = released

        for (cnt = 0; cnt < 4; cnt++)                                                           // debounce: 8 x 4 msec = 32 msec
        {
            delay_msec (1);

            if (z80_settings.keyboard & KEYBOARD_USB)                                           // call USB statemachine during wait
            {
                usb_hid_host_process (TRUE);
            }
        }
    }

    if (zxkey_pressed (ROW_SHIFT, MASK_SHIFT))                                                  // Caps Shift pressed
    {
        if (zxkey_changed (ROW_D_ARROW, MASK_D_ARROW))                                          // Caps Shift + 6 = Down Arrow
        {
            scancode = SCANCODE_D_ARROW;

            if (! zxkey_pressed (ROW_D_ARROW, MASK_D_ARROW))
            {
                scancode |= PS2KEY_RELEASED_FLAG;
            }
        }
        else if (zxkey_changed (ROW_U_ARROW, MASK_U_ARROW))                                     // Caps Shift + 7 = Up Arrow
        {
            scancode = SCANCODE_U_ARROW;

            if (! zxkey_pressed (ROW_U_ARROW, MASK_U_ARROW))
            {
                scancode |= PS2KEY_RELEASED_FLAG;
            }
        }
        else if (zxkey_changed (ROW_BSP, MASK_BSP))                                             // Caps Shift + 0 = Backspace
        {
            scancode = SCANCODE_BSP;

            if (! zxkey_pressed (ROW_BSP, MASK_BSP))
            {
                scancode |= PS2KEY_RELEASED_FLAG;
            }
        }
        else if (zxkey_changed (ROW_ESC, MASK_ESC))                                             // Caps Shift + Space = Break (ESC)
        {
            scancode = SCANCODE_ESC;

            if (! zxkey_pressed (ROW_ESC, MASK_ESC))
            {
                scancode |= PS2KEY_RELEASED_FLAG;
            }
        }
    }
    else if (zxkey_pressed (ROW_SYM_SHIFT, MASK_SYM_SHIFT))                                     // Symbol Shift pressed
    {
        if (zxkey_changed (ROW_MINUS, MASK_MINUS))                                              // Symbol Shift + J = Minus
        {
            scancode = SCANCODE_MINUS;

            if (! zxkey_pressed (ROW_MINUS, MASK_MINUS))
            {
                scancode |= PS2KEY_RELEASED_FLAG;
            }
        }
    }
    else
    {
        for (row = 0; row < ZX_KBD_ROWS; row++)
        {
            if (zxkbd_inv_matrix[row] != 0)
            {
                for (col = 0; col < ZX_KBD_COLS; col++)
                {
                    if (zxkey_changed (row, (1 << col)))
                    {
                        scancode = scancodes[row][col];

                        if (scancode)
                        {
                            if (! zxkey_pressed (row, (1 << col)))
                            {
                                scancode |= PS2KEY_RELEASED_FLAG;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    if (scancode)
    {
        // printf ("zx scancode: %04x\n", scancode);
        ps2key_setscancode (scancode);
    }

    memcpy (last_zxkbd_inv_matrix, zxkbd_inv_matrix, sizeof (zxkbd_inv_matrix));
}
