/*-------------------------------------------------------------------------------------------------------------------------------------------
 * board-led.c - LED routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2014-2021 Frank Meyer - frank(at)fli4l.de
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
#include "board-led.h"
#include "io.h"

#define BOARD_LED_PERIPH_CLOCK_CMD   RCC_AHB1PeriphClockCmd
#define BOARD_LED_PERIPH             RCC_AHB1Periph_GPIOA
#define BOARD_LED_PORT               GPIOA

#define BOARD_LED2_LED               GPIO_Pin_6
#define BOARD_LED2_ON                GPIO_RESET_BIT(BOARD_LED_PORT, BOARD_LED2_LED)               // active low
#define BOARD_LED2_OFF               GPIO_SET_BIT(BOARD_LED_PORT, BOARD_LED2_LED)

#define BOARD_LED3_LED               GPIO_Pin_7
#define BOARD_LED3_ON                GPIO_RESET_BIT(BOARD_LED_PORT, BOARD_LED3_LED)               // active low
#define BOARD_LED3_OFF               GPIO_SET_BIT(BOARD_LED_PORT, BOARD_LED3_LED)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize LED port
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led_init (void)
{
    GPIO_InitTypeDef gpio;

    GPIO_StructInit (&gpio);
    BOARD_LED_PERIPH_CLOCK_CMD (BOARD_LED_PERIPH, ENABLE);     // enable clock for LED Port
    GPIO_SET_MODE_OUT_PP(gpio, BOARD_LED2_LED | BOARD_LED3_LED, GPIO_Speed_2MHz);
    GPIO_Init(BOARD_LED_PORT, &gpio);
    board_led2_off ();
    board_led3_off ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED2 on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led2_on (void)
{
    BOARD_LED2_ON;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED2 off
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led2_off (void)
{
    BOARD_LED2_OFF;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED3 on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led3_on (void)
{
    BOARD_LED3_ON;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LED3 off
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
board_led3_off (void)
{
    BOARD_LED3_OFF;
}
