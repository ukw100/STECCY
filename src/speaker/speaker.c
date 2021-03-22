/*-------------------------------------------------------------------------------------------------------------------------------------------
 * speaker.c - ZX speaker routines
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
#include "speaker.h"
#include "io.h"

#define SPEAKER_PERIPH_CLOCK_CMD    RCC_AHB1PeriphClockCmd
#define SPEAKER_PERIPH              RCC_AHB1Periph_GPIOC
#define SPEAKER_PORT                GPIOC

#define SPEAKER_PIN                 GPIO_Pin_13
#define SPEAKER_HIGH                GPIO_SET_BIT(SPEAKER_PORT, SPEAKER_PIN)
#define SPEAKER_LOW                 GPIO_RESET_BIT(SPEAKER_PORT, SPEAKER_PIN)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize speaker port
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
speaker_init (void)
{
    GPIO_InitTypeDef gpio;

    GPIO_StructInit (&gpio);
    SPEAKER_PERIPH_CLOCK_CMD (SPEAKER_PERIPH, ENABLE);

    gpio.GPIO_Pin   = SPEAKER_PIN;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

#if defined (STM32F10X)
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
#elif defined (STM32F4XX)
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
#endif

    GPIO_Init(SPEAKER_PORT, &gpio);
    speaker_low ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * speaker high
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
speaker_high (void)
{
    SPEAKER_HIGH;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * speaker low
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
speaker_low (void)
{
    SPEAKER_LOW;
}
