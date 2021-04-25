/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main.c - main module of zx-spectrum emulator STECCY
 *-------------------------------------------------------------------------------------------------------------------------------------------
 *
 *  System Clocks configured on STM32F407VE Black Board Board (see changes in system_stm32f4xx.c):
 *
 *      HSE_VALUE           8MHz, external 8MHz crystal
 *      PLL M               4
 *      PLL N               168
 *      PLL P               2
 *      AHB prescaler       1
 *      APB1 prescaler      4
 *      APB2 prescaler      2
 *
 *      Main clock:         168 MHz ((HSE_VALUE / PLL_M) * PLL_N) / PLL_P = ((8MHz / 4) * 168) / 2 = 168MHz
 *      AHB clock:          168 MHz (AHB  Prescaler = 1)
 *      APB1 clock:          42 MHz (APB1 Prescaler = 4)
 *      APB2 clock:          84 MHz (APB2 Prescaler = 2)
 *      Timer clock:        168 MHz
 *
 * Internal devices used on STM32F407VE BlackBoard:
 *
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Device                  | STM32F407VET6                                                             | Remarks                       |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Button          "K0"    | GPIO           PE4                                                        | not used                      |
 *    | Button          "K1"    | GPIO           PE3                                                        | not used                      |
 *    | Button          "WK_UP" | GPIO           PA0                                                        | not used                      |
 *    | Board LED       D2      | GPIO           PA6                                                        | not used                      |
 *    | Board LED       D3      | GPIO           PA7                                                        | not used                      |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *
 * External devices STM32F407VE BlackBoard with TFT - sorted by function:
 *
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Device                  | STM32F407VE BlackBoard                                                    | Remarks                       |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Console         USART   | USART2 ALT0    TX=PA2  RX=PA3                                             |                               |
 *    | ESP8266         RX      | USART3 ALT0    TX=PB10                                                    |                               |
 *    | ESP8266         TX      | USART3 ALT0    RX=PB11                                                    |                               |
 *    | ESP8266         RST     | GPIO           RST=PA4                                                    |                               |
 *    | ESP8266         CH_PD   | GPIO           CH_PD=PA5                                                  |                               |
 *    | ESP8266         GPIO0   | FLASH          PA8                                                        |                               |
 *    | ESP8266         GPIO13  | USART1         GPIO13=PA9                                                 | Only in STM32 bootloader mode |
 *    | ESP8266         GPIO15  | USART1         GPIO15=PA10                                                | Only in STM32 bootloader mode |
 *    | ESP8266         GPIO14  | RESET          GPIO14=RESET                                               |                               |
 *    | ESP8266         GPIO4   | BOOT0          GPIO4=BOOT0                                                |                               |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | SPI FLASH               |                                                                           |                               |
 *    | Flash W25Q16            | F_CS           PB0                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 CLK       PB3                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 MISO      PB4                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 MOSI      PB5                                                        | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | I2C                     |                                                                           |                               |
 *    | Port Expander   SCL     | I2C1 SCL       PB8                                                        | used for PCF8574/PCF8574A     |
 *    | Port Expander   SDA     | I2C1 SDA       PB9                                                        | used for PCF8574/PCF8574A     |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | FSMC            TFT     |                                                                           |                               |
 *    | FSMC RST        RS      | FSMC RST       STM32 RESET                                                | fix on BlackBoard             |
 *    | FSMC D15        DB15    | FSMC D15       PD10                                                       | fix on BlackBoard             |
 *    | FSMC D14        DB14    | FSMC D14       PD9                                                        | fix on BlackBoard             |
 *    | FSMC D13        DB13    | FSMC D13       PD8                                                        | fix on BlackBoard             |
 *    | FSMC D12        DB12    | FSMC D12       PE15                                                       | fix on BlackBoard             |
 *    | FSMC D11        DB11    | FSMC D11       PE14                                                       | fix on BlackBoard             |
 *    | FSMC D10        DB10    | FSMC D10       PE13                                                       | fix on BlackBoard             |
 *    | FSMC D9         DB9     | FSMC D9        PE12                                                       | fix on BlackBoard             |
 *    | FSMC D8         DB8     | FSMC D8        PE11                                                       | fix on BlackBoard             |
 *    | FSMC D7         DB7     | FSMC D7        PE10                                                       | fix on BlackBoard             |
 *    | FSMC D6         DB6     | FSMC D6        PE9                                                        | fix on BlackBoard             |
 *    | FSMC D5         DB5     | FSMC D5        PE8                                                        | fix on BlackBoard             |
 *    | FSMC D4         DB4     | FSMC D4        PE7                                                        | fix on BlackBoard             |
 *    | FSMC D3         DB3     | FSMC D3        PD1                                                        | fix on BlackBoard             |
 *    | FSMC D2         DB2     | FSMC D2        PD0                                                        | fix on BlackBoard             |
 *    | FSMC D1         DB1     | FSMC D1        PD15                                                       | fix on BlackBoard             |
 *    | FSMC D0         DB0     | FSMC D0        PD14                                                       | fix on BlackBoard             |
 *    | FSMC NOE        RD      | FSMC NOE       PD4                                                        | fix on BlackBoard             |
 *    | FSMC NWE        WR      | FSMC NWE       PD5                                                        | fix on BlackBoard             |
 *    | FSMC A18        RS      | FSMC A18       PD13                                                       | fix on BlackBoard             |
 *    | FSMC NE1        CS      | FSMC NE1       PD7                                                        | fix on BlackBoard             |
 *    | LCD BL                  | LCD            PB1 (not used)                                             | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | TOUCH           TFT     |                                                                           |                               |
 *    | TOUCH           T_CS    | T_CS           PB12                                                       | fix on BlackBoard             |
 *    | TOUCH           T_SCK   | T_SCK          PB13                                                       | fix on BlackBoard             |
 *    | TOUCH           T_MISO  | T_MISO         PB14                                                       | fix on BlackBoard             |
 *    | TOUCH           T_MOSI  | T_MOSI         PB15                                                       | fix on BlackBoard             |
 *    | TOUCH           T_PEN   | T_PEN          PC5                                                        | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *
 * List of STM32F407VE BlackBoard pins - sorted by Port/Pin:
 *
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Reserved        WK_UP   | WK_UP button   PA0                                                        | fix on BlackBoard             |
 *    | Free                    | GPIO           PA1                                                        |                               |
 *    | Console         RX      | USART2 TX      PA2                                                        |                               |
 *    | Console         TX      | USART2 RX      PA3                                                        |                               |
 *    | ESP8266         RST     | GPIO           PA4                                                        |                               |
 *    | ESP8266         CH_PD   | GPIO           PA5                                                        |                               |
 *    | Board LED       D2      | Board LED D2   PA6                                                        | fix on BlackBoard             |
 *    | Board LED       D3      | Board LED D3   PA7                                                        | fix on BlackBoard             |
 *    | ESP8266         GPIO0   | FLASH          PA8                                                        |                               |
 *    | ESP8266         GPIO13  | USART1 TX      PA9                                                        | Only in STM32 bootloader mode |
 *    | ESP8266         GPIO15  | USART1 RX      PA10                                                       | Only in STM32 bootloader mode |
 *    | Reserved        USB DM  | USB DM -       PA11                                                       | fix on BlackBoard             |
 *    | Reserved        USB DP  | USB DP +       PA12                                                       | fix on BlackBoard             |
 *    | Reserved        JTAG    | JTAG TMS       PA13                                                       | fix on BlackBoard             |
 *    | Reserved        JTAG    | JTAG TCK       PA14                                                       | fix on BlackBoard             |
 *    | Reserved        JTAG    | JTAG TDI       PA15                                                       | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | Flash W25Q16            | F_CS           PB0                                                        | fix on BlackBoard             |
 *    | LCD             BL      | LCD BL         PB1 (not used)                                             | fix on BlackBoard             |
 *    | Reserved                | BOOT1          PB2                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 CLK       PB3                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 MISO      PB4                                                        | fix on BlackBoard             |
 *    | Flash W25Q16            | SPI1 MOSI      PB5                                                        | fix on BlackBoard             |
 *    | PS/2 keyboard   Data    | PS/2 Data      PB6                                                        |                               |
 *    | PS/2 keyboard   Clock   | PS/2 Clock     PB7                                                        |                               |
 *    | Port Expander   SCL     | I2C1 SCL       PB8                                                        | used for PCF8574/PCF8574A     |
 *    | Port Expander   SDA     | I2C1 SDA       PB9                                                        | used for PCF8574/PCF8574A     |
 *    | ESP8266 USART   TX      | USART3 TX      PB10                                                       |                               |
 *    | ESP8266 USART   RX      | USART3 RX      PB11                                                       |                               |
 *    | TOUCH           T_CS    | T_CS           PB12                                                       | Touch (yet not used)          |
 *    | TOUCH           T_SCK   | T_SCK          PB13                                                       | Touch (yet not used)          |
 *    | TOUCH           T_MISO  | T_MISO         PB14                                                       | Touch (yet not used)          |
 *    | TOUCH           T_MOSI  | T_MOSI         PB15                                                       | Touch (yet not used)          |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | ZX keyboard row 0       | GPIO           PC0                                                        |                               |
 *    | ZX keyboard row 1       | GPIO           PC1                                                        |                               |
 *    | ZX keyboard row 2       | GPIO           PC2                                                        |                               |
 *    | ZX keyboard row 3       | GPIO           PC3                                                        |                               |
 *    | ZX keyboard row 4       | GPIO           PC4                                                        |                               |
 *    | ZX keyboard row 5       | GPIO           PC5                                                        | also T_PEN for touch          |
 *    | ZX keyboard row 6       | GPIO           PC6                                                        |                               |
 *    | ZX keyboard row 7       | GPIO           PC7                                                        |                               |
 *    | Reserved                | SDIO D0        PC8                                                        | fix on BlackBoard             |
 *    | Reserved                | SDIO D1        PC9                                                        | fix on BlackBoard             |
 *    | Reserved                | SDIO D2        PC10                                                       | fix on BlackBoard             |
 *    | Reserved                | SDIO D3        PC11                                                       | fix on BlackBoard             |
 *    | Reserved                | SDIO SCK       PC12                                                       | fix on BlackBoard             |
 *    | ZX Speaker              | GPIO           PC13                                                       |                               |
 *    | Reserved                | XTAL           PC14                                                       | fix on BlackBoard             |
 *    | Reserved                | XTAL           PC15                                                       | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | FSMC D2         DB2     | FSMC D2        PD0                                                        | fix on BlackBoard             |
 *    | FSMC D3         DB3     | FSMC D3        PD1                                                        | fix on BlackBoard             |
 *    | Reserved                | SDIO CMD       PD2                                                        | fix on BlackBoard             |
 *    | Free                    | FSMC CLK       PD3                                                        |                               |
 *    | FSMC NOE        RD      | FSMC NOE       PD4                                                        | fix on BlackBoard             |
 *    | FSMC NWE        WR      | FSMC NWE       PD5                                                        | fix on BlackBoard             |
 *    | FSMC NWAIT              | FSMC NWAIT     PD6                                                        | fix on BlackBoard, not used   |
 *    | FSMC NE1        CS      | FSMC NE1       PD7                                                        | fix on BlackBoard             |
 *    | FSMC D13        DB13    | FSMC D13       PD8                                                        | fix on BlackBoard             |
 *    | FSMC D14        DB14    | FSMC D14       PD9                                                        | fix on BlackBoard             |
 *    | FSMC D15        DB15    | FSMC D15       PD10                                                       | fix on BlackBoard             |
 *    | Free                    | FSMC A16       PD11                                                       |                               |
 *    | Free                    | FSMC A17       PD12                                                       |                               |
 *    | Free                    | FSMC A18       PD13                                                       | fix on BlackBoard             |
 *    | FSMC D0         DB0     | FSMC D0        PD14                                                       | fix on BlackBoard             |
 *    | FSMC D1         DB1     | FSMC D1        PD15                                                       | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *    | ZX keyboard column 0    | GPIO           PE0                                                        |                               |
 *    | ZX keyboard column 1    | GPIO           PE1                                                        |                               |
 *    | ZX keyboard column 2    | GPIO           PE2                                                        |                               |
 *    | ZX keyboard column 3    | GPIO           PE3                                                        | also "K1", floating           |
 *    | ZX keyboard column 4    | GPIO           PE4                                                        | also "K0", floating           |
 *    | ZX extra    column 5    | GPIO           PE5                                                        | menu key                      |
 *    | Free                    |                PE6                                                        |                               |
 *    | FSMC D4         DB4     | FSMC D4        PE7                                                        | fix on BlackBoard             |
 *    | FSMC D5         DB5     | FSMC D5        PE8                                                        | fix on BlackBoard             |
 *    | FSMC D6         DB6     | FSMC D6        PE9                                                        | fix on BlackBoard             |
 *    | FSMC D7         DB7     | FSMC D7        PE10                                                       | fix on BlackBoard             |
 *    | FSMC D8         DB8     | FSMC D8        PE11                                                       | fix on BlackBoard             |
 *    | FSMC D9         DB9     | FSMC D9        PE12                                                       | fix on BlackBoard             |
 *    | FSMC D10        DB10    | FSMC D10       PE13                                                       | fix on BlackBoard             |
 *    | FSMC D11        DB11    | FSMC D11       PE14                                                       | fix on BlackBoard             |
 *    | FSMC D12        DB12    | FSMC D12       PE15                                                       | fix on BlackBoard             |
 *    +-------------------------+---------------------------------------------------------------------------+-------------------------------+
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2019-2021 Frank Meyer - frank(at)uclock.de
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
#include "stm32f4xx_conf.h"
#include "stm32_sdcard.h"
#include "z80.h"
#include "delay.h"
#include "board-led.h"
#include "speaker.h"
#include "i2c1-dma.h"
#include "wii-gamepad.h"
#include "joystick.h"
#include "tft.h"
#include "console.h"
#include "fs-stdio.h"
#include "menu.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * timer definitions:
 *
 *      F_INTERRUPTS    = TIM_CLK / (TIM_PRESCALER + 1) / (TIM_PERIOD + 1)
 * <==> TIM_PRESCALER   = TIM_CLK / F_INTERRUPTS / (TIM_PERIOD + 1) - 1
 *
 * STM32F407VE with 10000 interrupts per second:
 *      TIM_PERIOD      =    8 - 1 =    7
 *      TIM_PRESCALER   = 1050 - 1 = 1049
 *      F_INTERRUPTS    = 84000000 / 1050 / 8 = 10000 (0.00% error)
 *
 * STM32F407VE with 50000 interrupts per second:
 *      TIM_PERIOD      =    8 - 1 =    7
 *      TIM_PRESCALER   =  210 - 1 =  209
 *      F_INTERRUPTS    = 84000000 / 210 / 8 = 50000 (0.00% error)
 *
 * STM32F407VE with 100000 interrupts per second:
 *      TIM_PERIOD      =    8 - 1 =    7
 *      TIM_PRESCALER   =  105 - 1 =  104
 *      F_INTERRUPTS    = 84000000 / 105 / 8 = 100000 (0.00% error)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined (STM32F407)                                                     // STM32F407VE Black Board @168MHz
