/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341.c - ILI9341 routines
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
#if defined (ILI9341)

#include "tft.h"
#include "delay.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * level 1 command codes:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ILI9341_NOP                                             0x00
#define ILI9341_SOFT_RESET                                      0x01
#define ILI9341_READ_DISPLAY_IDENTIFICATION_INFO                0x04
#define ILI9341_READ_DISPLAY_STATUS                             0x09
#define ILI9341_READ_DISPLAY_POWER_MODE                         0x0A
#define ILI9341_READ_DISPLAY_MADCTL                             0x0B
#define ILI9341_READ_DISPLAY_PIXEL_FORMAT                       0x0C
#define ILI9341_READ_DISPLAY_IMAGE_FORMAT                       0x0D
#define ILI9341_READ_DISPLAY_SIGNAL_MODE                        0x0E
#define ILI9341_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT             0x0F
#define ILI9341_ENTER_SLEEP_MODE                                0x10
#define ILI9341_SLEEP_OUT                                       0x11
#define ILI9341_PARTIAL_MODE_ON                                 0x12
#define ILI9341_NORMAL_DISPLAY_MODE_ON                          0x13
#define ILI9341_DISPLAY_INVERSION_OFF                           0x20
#define ILI9341_DISPLAY_INVERSION_ON                            0x21
#define ILI9341_GAMMA_SET                                       0x26
#define ILI9341_DISPLAY_OFF                                     0x28
#define ILI9341_DISPLAY_ON                                      0x29
#define ILI9341_COLUMN_ADDRESS_SET                              0x2A
#define ILI9341_PAGE_ADDRESS_SET                                0x2B
#define ILI9341_MEMORY_WRITE                                    0x2C
#define ILI9341_COLOR_SET                                       0x2D
#define ILI9341_MEMORY_READ                                     0x2E
#define ILI9341_PARTIAL_AREA                                    0x30
#define ILI9341_VERTICAL_SCROLLING_DEFINITION                   0x33
#define ILI9341_TEARING_EFFECT_LINE_OFF                         0x34
#define ILI9341_TEARING_EFFECT_LINE_ON                          0x35
#define ILI9341_MEMORY_ACCESS_CONTROL                           0x36
#define ILI9341_VERTICAL_SCROLLING_START_ADDRESS                0x37
#define ILI9341_IDLE_MODE_OFF                                   0x38
#define ILI9341_IDLE_MODE_ON                                    0x39
#define ILI9341_COLMOD_PIXEL_FORMAT_SET                         0x3A
#define ILI9341_WRITE_MEMORY_CONTINUE                           0x3C
#define ILI9341_READ_MEMORY_CONTINUE                            0x3E
#define ILI9341_SET_TEAR_SCANLINE                               0x44
#define ILI9341_GET_SCANLINE                                    0x45
#define ILI9341_WRITE_DISPLAY_BRIGHTNESS                        0x51
#define ILI9341_READ_DISPLAY_BRIGHTNESS                         0x52
#define ILI9341_WRITE_CTRL_DISPLAY                              0x53
#define ILI9341_READ_CTRL_DISPLAY                               0x54
#define ILI9341_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL       0x55
#define ILI9341_READ_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL        0x56
#define ILI9341_WRITE_CABC_MINIMUM_BRIGHTNESS                   0x5E
#define ILI9341_READ_CABC_MINIMUM_BRIGHTNESS                    0x5F
#define ILI9341_READ_ID1                                        0xDA
#define ILI9341_READ_ID2                                        0xDB
#define ILI9341_READ_ID3                                        0xDC

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * level 2 command codes:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ILI9341_RGB_INTERFACE_SIGNAL_CONTROL                    0xB0
#define ILI9341_FRAME_RATE_CONTROL_IN_NORMAL_MODE_FULL_COLORS   0xB1
#define ILI9341_FRAME_RATE_CONTROL_IN_IDLE_MODE_8_COLORS        0xB2
#define ILI9341_FRAME_RATE_CONTROL_IN_PARTIAL_MODE_FULL_COLORS  0xB3
#define ILI9341_DISPLAY_INVERSION_CONTROL                       0xB4
#define ILI9341_BLANKING_PORCH_CONTROL                          0xB5
#define ILI9341_DISPLAY_FUNCTION_CONTROL                        0xB6
#define ILI9341_ENTRY_MODE_SET                                  0xB7
#define ILI9341_BACKLIGHT_CONTROL_1                             0xB8
#define ILI9341_BACKLIGHT_CONTROL_2                             0xB9
#define ILI9341_BACKLIGHT_CONTROL_3                             0xBA
#define ILI9341_BACKLIGHT_CONTROL_4                             0xBB
#define ILI9341_BACKLIGHT_CONTROL_5                             0xBC
#define ILI9341_BACKLIGHT_CONTROL_7                             0xBE
#define ILI9341_BACKLIGHT_CONTROL_8                             0xBF
#define ILI9341_POWER_CONTROL_1                                 0xC0
#define ILI9341_POWER_CONTROL_2                                 0xC1
#define ILI9341_VCOM_CONTROL_1                                  0xC5
#define ILI9341_VCOM_CONTROL_2                                  0xC7
#define ILI9341_POWERA                                          0xCB
#define ILI9341_POWERB                                          0xCF

