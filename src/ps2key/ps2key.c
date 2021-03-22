/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key.c - handle PS/2 AT keyboard events
 *-------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Possible pins for external interrupt:
 *
 *   EXT_INT5: PA5, PB5, PC5, PD5, PE5, PF5, PG5, PH5, PI5
 *   EXT_INT6: PA6, PB6, PC6, PD6, PE6, PF6, PG6, PH6, PI6
 *   EXT_INT7: PA7, PB7, PC7, PD7, PE7, PF7, PG7, PH7, PI7
 *   EXT_INT8: PA8, PB8, PC8, PD8, PE8, PF8, PG8, PH8, PI8
 *   EXT_INT9: PA9, PB9, PC9, PD9, PE9, PF9, PG9, PH9, PI9
 *
 * Scancode examples:
 *
 *   Key            Makecode    Breakcode
 *   --------------------------------------------
 *   A              0x1C        0xF0 0x1C
 *   5              0x2Ee       0xF0 0x2E
 *   F10            0x09        0xf0 0x09
 *   Cursor right   0xE0 0x74   0xE0 0xF0 0x74
 *   right Ctrl     0xE0 0x14   0xE0 0xF0 0x14
 *
 *
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
#include "ps2key.h"
#include "zxkbd.h"
#include "keypress.h"
#include "stm32f4xx.h"
#include "z80.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * data pin PS2 = PB7
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define PS2KEY_CLOCK_PIN                GPIO_Pin_7
#define PS2KEY_CLOCK_GPIO_PORT          GPIOB
#define PS2KEY_CLOCK_GPIO_CLK           RCC_AHB1Periph_GPIOB
#define PS2KEY_CLOCK_EXTI_LINE          EXTI_Line7
#define PS2KEY_CLOCK_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOB
#define PS2KEY_CLOCK_EXTI_PIN_SOURCE    EXTI_PinSource7

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * data pin PS2 = PB6
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define PS2KEY_DATA_PIN                 GPIO_Pin_6
#define PS2KEY_DATA_GPIO_PORT           GPIOB
#define PS2KEY_DATA_GPIO_CLK            RCC_AHB1Periph_GPIOB

#define PS2KEY_FRAME_LEN                11

#define PS2KEY_CMD_EXTEND               0xE0
#define PS2KEY_CMD_BREAK                0xF0
#define PS2KEY_CMD_SHIFT                0x12

static uint_fast8_t                     ps2key_mode     = PS2KEY_CALLBACK_MODE;
uint_fast16_t                           ps2key_scancode = 0x0000;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extint_5to9_init () - initialize external interrupt
 *
 * PB7 = Clock
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
extint_5to9_init (void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;                                     // GPIO_PuPd_UP, better: 4,7k external pullup
    GPIO_InitStructure.GPIO_Pin     = PS2KEY_CLOCK_PIN;
    GPIO_Init (PS2KEY_CLOCK_GPIO_PORT, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    SYSCFG_EXTILineConfig(PS2KEY_CLOCK_EXTI_PORT_SOURCE, PS2KEY_CLOCK_EXTI_PIN_SOURCE);     // connect EXT_INT7 with pin B7

    EXTI_InitStructure.EXTI_Line    = PS2KEY_CLOCK_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority           = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelCmd                   = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_setmode () - set mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ps2key_setmode (uint_fast8_t mode)
{
    ps2key_mode = mode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_getscancode () - get last scancode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast16_t
ps2key_getscancode (void)
{
    uint_fast16_t   rtc;

    if (z80_settings.keyboard & KEYBOARD_ZX)
    {
        zxkbd_emulate_ps2keys ();
    }

    rtc = ps2key_scancode;
    ps2key_scancode = 0x0000;
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_setscancode () - set 16 bit scancode incl. flags (e.g. PS2KEY_RELEASED_FLAG)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ps2key_setscancode (uint16_t scancode)
{
    ps2key_scancode = scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_set_scancode8 () - set scancode (called from ISR)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ps2key_set_scancode8 (uint8_t scancode)
{
    static uint_fast16_t flag = 0x0000;

    if (scancode == PS2KEY_CMD_BREAK)
    {
        flag |= PS2KEY_RELEASED_FLAG;
    }
    else if (scancode == PS2KEY_CMD_EXTEND)
    {
        flag |= PS2KEY_EXTENDED_FLAG;
    }
    else
    {
        static uint16_t     last_complete_scancode = 0x0000;
        uint16_t            complete_scancode = scancode | flag;

        if (last_complete_scancode != complete_scancode)
        {
            last_complete_scancode = complete_scancode;                                     // ignore keyboard repetitions
            // printf ("scan: %04x\r\n", complete_scancode);
            // fflush (stdout);

            if (ps2key_mode == PS2KEY_CALLBACK_MODE)
            {
                keypress (complete_scancode);
            }
            else
            {
                ps2key_setscancode (complete_scancode);
            }
        }
        flag = 0x0000;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_isr () - ISR
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ps2key_isr (void)
{
    static uint8_t  bitpos      = 0;
    static uint8_t  scancode    = 0;
    uint8_t         value;

    value = GPIO_ReadInputDataBit(PS2KEY_DATA_GPIO_PORT, PS2KEY_DATA_PIN);

    if ((bitpos > 0) && (bitpos < 9))
    {
        scancode >>= 1;

        if (value == Bit_SET)
        {
            scancode |= 0x80;
        }
    }

    bitpos++;

    if (bitpos >= PS2KEY_FRAME_LEN)
    {
        bitpos = 0;
        ps2key_set_scancode8 (scancode);
        scancode = 0;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EXTI9_5_IRQHandler() - IRQ handler
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
EXTI9_5_IRQHandler(void)
{
    uint8_t value;

    if(EXTI_GetITStatus(PS2KEY_CLOCK_EXTI_LINE) != RESET)
    {
        EXTI_ClearITPendingBit(PS2KEY_CLOCK_EXTI_LINE);

        value = GPIO_ReadInputDataBit(PS2KEY_CLOCK_GPIO_PORT, PS2KEY_CLOCK_PIN);

        if (value == Bit_RESET)
        {
            ps2key_isr ();
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ps2key_init () - initialize PS2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ps2key_init (void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    extint_5to9_init ();

    RCC_AHB1PeriphClockCmd(PS2KEY_DATA_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;                                   // GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin   = PS2KEY_DATA_PIN;
    GPIO_Init(PS2KEY_DATA_GPIO_PORT, &GPIO_InitStructure);
}
