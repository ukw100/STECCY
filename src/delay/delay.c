/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay.c - delay functions
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
#include <stdint.h>
#include "delay.h"

static uint_fast8_t         resolution  = DELAY_DEFAULT_RESOLUTION;             // resolution in usec, see delay.h for default
static uint32_t             msec_factor = 1000 / DELAY_DEFAULT_RESOLUTION;      // factor for msec delays

volatile uint32_t           delay_counter;                                      // counts down in units of resolution, see above

void SysTick_Handler(void);                                                     // keep compiler happy

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SysTick_Handler() - decrement delay_counter
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
SysTick_Handler(void)
{
    if (delay_counter > 0)
    {
        delay_counter--;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay_usec() - delay n microseconds (usec)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
delay_usec (uint32_t usec)
{
    if (resolution == DELAY_RESOLUTION_1_US)
    {
        delay_counter = usec + 1;                                                // +1: next tick can occur within 0..1 usec
    }
    else
    {
        delay_counter = usec / resolution + 1;                                   // see above
    }

    while (delay_counter != 0)
    {
        ;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay_msec() - delay n milliseconds (msec)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
delay_msec (uint32_t msec)
{
    delay_counter = msec * msec_factor;

    while (delay_counter != 0)
    {
        ;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay_msec() - delay n seconds (sec)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
delay_sec (uint32_t sec)
{
    while (sec--)
    {
        delay_msec (1000);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * delay_init() - init delay functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
delay_init (uint_fast8_t res)
{
    uint32_t    divider;

    if (res == 0)
    {
        res = DELAY_RESOLUTION_100_US;                          // default is resolution of 100us
    }

    divider         = 1000000 / res;
    msec_factor     = 1000 / res;

    SysTick_Config (SystemCoreClock / divider);
    resolution = res;
}