#define ILI9341_NV_MEMORY_WRITE                                 0xD0
#define ILI9341_NV_MEMORY_PROTECTION_KEY                        0xD1
#define ILI9341_NV_MEMORY_STATUS_READ                           0xD2
#define ILI9341_READ_ID4                                        0xD3
#define ILI9341_POSITIVE_GAMMA_CORRECTION                       0xE0
#define ILI9341_NEGATIVE_GAMMA_CORRECTION                       0xE1
#define ILI9341_DIGITAL_GAMMA_CONTROL_1                         0xE2
#define ILI9341_DIGITAL_GAMMA_CONTROL_2                         0xE3
#define ILI9341_DRIVER_TIMING_CONTROL_A                         0xE8
#define ILI9341_DRIVER_TIMING_CONTROL_A2                        0xE9
#define ILI9341_DRIVER_TIMING_CONTROL_B                         0xEA
#define ILI9341_POWER_ON_SEQUENCE_CONTROL                       0xED
#define ILI9341_ENABLE_G3                                       0xF2
#define ILI9341_INTERFACE_CONTROL                               0xF6
#define ILI9341_PUMP_RATIO_CONTROL                              0xF7

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * configuration values
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
ILI9341_GLOBALS  ili9341;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_reset_ctrl_lines () - reset control lines, set them to input to allow auto-init
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_reset_ctrl_lines (void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    //Enable clocks
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Pin     =   GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15 |
                                        GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_13 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode    =   GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd    =   GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed   =   GPIO_Speed_100MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin     =   GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                        GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_init_ctrl_lines() - init control lines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_init_ctrl_lines (void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // --------------------------------------------------------------------------------------------------------------------------------------
    // Enable GPIOD, GPIOE, GPIOF, GPIOG and AFIO clocks
    // --------------------------------------------------------------------------------------------------------------------------------------
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF, ENABLE);

    // --------------------------------------------------------------------------------------------------------------------------------------
    // D0  = FSMC_D2
    // D1  = FSMC_D3
    // D8  = FSMC_D13
    // D9  = FSMC_D14
    // D10 = FSMC_D15
    // D14 = FSMC_D0
    // D15 = FSMC_D1
    // D4  = FSMC_NOE (/RD)
    // D5  = FSMC_NWE (/WR)
    // --------------------------------------------------------------------------------------------------------------------------------------

    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15 |
                                      GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource0,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource4,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource5,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource10,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource14,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource15,   GPIO_AF_FSMC);

    // --------------------------------------------------------------------------------------------------------------------------------------
    // E7  = FSMC_D4
    // E8  = FSMC_D5
    // E9  = FSMC_D63
    // E10 = FSMC_D7
    // E11 = FSMC_D8
    // E12 = FSMC_D9
    // E13 = FSMC_D10
    // E14 = FSMC_D11
    // E14 = FSMC_D12
    // --------------------------------------------------------------------------------------------------------------------------------------

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 |
                                  GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOE, GPIO_PinSource7,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource8,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9,    GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource10,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource11,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource12,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource13,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource14,   GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource15,   GPIO_AF_FSMC);

    // --------------------------------------------------------------------------------------------------------------------------------------
    // D13 = FSMC_A18 = /RS
    // --------------------------------------------------------------------------------------------------------------------------------------
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_FSMC);

    // --------------------------------------------------------------------------------------------------------------------------------------
    // D7 = FSMC_NE1 = /CS
    // --------------------------------------------------------------------------------------------------------------------------------------
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource7, GPIO_AF_FSMC);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_init_fsmc () - init FSMC
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_init_fsmc (void)
{
    FSMC_NORSRAMInitTypeDef             fsmc;
    FSMC_NORSRAMTimingInitTypeDef       ftime;

    RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);                                            // Enable FMC clock

    ftime.FSMC_AddressSetupTime         = 15;
    ftime.FSMC_AddressHoldTime          = 0;
    ftime.FSMC_DataSetupTime            = 13;
    ftime.FSMC_BusTurnAroundDuration    = 0;
    ftime.FSMC_CLKDivision              = 0;
    ftime.FSMC_DataLatency              = 0;
    ftime.FSMC_AccessMode               = FSMC_AccessMode_A;

    fsmc.FSMC_Bank                      = FSMC_Bank1_NORSRAM1;
    fsmc.FSMC_DataAddressMux            = FSMC_DataAddressMux_Disable;                              // Data/Address MUX     = Disable
    fsmc.FSMC_MemoryType                = FSMC_MemoryType_SRAM;                                     // Memory Type          = SRAM
    fsmc.FSMC_MemoryDataWidth           = FSMC_MemoryDataWidth_16b;                                 // Data Width           = 16bit
    fsmc.FSMC_BurstAccessMode           = FSMC_BurstAccessMode_Disable;
    fsmc.FSMC_AsynchronousWait          = FSMC_AsynchronousWait_Disable;                            // Asynchronous Wait    = Disable
    fsmc.FSMC_WaitSignalPolarity        = FSMC_WaitSignalPolarity_Low;
    fsmc.FSMC_WrapMode                  = FSMC_WrapMode_Disable;
    fsmc.FSMC_WaitSignalActive          = FSMC_WaitSignalActive_BeforeWaitState;
    fsmc.FSMC_WriteOperation            = FSMC_WriteOperation_Enable;                               // Write Operation      = Enable
    fsmc.FSMC_WaitSignal                = FSMC_WaitSignal_Disable;
    fsmc.FSMC_ExtendedMode              = FSMC_ExtendedMode_Disable;                                // Extended Mode        = Disable
    fsmc.FSMC_WriteBurst                = FSMC_WriteBurst_Disable;
    fsmc.FSMC_ReadWriteTimingStruct     = &ftime;
    fsmc.FSMC_WriteTimingStruct         = &ftime;

    FSMC_NORSRAMInit(&fsmc);
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_soft_reset () - software reset
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_soft_reset (void)
{
    ili9341_write_command (ILI9341_SOFT_RESET);
    delay_msec (10);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_set_column_address () - Set the column address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_set_column_address (uint_fast16_t start_column_number, uint_fast16_t end_column_number)
{
    ili9341_write_command (ILI9341_COLUMN_ADDRESS_SET);
    ili9341_write_data (start_column_number >> 8);
    ili9341_write_data (start_column_number & 0xFF);
    ili9341_write_data (end_column_number >> 8);
    ili9341_write_data (end_column_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_set_page_address () - Set the page address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_set_page_address (uint_fast16_t start_page_row_number, uint_fast16_t end_page_row_number)
{
    ili9341_write_command (ILI9341_PAGE_ADDRESS_SET);
    ili9341_write_data (start_page_row_number >> 8);
    ili9341_write_data (start_page_row_number & 0xFF);
    ili9341_write_data (end_page_row_number >> 8);
    ili9341_write_data (end_page_row_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_write_memory_start () - Transfer image information from the host processor interface
 * to the ILI9341 starting at the location provided by set_column_address and set_page_address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_write_memory_start (void)
{
    ili9341_write_command (ILI9341_MEMORY_WRITE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_read_memory_start () - Transfer image data from the ILI9341 to the host processor
 * interface starting at the location provided by set_column_address and set_page_address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_read_memory_start (void)
{
    ili9341_write_command (ILI9341_MEMORY_READ);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_powera ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_powera (void)
{
    ili9341_write_command(ILI9341_POWERA);
    ili9341_write_data(0x39);
    ili9341_write_data(0x2C);
    ili9341_write_data(0x00);
    ili9341_write_data(0x34);
    ili9341_write_data(0x02);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_powerb ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_powerb (void)
{
    ili9341_write_command(ILI9341_POWERB);
    ili9341_write_data(0x00);
    ili9341_write_data(0xC1);
    ili9341_write_data(0x30);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_driver_timing_control_a ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_driver_timing_control_a (void)
{
    ili9341_write_command(ILI9341_DRIVER_TIMING_CONTROL_A);
    ili9341_write_data(0x85);
    ili9341_write_data(0x00);
    ili9341_write_data(0x78);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_driver_timing_control_b ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_driver_timing_control_b (void)
{
    ili9341_write_command(ILI9341_DRIVER_TIMING_CONTROL_B);
    ili9341_write_data(0x00);
    ili9341_write_data(0x00);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_power_on_sequence_control ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_power_on_sequence_control (void)
{
    ili9341_write_command(ILI9341_POWER_ON_SEQUENCE_CONTROL);
    ili9341_write_data(0x64);
    ili9341_write_data(0x03);
    ili9341_write_data(0x12);
    ili9341_write_data(0x81);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_pump_ration_control ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_pump_ration_control (void)
{
    ili9341_write_command(ILI9341_PUMP_RATIO_CONTROL);
    ili9341_write_data(0x20);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_power_control_1 ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_power_control_1 (void)
{
    ili9341_write_command(ILI9341_POWER_CONTROL_1);
    ili9341_write_data(0x23);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_power_control_2 ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_power_control_2 (void)
{
    ili9341_write_command(ILI9341_POWER_CONTROL_2);
    ili9341_write_data(0x10);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_vcom_control_1 ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_vcom_control_1 (void)
{
    ili9341_write_command(ILI9341_VCOM_CONTROL_1);
    ili9341_write_data(0x3E);
    ili9341_write_data(0x28);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_vcom_control_2 ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_vcom_control_2 (void)
{
    ili9341_write_command(ILI9341_VCOM_CONTROL_2);
    ili9341_write_data(0x86);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_memory_access_control ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ILI9341_MADCTL_ORDER_RGB                0x00
#define ILI9341_MADCTL_ORDER_BGR                0x08

#define ILI9341_MADCTL_ROW_COL_EXCHANGE_ON      0x20
#define ILI9341_MADCTL_ROW_COL_EXCHANGE_OFF     0x00

#define ILI9341_MADCTL_FLIP_HORIZONTAL_ON       0x40
#define ILI9341_MADCTL_FLIP_HORIZONTAL_OFF      0x00

#define ILI9341_MADCTL_FLIP_VERTICAL_ON         0x80
#define ILI9341_MADCTL_FLIP_VERTICAL_OFF        0x00

static void
ili9341_memory_access_control (void)
{
    uint8_t    madctl = 0x00;

    if (ili9341.flags & ILI9341_GLOBAL_FLAGS_RGB_ORDER)
    {
        madctl |= ILI9341_MADCTL_ORDER_RGB;
    }
    else
    {
        madctl |= ILI9341_MADCTL_ORDER_BGR;
    }

    if (ili9341.flags & ILI9341_GLOBAL_FLAGS_ROW_COL_EXCHANGE)
    {
        madctl |= ILI9341_MADCTL_ROW_COL_EXCHANGE_ON;
    }
    else
    {
        madctl |= ILI9341_MADCTL_ROW_COL_EXCHANGE_OFF;
    }

    if (ili9341.flags & ILI9341_GLOBAL_FLAGS_FLIP_HORIZONTAL)
    {
        madctl |= ILI9341_MADCTL_FLIP_HORIZONTAL_OFF;                               // we use landscape as default
    }
    else
    {
        madctl |= ILI9341_MADCTL_FLIP_HORIZONTAL_ON;
    }

    if (ili9341.flags & ILI9341_GLOBAL_FLAGS_FLIP_VERTICAL)
    {
        madctl |= ILI9341_MADCTL_FLIP_VERTICAL_ON;
    }
    else
    {
        madctl |= ILI9341_MADCTL_FLIP_VERTICAL_OFF;
    }

    ili9341_write_command(ILI9341_MEMORY_ACCESS_CONTROL);
    ili9341_write_data (madctl);
    ili9341_write_command(ILI9341_MEMORY_ACCESS_CONTROL);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_colmod_pixel_format_set ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_colmod_pixel_format_set (void)
{
    ili9341_write_command(ILI9341_COLMOD_PIXEL_FORMAT_SET);
    ili9341_write_data(0x55);                                                               // 16-bit interface & 16-bit format
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_frame_rate_control_in_normal_mode_full_colors ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_frame_rate_control_in_normal_mode_full_colors (void)
{
    ili9341_write_command(ILI9341_FRAME_RATE_CONTROL_IN_NORMAL_MODE_FULL_COLORS);
    ili9341_write_data(0x00);
    ili9341_write_data(0x18);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_display_function_control ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_display_function_control (void)
{
    ili9341_write_command(ILI9341_DISPLAY_FUNCTION_CONTROL);
    ili9341_write_data(0x08);
    ili9341_write_data(0x82);
    ili9341_write_data(0x27);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_enable_g3 ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_enable_g3 (void)
{
    ili9341_write_command(ILI9341_ENABLE_G3);
    ili9341_write_data(0x00);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_sleep_out ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_sleep_out (void)
{
    ili9341_write_command(ILI9341_SLEEP_OUT);
    delay_msec(10);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_display_on ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ili9341_display_on (void)
{
    ili9341_write_command(ILI9341_DISPLAY_ON);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_set_flags () - set flags (yet not used)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_set_flags(uint_fast8_t new_flags)
{
    ili9341.flags = new_flags;
    ili9341_memory_access_control ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ili9341_init ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ili9341_init ()
{
    ili9341.flags = 0;

    ili9341_reset_ctrl_lines ();                                                            // control lines as input
    ili9341_init_ctrl_lines ();
    ili9341_init_fsmc ();
    delay_msec(20);                                                                         // wait for display coming up

    ili9341_soft_reset ();
    ili9341_powera ();
    ili9341_powerb ();
    ili9341_driver_timing_control_a ();
    ili9341_driver_timing_control_b ();
    ili9341_power_on_sequence_control ();
    ili9341_pump_ration_control ();
    ili9341_power_control_1 ();
    ili9341_power_control_2 ();
    ili9341_vcom_control_1 ();
    ili9341_vcom_control_2 ();
    ili9341_memory_access_control ();
    ili9341_colmod_pixel_format_set ();
    ili9341_frame_rate_control_in_normal_mode_full_colors ();
    ili9341_display_function_control ();
    ili9341_enable_g3 ();
    ili9341_sleep_out ();
    ili9341_display_on ();
}

#else                                                                                       // ISO C forbids empty translation unit
int ili9341 = 0;
#endif // ILI9341