#define TIM_CLK                 84000000L                                   // APB2 clock: 84 MHz
#define TIM_PERIOD              7
#else
#error STM32 unknown
#endif

#define F_INTERRUPTS            50000                                       // 50000 interrupts per second: every 20 usec
#define TIM_PRESCALER           ((TIM_CLK / F_INTERRUPTS) / (TIM_PERIOD + 1) - 1)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize timer2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
timer2_init (void)
{
    TIM_TimeBaseInitTypeDef     tim;
    NVIC_InitTypeDef            nvic;

    TIM_TimeBaseStructInit (&tim);
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2, ENABLE);

    tim.TIM_ClockDivision   = TIM_CKD_DIV1;
    tim.TIM_CounterMode     = TIM_CounterMode_Up;
    tim.TIM_Period          = TIM_PERIOD;
    tim.TIM_Prescaler       = TIM_PRESCALER;
    TIM_TimeBaseInit (TIM2, &tim);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    nvic.NVIC_IRQChannel                    = TIM2_IRQn;
    nvic.NVIC_IRQChannelCmd                 = ENABLE;
    nvic.NVIC_IRQChannelPreemptionPriority  = 0x0F;
    nvic.NVIC_IRQChannelSubPriority         = 0x0F;
    NVIC_Init (&nvic);

    TIM_Cmd(TIM2, ENABLE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * timer2 IRQ handler - called every 20 usec
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
extern void TIM2_IRQHandler (void);                                                         // keep compiler happy

void
TIM2_IRQHandler (void)
{
    static uint32_t cnt;
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    uptime++;
    cnt++;

    if (cnt == 1000)                                                                        // 20 msec = 50 Hz
    {
        cnt = 0;
        update_display = 1;
    }
}


static int
do_mount (void)
{
    static uint_fast8_t     mounted;                                                        // flag: already mounted
    static FATFS            fs;                                                             // must be static!
    FRESULT                 res;
    uint_fast8_t            cnt = 0;
    int                     rtc;

    if (mounted)
    {
        printf ("SD card already mounted\n");
        rtc = EXIT_SUCCESS;
    }
    else
    {
        do
        {
            res = f_mount (&fs, "", 1);

            if (res == FR_NOT_READY)
            {
                delay_msec (10);
                cnt++;
            }
        } while (res == FR_NOT_READY && cnt < 10);

        if (res == FR_OK)
        {
            printf ("SD card mounted, retry count = %d\n", cnt);
            mounted = 1;
            rtc = EXIT_SUCCESS;
        }
        else
        {
            fs_perror ("mount", res);
            rtc = EXIT_FAILURE;
        }
    }
    return rtc;
}

int
main (void)
{
    SystemInit ();
    SystemCoreClockUpdate();                                                // needed for Nucleo board

#if defined (STM32F103)                                                     // disable JTAG to get back PB3, PB4, PA13, PA14, PA15
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);                    // turn on clock for the alternate function register
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);                // disable the JTAG, enable the SWJ interface
#endif

    delay_init (DELAY_RESOLUTION_5_US);                                     // initialize delay functions with granularity of 5 us
    timer2_init ();

    console_init (115200);

    sdcard_init ();
    console_puts ("\r\nWelcome to STECCY\r\n");

    // i2c_init (100000);                                                   // PCF8574 works only with 100kHz
    joystick_init ();                                                       // Wii Gamepad works with 400kHz

    if (joystick_is_online)
    {
        if (wii_type == WII_TYPE_GAMEPAD)
        {
            console_puts ("Detected Wii Gamepad\r\n");
        }
        else
        {
            console_puts ("Detected Wii Nunchuk\r\n");
        }
    }
    else
    {
        console_puts ("No Wii device detected\r\n");
    }

    do_mount ();

    board_led_init ();
    speaker_init ();
    tft_init ();
#if defined (SSD1963)
    ssd1963_set_flags (SSD1963_GLOBAL_FLAGS_RGB_ORDER);
#elif defined (ILI9341)
#endif
    tft_fill_screen (0);
    menu_init ();

    zx_spectrum ();
}
