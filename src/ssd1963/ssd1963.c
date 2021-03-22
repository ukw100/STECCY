/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963.c - SSD1963 routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2018-2021 Frank Meyer - frank(at)fli4l.de
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
#if defined (SSD1963)

#include "tft.h"

#if 0
#include "eep.h"
#include "eeprom-data.h"
#include "vars.h"
#endif

#include "delay.h"

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * command codes:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_NOP                             0x00
#define SSD1963_SOFT_RESET                      0x01
#define SSD1963_GET_POWER_MODE                  0x0A
#define SSD1963_GET_ADDRESS_MODE                0x0B
#define SSD1963_GET_DISPLAY_MODE                0x0D
#define SSD1963_GET_TEAR_EFFECT_STATUS          0x0E
#define SSD1963_ENTER_SLEEP_MODE                0x10
#define SSD1963_EXIT_SLEEP_MODE                 0x11
#define SSD1963_ENTER_PARTIAL_MODE              0x12
#define SSD1963_ENTER_NORMAL_MODE               0x13
#define SSD1963_EXIT_INVERT_MODE                0x20
#define SSD1963_ENTER_INVERT_MODE               0x21
#define SSD1963_SET_GAMMA_CURVE                 0x26
#define SSD1963_SET_DISPLAY_OFF                 0x28
#define SSD1963_SET_DISPLAY_ON                  0x29
#define SSD1963_SET_COLUMN_ADDRESS              0x2A
#define SSD1963_SET_PAGE_ADDRESS                0x2B
#define SSD1963_WRITE_MEMORY_START              0x2C
#define SSD1963_READ_MEMORY_START               0x2E
#define SSD1963_SET_PARTIAL_AREA                0x30
#define SSD1963_SET_SCROLL_AREA                 0x33
#define SSD1963_SET_TEAR_OFF                    0x34
#define SSD1963_SET_TEAR_ON                     0x35
#define SSD1963_SET_ADDRESS_MODE                0x36
#define SSD1963_SET_SCROLL_START                0x37
#define SSD1963_EXIT_IDLE_MODE                  0x38
#define SSD1963_ENTER_IDLE_MODE                 0x39
#define SSD1963_WRITE_MEMORY_CONTINUE           0x3C
#define SSD1963_READ_MEMORY_CONTINUE            0x3E
#define SSD1963_SET_TEAR_SCANLINE               0x44
#define SSD1963_GET_SCANLINE                    0x45
#define SSD1963_READ_DDB                        0xA1
#define SSD1963_SET_LCD_MODE                    0xB0
#define SSD1963_GET_LCD_MODE                    0xB1
#define SSD1963_SET_HORI_PERIOD                 0xB4
#define SSD1963_GET_HORI_PERIOD                 0xB5
#define SSD1963_SET_VERT_PERIOD                 0xB6
#define SSD1963_GET_VERT_PERIOD                 0xB7
#define SSD1963_SET_GPIO_CONF                   0xB8
#define SSD1963_GET_GPIO_CONF                   0xB9
#define SSD1963_SET_GPIO_VALUE                  0xBA
#define SSD1963_GET_GPIO_STATUS                 0xBB
#define SSD1963_SET_POST_PROC                   0xBC
#define SSD1963_GET_POST_PROC                   0xBD
#define SSD1963_SET_PWM_CONF                    0xBE
#define SSD1963_GET_PWM_CONF                    0xBF
#define SSD1963_SET_LCD_GEN0                    0xC0
#define SSD1963_GET_LCD_GEN0                    0xC1
#define SSD1963_SET_LCD_GEN1                    0xC2
#define SSD1963_GET_LCD_GEN1                    0xC3
#define SSD1963_SET_LCD_GEN2                    0xC4
#define SSD1963_GET_LCD_GEN2                    0xC5
#define SSD1963_SET_LCD_GEN3                    0xC6
#define SSD1963_GET_LCD_GEN3                    0xC7
#define SSD1963_SET_GPIO0_ROP                   0xC8
#define SSD1963_GET_GPIO0_ROP                   0xC9
#define SSD1963_SET_GPIO1_ROP                   0xCA
#define SSD1963_GET_GPIO1_ROP                   0xCB
#define SSD1963_SET_GPIO2_ROP                   0xCC
#define SSD1963_GET_GPIO2_ROP                   0xCD
#define SSD1963_SET_GPIO3_ROP                   0xCE
#define SSD1963_GET_GPIO3_ROP                   0xCF
#define SSD1963_SET_DBC_CONF                    0xD0
#define SSD1963_GET_DBC_CONF                    0xD1
#define SSD1963_SET_DBC_TH                      0xD4
#define SSD1963_GET_DBC_TH                      0xD5
#define SSD1963_SET_PLL                         0xE0
#define SSD1963_SET_PLL_MN                      0xE2
#define SSD1963_GET_PLL_MN                      0xE3
#define SSD1963_GET_PLL_STATUS                  0xE4
#define SSD1963_SET_DEEP_SLEEP                  0xE5
#define SSD1963_SET_LSHIFT_FREQ                 0xE6
#define SSD1963_GET_LSHIFT_FREQ                 0xE7
#define SSD1963_SET_PIXEL_DATA_INTERFACE        0xF0
#define SSD1963_GET_PIXEL_DATA_INTERFACE        0xF1

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of get_power_mode()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_POWER_MODE_IDLE_MODE_OFF                                                (0x00 << 6)
#define SSD1963_POWER_MODE_IDLE_MODE_ON                                                 (0x01 << 6)
#define SSD1963_POWER_MODE_IDLE_MODE_MASK                                               (0x01 << 6)

#define SSD1963_POWER_MODE_PARTIAL_MODE_OFF                                             (0x00 << 5)
#define SSD1963_POWER_MODE_PARTIAL_MODE_ON                                              (0x01 << 5)
#define SSD1963_POWER_MODE_PARTIAL_MODE_MASK                                            (0x01 << 5)

#define SSD1963_POWER_MODE_SLEEP_MODE_ON                                                (0x00 << 4)
#define SSD1963_POWER_MODE_SLEEP_MODE_OFF                                               (0x01 << 4)
#define SSD1963_POWER_MODE_SLEEP_MODE_MASK                                              (0x01 << 4)

#define SSD1963_POWER_MODE_NORMAL_MODE_OFF                                              (0x00 << 3)
#define SSD1963_POWER_MODE_NORMAL_MODE_ON                                               (0x01 << 3)
#define SSD1963_POWER_MODE_NORMAL_MODE_MASK                                             (0x01 << 3)

#define SSD1963_POWER_MODE_DISPLAY_OFF                                                  (0x00 << 2)
#define SSD1963_POWER_MODE_DISPLAY_ON                                                   (0x01 << 2)
#define SSD1963_POWER_MODE_DISPLAY_MASK                                                 (0x01 << 2)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gamma_curve() & ssd1963_set_gamma_curve()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GAMMA_CURVE_NONE                                                        (0x00 << 0)
#define SSD1963_GAMMA_CURVE_0                                                           (0x01 << 0)
#define SSD1963_GAMMA_CURVE_1                                                           (0x02 << 0)
#define SSD1963_GAMMA_CURVE_2                                                           (0x03 << 0)
#define SSD1963_GAMMA_CURVE_3                                                           (0x04 << 0)
#define SSD1963_GAMMA_CURVE_MASK                                                        (0x07 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_address_mode() & ssd1963_get_address_mode()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_ADDRESS_MODE_PAGE_ADDRESS_ORDER_TOP_TO_BOTTOM                           (0x00 << 7)
#define SSD1963_ADDRESS_MODE_PAGE_ADDRESS_ORDER_BOTTOM_TO_TOP                           (0x01 << 7)
#define SSD1963_ADDRESS_MODE_PAGE_ADDRESS_ORDER_MASK                                    (0x01 << 7)

#define SSD1963_ADDRESS_MODE_COLUMN_ADDRESS_ORDER_LEFT_TO_RIGHT                         (0x00 << 6)
#define SSD1963_ADDRESS_MODE_COLUMN_ADDRESS_ORDER_RIGHT_TO_LEFT                         (0x01 << 6)
#define SSD1963_ADDRESS_MODE_COLUMN_ADDRESS_ORDER_MASK                                  (0x01 << 6)

#define SSD1963_ADDRESS_MODE_PAGE_COLUMN_ORDER_NORMAL_MODE                              (0x00 << 5)
#define SSD1963_ADDRESS_MODE_PAGE_COLUMN_ORDER_REVERSE_MODE                             (0x01 << 5)
#define SSD1963_ADDRESS_MODE_PAGE_COLUMN_ORDER_MASK                                     (0x01 << 5)

#define SSD1963_ADDRESS_MODE_LINE_ADDRESS_ORDER_LCD_REFRESH_TOP_TO_BOTTOM               (0x00 << 4)
#define SSD1963_ADDRESS_MODE_LINE_ADDRESS_ORDER_LCD_REFRESH_BOTTOM_TO_TOP               (0x01 << 4)
#define SSD1963_ADDRESS_MODE_LINE_ADDRESS_ORDER_MASK                                    (0x01 << 4)

#define SSD1963_ADDRESS_MODE_RGB_BGR_ORDER_RGB                                          (0x00 << 3)
#define SSD1963_ADDRESS_MODE_RGB_BGR_ORDER_BGR                                          (0x01 << 3)
#define SSD1963_ADDRESS_MODE_RGB_BGR_ORDER_MASK                                         (0x01 << 3)

#define SSD1963_ADDRESS_MODE_DISPLAY_DATA_LATCH_DATA_LCD_REFRESH_LEFT_TO_RIGHT          (0x00 << 2)
#define SSD1963_ADDRESS_MODE_DISPLAY_DATA_LATCH_DATA_LCD_REFRESH_RIGHT_TO_LEFT          (0x01 << 2)
#define SSD1963_ADDRESS_MODE_DISPLAY_DATA_LATCH_DATA_MASK                               (0x01 << 2)

#define SSD1963_ADDRESS_MODE_FLIP_HORIZONTAL_OFF                                        (0x00 << 1)
#define SSD1963_ADDRESS_MODE_FLIP_HORIZONTAL_ON                                         (0x01 << 1)
#define SSD1963_ADDRESS_MODE_FLIP_HORIZONTAL_MASK                                       (0x01 << 1)

#define SSD1963_ADDRESS_MODE_FLIP_VERTICAL_OFF                                          (0x00 << 0)
#define SSD1963_ADDRESS_MODE_FLIP_VERTICAL_ON                                           (0x01 << 0)
#define SSD1963_ADDRESS_MODE_FLIP_VERTICAL_MASK                                         (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_get_display_mode()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_DISPLAY_MODE_VERTICAL_SCROLLING_OFF                                     (0x00 << 7)
#define SSD1963_DISPLAY_MODE_VERTICAL_SCROLLING_ON                                      (0x01 << 7)
#define SSD1963_DISPLAY_MODE_VERTICAL_SCROLLING_MASK                                    (0x01 << 7)

#define SSD1963_DISPLAY_MODE_INVERT_INVERT_MODE_OFF                                     (0x00 << 5)
#define SSD1963_DISPLAY_MODE_INVERT_INVERT_MODE_ON                                      (0x01 << 5)
#define SSD1963_DISPLAY_MODE_INVERT_INVERT_MODE_MASK                                    (0x01 << 5)

#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_0                                              (0x00 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_1                                              (0x01 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_2                                              (0x02 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_3                                              (0x03 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_RESERVED_4                                     (0x04 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_RESERVED_5                                     (0x05 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_RESERVED_6                                     (0x06 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_RESERVED_7                                     (0x07 << 0)
#define SSD1963_DISPLAY_MODE_GAMMA_CURVE_MASK                                           (0x07 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_get_tear_effect_status()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_TEAR_EFFECT_STATUS_OFF                                                  (0x00 << 7)
#define SSD1963_TEAR_EFFECT_STATUS_ON                                                   (0x01 << 7)
#define SSD1963_TEAR_EFFECT_STATUS_MASK                                                 (0x01 << 7)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_lcd_mode() & ssd1963_get_lcd_mode()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
// lcd_mode_a:
#define SSD1963_LCD_MODE_A_PANEL_DATA_WIDTH_18_BIT                                      (0x00 << 5)
#define SSD1963_LCD_MODE_A_PANEL_DATA_WIDTH_24_BIT                                      (0x01 << 5)
#define SSD1963_LCD_MODE_A_PANEL_DATA_WIDTH_MASK                                        (0x01 << 5)

#define SSD1963_LCD_MODE_A_COLOR_DEPTH_ENHANCEMENT_DISABLE                              (0x00 << 4)
#define SSD1963_LCD_MODE_A_COLOR_DEPTH_ENHANCEMENT_ENABLE                               (0x01 << 4)
#define SSD1963_LCD_MODE_A_COLOR_DEPTH_ENHANCEMENT_MASK                                 (0x01 << 4)

#define SSD1963_LCD_MODE_A_TFT_FRC_ENABLE_DITHERING                                     (0x00 << 3)
#define SSD1963_LCD_MODE_A_TFT_FRC_ENABLE_FRC                                           (0x01 << 3)
#define SSD1963_LCD_MODE_A_TFT_FRC_FRC_MASK                                             (0x01 << 3)

#define SSD1963_LCD_MODE_A_LSHIFT_POLARITY_DATA_LATCH_IN_FALLING_EDGE                   (0x00 << 2)
#define SSD1963_LCD_MODE_A_LSHIFT_POLARITY_DATA_LATCH_IN_RISING_EDGE                    (0x01 << 2)
#define SSD1963_LCD_MODE_A_LSHIFT_POLARITY_DATA_LATCH_MASK                              (0x01 << 2)

#define SSD1963_LCD_MODE_A_LLINE_POLARITY_ACTIVE_LOW                                    (0x00 << 1)
#define SSD1963_LCD_MODE_A_LLINE_POLARITY_ACTIVE_HIGH                                   (0x01 << 1)
#define SSD1963_LCD_MODE_A_LLINE_POLARITY_MASK                                          (0x01 << 1)

#define SSD1963_LCD_MODE_A_LFRAME_POLARITY_ACTIVE_LOW                                   (0x00 << 0)
#define SSD1963_LCD_MODE_A_LFRAME_POLARITY_ACTIVE_HIGH                                  (0x01 << 0)
#define SSD1963_LCD_MODE_A_LFRAME_POLARITY_MASK                                         (0x01 << 0)

// lcd_mode_b:
#define SSD1963_LCD_MODE_B_TFT_TYPE_TFT_00_MODE                                         (0x00 << 5)
#define SSD1963_LCD_MODE_B_TFT_TYPE_TFT_01_MODE                                         (0x01 << 5)
#define SSD1963_LCD_MODE_B_TFT_TYPE_SERIAL_RGB_MODE                                     (0x10 << 5)
#define SSD1963_LCD_MODE_B_TFT_TYPE_SERIAL_RGB_DUMMY_MODE                               (0x11 << 5)
#define SSD1963_LCD_MODE_B_TFT_TYPE_MASK                                                (0x11 << 5)

// lcd_mode_sequence:
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_RGB                                              (0x00 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_RBG                                              (0x01 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_GRB                                              (0x02 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_GBR                                              (0x03 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_BRG                                              (0x04 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_BGR                                              (0x05 << 3)
#define SSD1963_LCD_MODE_SEQ_EVEN_LINE_MASK                                             (0x07 << 3)

#define SSD1963_LCD_MODE_SEQ_ODD_LINE_RGB                                               (0x00 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_RBG                                               (0x01 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_GRB                                               (0x02 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_GBR                                               (0x03 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_BRG                                               (0x04 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_BGR                                               (0x05 << 0)
#define SSD1963_LCD_MODE_SEQ_ODD_LINE_MASK                                              (0x07 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio_conf() & ssd1963_get_gpio_conf()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
// gpio_conf_a
#define SSD1963_GPIO_CONF_A_GPIO3_CONTROLLED_BY_HOST                                    (0x00 << 7)
#define SSD1963_GPIO_CONF_A_GPIO3_CONTROLLED_BY_LCDC                                    (0x01 << 7)
#define SSD1963_GPIO_CONF_A_GPIO3_CONTROLLED_MASK                                       (0x01 << 7)

#define SSD1963_GPIO_CONF_A_GPIO2_CONTROLLED_BY_HOST                                    (0x00 << 6)
#define SSD1963_GPIO_CONF_A_GPIO2_CONTROLLED_BY_LCDC                                    (0x01 << 6)
#define SSD1963_GPIO_CONF_A_GPIO2_CONTROLLED_MASK                                       (0x01 << 6)

#define SSD1963_GPIO_CONF_A_GPIO1_CONTROLLED_BY_HOST                                    (0x00 << 5)
#define SSD1963_GPIO_CONF_A_GPIO1_CONTROLLED_BY_LCDC                                    (0x01 << 5)
#define SSD1963_GPIO_CONF_A_GPIO1_CONTROLLED_MASK                                       (0x01 << 5)

#define SSD1963_GPIO_CONF_A_GPIO0_CONTROLLED_BY_HOST                                    (0x00 << 4)
#define SSD1963_GPIO_CONF_A_GPIO0_CONTROLLED_BY_LCDC                                    (0x01 << 4)
#define SSD1963_GPIO_CONF_A_GPIO0_CONTROLLED_MASK                                       (0x01 << 4)

#define SSD1963_GPIO_CONF_A_GPIO3_DIRECTION_INPUT                                       (0x00 << 3)
#define SSD1963_GPIO_CONF_A_GPIO3_DIRECTION_OUTPUT                                      (0x01 << 3)
#define SSD1963_GPIO_CONF_A_GPIO3_DIRECTION_MASK                                        (0x01 << 3)

#define SSD1963_GPIO_CONF_A_GPIO2_DIRECTION_INPUT                                       (0x00 << 2)
#define SSD1963_GPIO_CONF_A_GPIO2_DIRECTION_OUTPUT                                      (0x01 << 2)
#define SSD1963_GPIO_CONF_A_GPIO2_DIRECTION_MASK                                        (0x01 << 2)

#define SSD1963_GPIO_CONF_A_GPIO1_DIRECTION_INPUT                                       (0x00 << 1)
#define SSD1963_GPIO_CONF_A_GPIO1_DIRECTION_OUTPUT                                      (0x01 << 1)
#define SSD1963_GPIO_CONF_A_GPIO1_DIRECTION_MASK                                        (0x01 << 1)

#define SSD1963_GPIO_CONF_A_GPIO0_DIRECTION_INPUT                                       (0x00 << 0)
#define SSD1963_GPIO_CONF_A_GPIO0_DIRECTION_OUTPUT                                      (0x01 << 0)
#define SSD1963_GPIO_CONF_A_GPIO0_DIRECTION_MASK                                        (0x01 << 0)

// gpio_conf_b
#define SSD1963_GPIO_CONF_B_GPIO0_IS_CONTROL                                            (0x00 << 0)
#define SSD1963_GPIO_CONF_B_GPIO0_IS_GPIO                                               (0x01 << 0)
#define SSD1963_GPIO_CONF_B_GPIO0_MASK                                                  (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio_value() & ssd1963_get_gpio_value()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GPIO_VALUE_GPIO3_0                                                      (0x00 << 3)
#define SSD1963_GPIO_VALUE_GPIO3_1                                                      (0x01 << 3)
#define SSD1963_GPIO_VALUE_GPIO3_MASK                                                   (0x01 << 3)

#define SSD1963_GPIO_VALUE_GPIO2_0                                                      (0x00 << 2)
#define SSD1963_GPIO_VALUE_GPIO2_1                                                      (0x01 << 2)
#define SSD1963_GPIO_VALUE_GPIO2_MASK                                                   (0x01 << 2)

#define SSD1963_GPIO_VALUE_GPIO1_0                                                      (0x00 << 1)
#define SSD1963_GPIO_VALUE_GPIO1_1                                                      (0x01 << 1)
#define SSD1963_GPIO_VALUE_GPIO1_MASK                                                   (0x01 << 1)

#define SSD1963_GPIO_VALUE_GPIO0_0                                                      (0x00 << 0)
#define SSD1963_GPIO_VALUE_GPIO0_1                                                      (0x01 << 0)
#define SSD1963_GPIO_VALUE_GPIO0_MASK                                                   (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_post_proc() & ssd1963_get_post_proc()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_POST_PROC_DISABLE                                                       (0x00 << 0)
#define SSD1963_POST_PROC_ENABLE                                                        (0x01 << 0)
#define SSD1963_POST_PROC_MASK                                                          (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_pwm_conf() & ssd1963_get_pwm_conf()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
// possible values of pwm_conf_mask:
#define SSD1963_PWM_CONF_PWM_CONTROLLED_BY_HOST                                         (0x00 << 3)
#define SSD1963_PWM_CONF_PWM_CONTROLLED_BY_DBC                                          (0x01 << 3)
#define SSD1963_PWM_CONF_PWM_CONTROLLED_MASK                                            (0x01 << 3)

#define SSD1963_PWM_CONF_PWM_DISABLED                                                   (0x00 << 0)
#define SSD1963_PWM_CONF_PWM_ENABLED                                                    (0x01 << 0)
#define SSD1963_PWM_CONF_PWM_MASK                                                       (0x01 << 0)

// possible values of prescaler:
#define SSD1963_PWM_CONF_PRESCALER_OFF                                                  (0x00 << 0)
#define SSD1963_PWM_CONF_PRESCALER_1                                                    (0x01 << 0)
#define SSD1963_PWM_CONF_PRESCALER_2                                                    (0x02 << 0)
#define SSD1963_PWM_CONF_PRESCALER_3                                                    (0x03 << 0)
#define SSD1963_PWM_CONF_PRESCALER_4                                                    (0x04 << 0)
#define SSD1963_PWM_CONF_PRESCALER_6                                                    (0x05 << 0)
#define SSD1963_PWM_CONF_PRESCALER_8                                                    (0x06 << 0)
#define SSD1963_PWM_CONF_PRESCALER_12                                                   (0x07 << 0)
#define SSD1963_PWM_CONF_PRESCALER_16                                                   (0x08 << 0)
#define SSD1963_PWM_CONF_PRESCALER_24                                                   (0x09 << 0)
#define SSD1963_PWM_CONF_PRESCALER_32                                                   (0x0A << 0)
#define SSD1963_PWM_CONF_PRESCALER_48                                                   (0x0B << 0)
#define SSD1963_PWM_CONF_PRESCALER_64                                                   (0x0C << 0)
#define SSD1963_PWM_CONF_PRESCALER_96                                                   (0x0D << 0)
#define SSD1963_PWM_CONF_PRESCALER_128                                                  (0x0E << 0)
#define SSD1963_PWM_CONF_PRESCALER_192                                                  (0x0F << 0)
#define SSD1963_PWM_CONF_PRESCALER_MASK                                                 (0x0F << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_lcd_gen0() & ssd1963_get_lcd_gen0()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_LCD_GEN0_NOT_RESET_IN_STARTING_POINT                                    (0x00 << 7)
#define SSD1963_LCD_GEN0_RESET_IN_STARTING_POINT                                        (0x01 << 7)
#define SSD1963_LCD_GEN0_MASK                                                           (0x01 << 7)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_lcd_gen1() & ssd1963_get_lcd_gen1()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_LCD_GEN1_NOT_RESET_IN_STARTING_POINT                                    (0x00 << 7)
#define SSD1963_LCD_GEN1_RESET_IN_STARTING_POINT                                        (0x01 << 7)
#define SSD1963_LCD_GEN1_MASK                                                           (0x01 << 7)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_lcd_gen2() & ssd1963_get_lcd_gen2()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_LCD_GEN2_NOT_RESET_IN_STARTING_POINT                                    (0x00 << 7)
#define SSD1963_LCD_GEN2_RESET_IN_STARTING_POINT                                        (0x01 << 7)
#define SSD1963_LCD_GEN2_MASK                                                           (0x01 << 7)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_lcd_gen3() & ssd1963_get_lcd_gen3()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_LCD_GEN3_NOT_RESET_IN_STARTING_POINT                                    (0x00 << 7)
#define SSD1963_LCD_GEN3_RESET_IN_STARTING_POINT                                        (0x01 << 7)
#define SSD1963_LCD_GEN3_MASK                                                           (0x01 << 7)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio0_rop() & ssd1963_get_gpio0_rop()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GPIO0_SOURCE1_GEN0                                                      (0x00 << 5)
#define SSD1963_GPIO0_SOURCE1_GEN1                                                      (0x01 << 5)
#define SSD1963_GPIO0_SOURCE1_GEN2                                                      (0x02 << 5)
#define SSD1963_GPIO0_SOURCE1_GEN3                                                      (0x03 << 5)
#define SSD1963_GPIO0_SOURCE1_MASK                                                      (0x03 << 5)

#define SSD1963_GPIO0_SOURCE2_GEN0                                                      (0x00 << 2)
#define SSD1963_GPIO0_SOURCE2_GEN1                                                      (0x01 << 2)
#define SSD1963_GPIO0_SOURCE2_GEN2                                                      (0x02 << 2)
#define SSD1963_GPIO0_SOURCE2_GEN3                                                      (0x03 << 2)
#define SSD1963_GPIO0_SOURCE2_MASK                                                      (0x03 << 2)

#define SSD1963_GPIO0_SOURCE3_GEN0                                                      (0x00 << 0)
#define SSD1963_GPIO0_SOURCE3_GEN1                                                      (0x01 << 0)
#define SSD1963_GPIO0_SOURCE3_GEN2                                                      (0x02 << 0)
#define SSD1963_GPIO0_SOURCE3_GEN3                                                      (0x03 << 0)
#define SSD1963_GPIO0_SOURCE3_MASK                                                      (0x03 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio1_rop() & ssd1963_get_gpio1_rop()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GPIO1_SOURCE1_GEN0                                                      (0x00 << 5)
#define SSD1963_GPIO1_SOURCE1_GEN1                                                      (0x01 << 5)
#define SSD1963_GPIO1_SOURCE1_GEN2                                                      (0x02 << 5)
#define SSD1963_GPIO1_SOURCE1_GEN3                                                      (0x03 << 5)
#define SSD1963_GPIO1_SOURCE1_MASK                                                      (0x03 << 5)

#define SSD1963_GPIO1_SOURCE2_GEN0                                                      (0x00 << 2)
#define SSD1963_GPIO1_SOURCE2_GEN1                                                      (0x01 << 2)
#define SSD1963_GPIO1_SOURCE2_GEN2                                                      (0x02 << 2)
#define SSD1963_GPIO1_SOURCE2_GEN3                                                      (0x03 << 2)
#define SSD1963_GPIO1_SOURCE2_MASK                                                      (0x03 << 2)

#define SSD1963_GPIO1_SOURCE3_GEN0                                                      (0x00 << 0)
#define SSD1963_GPIO1_SOURCE3_GEN1                                                      (0x01 << 0)
#define SSD1963_GPIO1_SOURCE3_GEN2                                                      (0x02 << 0)
#define SSD1963_GPIO1_SOURCE3_GEN3                                                      (0x03 << 0)
#define SSD1963_GPIO1_SOURCE3_MASK                                                      (0x03 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio2_rop() & ssd1963_get_gpio2_rop()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GPIO2_SOURCE1_GEN0                                                      (0x00 << 5)
#define SSD1963_GPIO2_SOURCE1_GEN1                                                      (0x01 << 5)
#define SSD1963_GPIO2_SOURCE1_GEN2                                                      (0x02 << 5)
#define SSD1963_GPIO2_SOURCE1_GEN3                                                      (0x03 << 5)
#define SSD1963_GPIO2_SOURCE1_MASK                                                      (0x03 << 5)

#define SSD1963_GPIO2_SOURCE2_GEN0                                                      (0x00 << 2)
#define SSD1963_GPIO2_SOURCE2_GEN1                                                      (0x01 << 2)
#define SSD1963_GPIO2_SOURCE2_GEN2                                                      (0x02 << 2)
#define SSD1963_GPIO2_SOURCE2_GEN3                                                      (0x03 << 2)
#define SSD1963_GPIO2_SOURCE2_MASK                                                      (0x03 << 2)

#define SSD1963_GPIO2_SOURCE3_GEN0                                                      (0x00 << 0)
#define SSD1963_GPIO2_SOURCE3_GEN1                                                      (0x01 << 0)
#define SSD1963_GPIO2_SOURCE3_GEN2                                                      (0x02 << 0)
#define SSD1963_GPIO2_SOURCE3_GEN3                                                      (0x03 << 0)
#define SSD1963_GPIO2_SOURCE3_MASK                                                      (0x03 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_gpio3_rop() & ssd1963_get_gpio3_rop()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_GPIO3_SOURCE1_GEN0                                                      (0x00 << 5)
#define SSD1963_GPIO3_SOURCE1_GEN1                                                      (0x01 << 5)
#define SSD1963_GPIO3_SOURCE1_GEN2                                                      (0x02 << 5)
#define SSD1963_GPIO3_SOURCE1_GEN3                                                      (0x03 << 5)
#define SSD1963_GPIO3_SOURCE1_MASK                                                      (0x03 << 5)

#define SSD1963_GPIO3_SOURCE2_GEN0                                                      (0x00 << 2)
#define SSD1963_GPIO3_SOURCE2_GEN1                                                      (0x01 << 2)
#define SSD1963_GPIO3_SOURCE2_GEN2                                                      (0x02 << 2)
#define SSD1963_GPIO3_SOURCE2_GEN3                                                      (0x03 << 2)
#define SSD1963_GPIO3_SOURCE2_MASK                                                      (0x03 << 2)

#define SSD1963_GPIO3_SOURCE3_GEN0                                                      (0x00 << 0)
#define SSD1963_GPIO3_SOURCE3_GEN1                                                      (0x01 << 0)
#define SSD1963_GPIO3_SOURCE3_GEN2                                                      (0x02 << 0)
#define SSD1963_GPIO3_SOURCE3_GEN3                                                      (0x03 << 0)
#define SSD1963_GPIO3_SOURCE3_MASK                                                      (0x03 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_dbc_conf() & ssd1963_get_dbc_conf()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_DBC_CONF_MANUAL_BRIGHTNESS_ENABLE                                       (0x00 << 6)
#define SSD1963_DBC_CONF_MANUAL_BRIGHTNESS_DISABLE                                      (0x01 << 6)
#define SSD1963_DBC_CONF_MANUAL_BRIGHTNESS_MASK                                         (0x01 << 6)

#define SSD1963_DBC_CONF_TRANSITION_EFFECT_DISABLE                                      (0x00 << 5)
#define SSD1963_DBC_CONF_TRANSITION_EFFECT_ENABLE                                       (0x01 << 5)
#define SSD1963_DBC_CONF_TRANSITION_EFFECT_MASK                                         (0x01 << 5)

#define SSD1963_DBC_CONF_ENERGY_SAVING_DBC_DISABLE                                      (0x00 << 2)
#define SSD1963_DBC_CONF_ENERGY_SAVING_DBC_CONSERVATIVE                                 (0x01 << 2)
#define SSD1963_DBC_CONF_ENERGY_SAVING_DBC_NORMAL                                       (0x02 << 2)
#define SSD1963_DBC_CONF_ENERGY_SAVING_DBC_AGGRESSIVE                                   (0x03 << 2)
#define SSD1963_DBC_CONF_ENERGY_SAVING_DBC_MASK                                         (0x03 << 2)

#define SSD1963_DBC_CONF_MASTER_DISABLE                                                 (0x00 << 0)
#define SSD1963_DBC_CONF_MASTER_ENABLE                                                  (0x01 << 0)
#define SSD1963_DBC_CONF_MASTER_MASK                                                    (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_pll()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_PLL_LOCK_USE_REFERENCE_CLOCK                                            (0x00 << 1)
#define SSD1963_PLL_LOCK_USE_PLL_OUTPUT                                                 (0x01 << 1)
#define SSD1963_PLL_LOCK_USE_MASK                                                       (0x01 << 1)

#define SSD1963_PLL_LOCK_DISABLE_PLL                                                    (0x00 << 0)
#define SSD1963_PLL_LOCK_ENABLE_PLL                                                     (0x01 << 0)
#define SSD1963_PLL_LOCK_PLL_MASK                                                       (0x01 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_pll_mn() & ssd1963_get_pll_mn()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_PLL_MN_IGNORE_MULTIPLIER_AND_DIVIDER                                    (0x00 << 2)
#define SSD1963_PLL_MN_EFFECTUATE_MULTIPLIER_AND_DIVIDER                                (0x01 << 2)
#define SSD1963_PLL_MN_MASK                                                             (0x01 << 2)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_get_pll_status()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_PLL_STATUS_NOT_LOCKED                                                   (0x00 << 2)
#define SSD1963_PLL_STATUS_LOCKED                                                       (0x01 << 2)
#define SSD1963_PLL_STATUS_MASK                                                         (0x01 << 2)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * parameters of ssd1963_set_pixel_data_interface() & ssd1963_get_pixel_data_interface()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define SSD1963_PIXEL_DATA_INTERFACE_8_BIT                                              (0x00 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_12_BIT                                             (0x01 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_16_BIT_PACKED                                      (0x02 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_16_BIT_565                                         (0x03 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_18_BIT                                             (0x04 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_24_BIT                                             (0x05 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_9_BIT                                              (0x06 << 0)
#define SSD1963_PIXEL_DATA_INTERFACE_MASK                                               (0x07 << 0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * configuration values
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
SSD1963_GLOBALS  ssd1963 =
{
    SSD1963_GLOBAL_FLAGS_RGB_ORDER | SSD1963_GLOBAL_FLAGS_FLIP_HORIZONTAL               // default: rgb, flip horizontal
};

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_reset_ctrl_lines () - reset control lines, set them to input to allow auto-init
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ssd1963_reset_ctrl_lines (void)
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
 * ssd1963_init_ctrl_lines() - init control lines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ssd1963_init_ctrl_lines (void)
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

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15 |
                                  GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

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
 * ssd1963_init_fsmc () - init FSMC
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ssd1963_init_fsmc (void)
{
    FSMC_NORSRAMInitTypeDef             fsmc;
    FSMC_NORSRAMTimingInitTypeDef       ftime;

    RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);                                            // Enable FMC clock

    ftime.FSMC_AddressSetupTime             = 15;
    ftime.FSMC_AddressHoldTime              = 0;
    ftime.FSMC_DataSetupTime                = 13;
    ftime.FSMC_BusTurnAroundDuration        = 0;
    ftime.FSMC_CLKDivision                  = 0;
    ftime.FSMC_DataLatency                  = 0;
    ftime.FSMC_AccessMode                   = FSMC_AccessMode_A;

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
 * ssd1963_nop () - No operation
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_nop (void)
{
    ssd1963_write_command (SSD1963_NOP);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_soft_reset () - Software Reset
 *
 * The SSD1963 performs a software reset.
 * All the configuration register will be reset except command 0xE0 to 0xE5.
 * Note:
 * The host processor must wait 5ms before sending any new commands to a SSD1963 following this command.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_soft_reset (void)
{
    ssd1963_write_command (SSD1963_SOFT_RESET);
    delay_msec (5);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_power_mode () - Get the current power mode
 *
 * possible bits set in power_mode:
 *   SSD1963_POWER_MODE_IDLE_MODE_ON            - idle mode is on
 *   SSD1963_POWER_MODE_PARTIAL_MODE_ON         - partial mode is on
 *   SSD1963_POWER_MODE_SLEEP_MODE_OFF          - sleep mode is *off*
 *   SSD1963_POWER_MODE_NORMAL_MODE_ON          - normal mode is on
 *   SSD1963_POWER_MODE_DISPLAY_ON              - display is on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_power_mode (uint_fast8_t * power_mode)
{
    ssd1963_write_command (SSD1963_GET_POWER_MODE);
    *power_mode = ssd1963_read_data ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_address_mode () - Get frame buffer to the display panel read order
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_address_mode (uint_fast8_t * address_mode)
{
    ssd1963_write_command (SSD1963_GET_ADDRESS_MODE);
    *address_mode = ssd1963_read_data ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_display_mode () - Get Display Image Mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_display_mode (uint_fast8_t * display_mode)
{
    ssd1963_write_command (SSD1963_GET_DISPLAY_MODE);
    *display_mode = ssd1963_read_data ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_tear_effect_status () - Get Tear Effect status
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_tear_effect_status (uint_fast8_t * tear_effect_status)
{
    ssd1963_write_command (SSD1963_GET_TEAR_EFFECT_STATUS);
    *tear_effect_status = ssd1963_read_data ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_enter_sleep_mode () - Turn off the panel
 *
 * This command will pull high the GPIO0.
 * If GPIO0 is configured as normal GPIO or LCD miscellaneous signal with command set_gpio_conf, this
 * command will be ignored.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_enter_sleep_mode (void)
{
    ssd1963_write_command (SSD1963_ENTER_SLEEP_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_exit_sleep_mode () - Turn on the panel.
 *
 * This command will pull low the GPIO0.
 * If GPIO0 is configured as normal GPIO or LCD miscellaneous signal with command set_gpio_conf, this
 * command will be ignored.
 *
 * Note:
 * The host processor must wait 5ms after sending this command before sending another command.
 * This command will automatic trigger set_display_on (0x29)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_exit_sleep_mode (void)
{
    ssd1963_write_command (SSD1963_EXIT_SLEEP_MODE);
    delay_msec (5);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_enter_partial_mode () - Part of the display area is used for image display.
 *
 * Once triggered, the Partial Display Mode window is described by the set_partial_area (0x30).
 * Once enter_normal_mode (0x13) is triggered, partial display mode will end.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_enter_partial_mode (void)
{
    ssd1963_write_command (SSD1963_ENTER_PARTIAL_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_enter_normal_mode () - The whole display area is used for image display.
 *
 * This command causes the SSD1963 to enter the normal mode.  Normal mode is defined as partial display and vertical
 * scroll mode are off.   That means the whole display area is used for image display.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_enter_normal_mode (void)
{
    ssd1963_write_command (SSD1963_ENTER_NORMAL_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_exit_invert_mode () - Displayed image colors are not inverted.
 *
 * This command causes the SSD1963 to stop inverting the image data on the display panel.
 * The frame buffer contents remain unchanged.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_exit_invert_mode (void)
{
    ssd1963_write_command (SSD1963_EXIT_INVERT_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_enter_invert_mode () - Displayed image colors are inverted.
 *
 * This command causes the SSD1963 to invert the image data only on the display panel.
 * The frame buffer contents remain unchanged.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_enter_invert_mode (void)
{
    ssd1963_write_command (SSD1963_ENTER_INVERT_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gamma_curve () - Selects the gamma curve used by the display panel.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gamma_curve (uint_fast8_t gamma_curve)
{
    ssd1963_write_command (SSD1963_SET_GAMMA_CURVE);
    ssd1963_write_data (gamma_curve);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_display_off () - Blanks the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_display_off (void)
{
    ssd1963_write_command (SSD1963_SET_DISPLAY_OFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_display_on () - Show the image on the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_display_on (void)
{
    ssd1963_write_command (SSD1963_SET_DISPLAY_ON);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_column_address () - Set the column address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_column_address (uint_fast16_t start_column_number, uint_fast16_t end_column_number)
{
    ssd1963_write_command (SSD1963_SET_COLUMN_ADDRESS);
    ssd1963_write_data (start_column_number >> 8);
    ssd1963_write_data (start_column_number & 0xFF);
    ssd1963_write_data (end_column_number >> 8);
    ssd1963_write_data (end_column_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_page_address () - Set the page address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_page_address (uint_fast16_t start_page_row_number, uint_fast16_t end_page_row_number)
{
    ssd1963_write_command (SSD1963_SET_PAGE_ADDRESS);
    ssd1963_write_data (start_page_row_number >> 8);
    ssd1963_write_data (start_page_row_number & 0xFF);
    ssd1963_write_data (end_page_row_number >> 8);
    ssd1963_write_data (end_page_row_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_write_memory_start () - Transfer image information from the host processor interface
 * to the SSD1963 starting at the location provided by set_column_address and set_page_address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_write_memory_start (void)
{
    ssd1963_write_command (SSD1963_WRITE_MEMORY_START);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_read_memory_start () - Transfer image data from the SSD1963 to the host processor
 * interface starting at the location provided by set_column_address and set_page_address
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_read_memory_start (void)
{
    ssd1963_write_command (SSD1963_READ_MEMORY_START);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_partial_area () - Defines the partial display area on the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_partial_area (uint_fast16_t start_display_row_number, uint_fast16_t end_display_row_number)
{
    ssd1963_write_command (SSD1963_SET_PARTIAL_AREA);
    ssd1963_write_data (start_display_row_number >> 8);
    ssd1963_write_data (start_display_row_number & 0xFF);
    ssd1963_write_data (end_display_row_number >> 8);
    ssd1963_write_data (end_display_row_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_scroll_area () - Defines the vertical scrolling and fixed area on display area
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_scroll_area (uint_fast16_t top_fixed_area_number, uint_fast16_t vertical_scrolling_area, uint_fast16_t bottom_fixed_area)
{
    ssd1963_write_command (SSD1963_SET_SCROLL_AREA);
    ssd1963_write_data (top_fixed_area_number >> 8);
    ssd1963_write_data (top_fixed_area_number & 0xFF);
    ssd1963_write_data (vertical_scrolling_area >> 8);
    ssd1963_write_data (vertical_scrolling_area & 0xFF);
    ssd1963_write_data (bottom_fixed_area >> 8);
    ssd1963_write_data (bottom_fixed_area & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_tear_off () - Synchronization information is not sent from SSD1963 to host processor
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_tear_off (void)
{
    ssd1963_write_command (SSD1963_SET_TEAR_OFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_tear_on () - Synchronization information is sent from SSD1963 to host processor at start of VFP
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_tear_on (uint_fast8_t tearing_effect_line_mode)
{
    ssd1963_write_command (SSD1963_SET_TEAR_ON);
    ssd1963_write_data (tearing_effect_line_mode);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_address_mode () - Set the read order from frame buffer to the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_address_mode (uint_fast8_t address_mode)
{
    ssd1963_write_command (SSD1963_SET_ADDRESS_MODE);
    ssd1963_write_data (address_mode);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_scroll_start () - Defines the vertical scrolling starting point
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_scroll_start (uint_fast16_t line_number)
{
    ssd1963_write_command (SSD1963_SET_SCROLL_START);
    ssd1963_write_data (line_number >> 8);
    ssd1963_write_data (line_number & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_exit_idle_mode () - Full color depth is used for the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_exit_idle_mode (void)
{
    ssd1963_write_command (SSD1963_EXIT_IDLE_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_enter_idle_mode () - Reduce color depth is used on the display panel
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_enter_idle_mode (void)
{
    ssd1963_write_command (SSD1963_ENTER_IDLE_MODE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_write_memory_continue () - Transfer image information from the host processor interface
 * to the SSD1963 from the last written location
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_write_memory_continue (void)
{
    ssd1963_write_command (SSD1963_WRITE_MEMORY_CONTINUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_read_memory_continue () - Read image data from the SSD1963
 * continuing after the last read_memory_continue or read_memory_start
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_read_memory_continue (void)
{
    ssd1963_write_command (SSD1963_READ_MEMORY_CONTINUE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_tear_scanline () - Synchronization information is sent from the SSD1963 to the
 * host processor when the display panel refresh reaches the provided scanline
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_tear_scanline (uint_fast16_t scanline)
{
    ssd1963_write_command (SSD1963_SET_TEAR_SCANLINE);

    ssd1963_write_data (scanline >> 8);
    ssd1963_write_data (scanline & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_scanline () - Get the current scan line
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_scanline (uint_fast16_t * scanlinep)
{
    uint_fast16_t scanline;

    ssd1963_write_command (SSD1963_GET_SCANLINE);

    scanline = ssd1963_read_data () << 8;
    scanline |= ssd1963_read_data ();

    *scanlinep = scanline;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_read_ddb () - Read the DDB from the provided location
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_read_ddb (uint_fast16_t * supplieridp, uint_fast8_t * productidp, uint_fast8_t * revp, uint_fast8_t * exitcodep)
{
    uint_fast16_t   supplierid;
    uint_fast8_t    productid;
    uint_fast8_t    rev;
    uint_fast8_t    exitcode;

    ssd1963_write_command (SSD1963_READ_DDB);

    supplierid      = ssd1963_read_data () << 8;
    supplierid      |= ssd1963_read_data ();

    productid       = ssd1963_read_data ();
    rev             = ssd1963_read_data ();
    exitcode        = ssd1963_read_data ();

    *supplieridp    = supplierid;
    *productidp     = productid;
    *revp           = rev;
    *exitcodep      = exitcode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lcd_mode () - Set the LCD panel mode and resolution
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lcd_mode (uint_fast8_t lcd_mode_a, uint_fast8_t lcd_mode_b, uint_fast16_t hor_panel_size, uint_fast16_t vert_panel_size, uint_fast8_t seq)
{
    ssd1963_write_command (SSD1963_SET_LCD_MODE);
    ssd1963_write_data (lcd_mode_a);
    ssd1963_write_data (lcd_mode_b);
    ssd1963_write_data (hor_panel_size >> 8);
    ssd1963_write_data (hor_panel_size & 0xFF);
    ssd1963_write_data (vert_panel_size >> 8);
    ssd1963_write_data (vert_panel_size & 0xFF);
    ssd1963_write_data (seq);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lcd_mode () - Get the current LCD panel mode, pad strength and resolution
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lcd_mode (uint_fast8_t * lcd_mode_ap, uint_fast8_t * lcd_mode_bp, uint_fast16_t * hor_panel_sizep, uint_fast16_t * vert_panel_sizep, uint_fast8_t * seqp)
{
    uint_fast8_t    lcd_mode_a;
    uint_fast8_t    lcd_mode_b;
    uint_fast16_t   hor_panel_size;
    uint_fast16_t   vert_panel_size;
    uint_fast8_t    seq;

    ssd1963_write_command (SSD1963_GET_LCD_MODE);

    lcd_mode_a      = ssd1963_read_data ();
    lcd_mode_b      = ssd1963_read_data ();
    hor_panel_size  = ssd1963_read_data () << 8;
    hor_panel_size  |= ssd1963_read_data ();
    vert_panel_size = ssd1963_read_data () << 8;
    vert_panel_size |= ssd1963_read_data ();
    seq             = ssd1963_read_data ();

    *lcd_mode_ap            = lcd_mode_a;
    *lcd_mode_bp            = lcd_mode_b;
    *hor_panel_sizep        = hor_panel_size;
    *vert_panel_sizep       = vert_panel_size;
    *seqp                   = seq;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_hori_period () - Set front porch
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_hori_period (uint_fast16_t ht, uint_fast16_t hps, uint_fast8_t hpw, uint_fast16_t lps, uint_fast8_t lpspp)
{
    ssd1963_write_command (SSD1963_SET_HORI_PERIOD);

    ssd1963_write_data (ht >> 8);
    ssd1963_write_data (ht & 0xFF);
    ssd1963_write_data (hps >> 8);
    ssd1963_write_data (hps & 0xFF);
    ssd1963_write_data (hpw);
    ssd1963_write_data (lps >> 8);
    ssd1963_write_data (lps & 0xFF);
    ssd1963_write_data (lpspp);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_hori_period () - Get current front porch settings
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_hori_period (uint_fast16_t * htp, uint_fast16_t * hpsp, uint_fast8_t * hpwp, uint_fast16_t * lpsp, uint_fast8_t * lpsppp)
{
    uint_fast16_t   ht;
    uint_fast16_t   hps;
    uint_fast8_t    hpw;
    uint_fast16_t   lps;
    uint_fast8_t    lpspp;

    ssd1963_write_command (SSD1963_GET_HORI_PERIOD);

    ht      = ssd1963_read_data () << 8;
    ht      |= ssd1963_read_data ();
    hps     = ssd1963_read_data () << 8;
    hps     |= ssd1963_read_data ();
    hpw     = ssd1963_read_data ();
    lps     = ssd1963_read_data () << 8;
    lps     |= ssd1963_read_data ();
    lpspp   = ssd1963_read_data ();

    *htp    = ht;
    *hpsp   = hps;
    *hpwp   = hpw;
    *lpsp   = lps;
    *lpsppp = lpspp;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_vert_period () - Set the vertical blanking interval between last scan line and
 * next LFRAME pulse
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_vert_period (uint_fast16_t vt, uint_fast16_t vps, uint_fast8_t vpw, uint_fast16_t fps)
{
    ssd1963_write_command (SSD1963_SET_VERT_PERIOD);

    ssd1963_write_data (vt >> 8);
    ssd1963_write_data (vt & 0xFF);
    ssd1963_write_data (vps >> 8);
    ssd1963_write_data (vps & 0xFF);
    ssd1963_write_data (vpw);
    ssd1963_write_data (fps >> 8);
    ssd1963_write_data (fps & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_vert_period () - Get the vertical blanking interval between last scan line and
 * next LFRAME pulse
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_vert_period (uint_fast16_t * vtp, uint_fast16_t * vpsp, uint_fast8_t * vpwp, uint_fast16_t * fpsp)
{
    uint_fast16_t   vt;
    uint_fast16_t   vps;
    uint_fast8_t    vpw;
    uint_fast16_t   fps;

    ssd1963_write_command (SSD1963_GET_VERT_PERIOD);

    vt      = ssd1963_read_data () << 8;
    vt      |= ssd1963_read_data ();
    vps     = ssd1963_read_data () << 8;
    vps     |= ssd1963_read_data ();
    vpw     = ssd1963_read_data ();
    fps     = ssd1963_read_data () << 8;
    fps     |= ssd1963_read_data ();

    *vtp    = vt;
    *vpsp   = vps;
    *vpwp   = vpw;
    *fpsp   = fps;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio_conf () - Set the GPIO configuration.
 * If the GPIO is not used for LCD, set the direction.  Otherwise, they are toggled with LCD signals.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio_conf (uint_fast8_t gpio_conf_a, uint_fast8_t gpio_conf_b)
{
    ssd1963_write_command (SSD1963_SET_GPIO_CONF);
    ssd1963_write_data (gpio_conf_a);
    ssd1963_write_data (gpio_conf_b);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio_conf () - Get the current GPIO configuration
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio_conf (uint_fast8_t * gpio_conf_ap, uint_fast8_t * gpio_conf_bp)
{
    uint_fast8_t gpio_conf_a;
    uint_fast8_t gpio_conf_b;

    ssd1963_write_command (SSD1963_GET_GPIO_CONF);

    gpio_conf_a = ssd1963_read_data ();
    gpio_conf_b = ssd1963_read_data ();

    *gpio_conf_ap   = gpio_conf_a;
    *gpio_conf_bp   = gpio_conf_b;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio_value () - Set GPIO value for GPIO configured as output
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio_value (uint_fast8_t gpio_value)
{
    ssd1963_write_command (SSD1963_SET_GPIO_VALUE);
    ssd1963_write_data (gpio_value);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio_status () - Read current GPIO status.
 * If the individual GPIO was configured as input, the value is the status of the corresponding pin.
 * Otherwise, it is the programmed value.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio_status (uint_fast8_t * gpio_statusp)
{
    uint_fast8_t    gpio_status;

    ssd1963_write_command (SSD1963_GET_GPIO_STATUS);

    gpio_status     = ssd1963_read_data ();
    *gpio_statusp   = gpio_status;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_post_proc () - Set the image post processor
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_post_proc (uint_fast8_t contrast, uint_fast8_t brightness, uint_fast8_t saturation, uint_fast8_t post_proc_enable)
{
    ssd1963_write_command (SSD1963_SET_POST_PROC);
    ssd1963_write_data (contrast);
    ssd1963_write_data (brightness);
    ssd1963_write_data (saturation);
    ssd1963_write_data (post_proc_enable);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_post_proc () - Get the image post processor
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_post_proc (uint_fast8_t * contrastp, uint_fast8_t * brightnessp, uint_fast8_t * saturationp, uint_fast8_t * post_proc_enablep)
{
    uint_fast8_t contrast;
    uint_fast8_t brightness;
    uint_fast8_t saturation;
    uint_fast8_t post_proc_enable;

    ssd1963_write_command (SSD1963_GET_POST_PROC);

    contrast            = ssd1963_read_data ();
    brightness          = ssd1963_read_data ();
    saturation          = ssd1963_read_data ();
    post_proc_enable    = ssd1963_read_data ();

    *contrastp          = contrast;
    *brightnessp        = brightness;
    *saturationp        = saturation;
    *post_proc_enablep  = post_proc_enable;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_pwm_conf () - Set the PWM configuration
 *
 * parameters:
 *
 *      pwmf:           PWM frequency, PWM signal frequency = PLL clock / (256 * (PWMF[7:0] + 1)) / 256
 *      pwm:            PWM duty cycle  = PWM[7:0] / 256 for DBC disable (0xD0] A0 = 0
 *                      If DBC enable (0xD0] A0 = 1, these parameter will be ignored
 *                      PWM always 0 if PWM[7:0] = 00h
 *      pwm_conf_mask:  SSD1963_PWM_CONF_PWM_CONTROLLED_BY_HOST or SSD1963_PWM_CONF_PWM_CONTROLLED_BY_DBC |
 *                      SSD1963_PWM_CONF_PWM_ENABLED or SSD1963_PWM_CONF_PWM_DISABLED
 *      man_br:         DBC manual brightness. When Manual Brightness Mode (0xD0) A[6] is enabled, the final DBC duty cycle
 *                      output will be multiplied by this value / 255.
 *      min_br:         minimum brightness
 *      prescaler:      brightness prescaler
 *
 * example: set brightness to 100%
 *
 *      ssd1963_set_pwm_conf (0x06,                                                                         // pwmf
 *                            0xFF,                                                                         // pwm duty cycle
 *                            SSD1963_PWM_CONF_PWM_CONTROLLED_BY_HOST | SSD1963_PWM_CONF_PWM_ENABLED,
 *                            0xF0,                                                                         // ??? brightness, should be 0xFF!
 *                            0x00,                                                                         // minimum brightness = 0
 *                            SSD1963_PWM_CONF_PRESCALER_OFF);                                              // no prescaler
 *
 * example: set brightness to 0%
 *
 *      ssd1963_set_pwm_conf (0x06,                                                                         // pwmf
 *                            0x00,                                                                         // pwm duty cycle
 *                            SSD1963_PWM_CONF_PWM_CONTROLLED_BY_HOST | SSD1963_PWM_CONF_PWM_ENABLED,       // pwm_conf_mask
 *                            0xF0,                                                                         // ??? brightness, should be 0x00!
 *                            0x00,                                                                         // minimum brightness = 0
 *                            SSD1963_PWM_CONF_PRESCALER_OFF);                                              // no prescaler
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_pwm_conf (uint_fast8_t pwmf, uint_fast8_t pwm, uint_fast8_t pwm_conf_mask, uint_fast8_t man_br, uint_fast8_t min_br, uint_fast8_t prescaler)
{
    ssd1963_write_command (SSD1963_SET_PWM_CONF);
    ssd1963_write_data (pwmf);
    ssd1963_write_data (pwm);
    ssd1963_write_data (pwm_conf_mask);
    ssd1963_write_data (man_br);
    ssd1963_write_data (min_br);
    ssd1963_write_data (prescaler);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_pwm_conf () - Get the PWM configuration
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_pwm_conf (uint_fast8_t * pwmfp, uint_fast8_t * pwmp, uint_fast8_t * pwm_conf_maskp,
                      uint_fast8_t * man_brp, uint_fast8_t * min_brp, uint_fast8_t * prescalerp, uint_fast8_t * backlight_duty_cyclep)
{
    uint_fast8_t pwmf;
    uint_fast8_t pwm;
    uint_fast8_t pwm_conf_mask;
    uint_fast8_t man_br;
    uint_fast8_t min_br;
    uint_fast8_t prescaler;
    uint_fast8_t backlight_duty_cycle;

    ssd1963_write_command (SSD1963_GET_PWM_CONF);

    pwmf                    = ssd1963_read_data ();
    pwm                     = ssd1963_read_data ();
    pwm_conf_mask           = ssd1963_read_data ();
    man_br                  = ssd1963_read_data ();
    min_br                  = ssd1963_read_data ();
    prescaler               = ssd1963_read_data ();
    backlight_duty_cycle    = ssd1963_read_data ();

    *pwmfp                  = pwmf;
    *pwmp                   = pwm;
    *pwm_conf_maskp         = pwm_conf_mask;
    *man_brp                = man_br;
    *min_brp                = min_br;
    *prescalerp             = prescaler;
    *backlight_duty_cyclep  = backlight_duty_cycle;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lcd_gen0 () - Set the rise, fall, period and toggling properties of LCD signal generator 0
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lcd_gen0 (uint_fast8_t lcd_gen0, uint_fast16_t gf0, uint_fast16_t gr0, uint_fast16_t gp0)
{
    ssd1963_write_command (SSD1963_SET_LCD_GEN0);
    ssd1963_write_data (lcd_gen0);
    ssd1963_write_data (gf0);
    ssd1963_write_data (gr0);
    ssd1963_write_data (gp0);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lcd_gen0 () - Get the current settings of LCD signal generator 0
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lcd_gen0 (uint_fast8_t * lcd_gen0p, uint_fast16_t * gf0p, uint_fast16_t * gr0p, uint_fast16_t * gp0p)
{
    uint_fast8_t    lcd_gen0;
    uint_fast16_t   gf0;
    uint_fast16_t   gr0;
    uint_fast16_t   gp0;

    ssd1963_write_command (SSD1963_GET_LCD_GEN0);

    lcd_gen0    = ssd1963_read_data ();

    gf0         = ssd1963_read_data () << 8;
    gf0         |= ssd1963_read_data ();

    gr0         = ssd1963_read_data () << 8;
    gr0         |= ssd1963_read_data ();

    gp0         = ssd1963_read_data () << 8;
    gp0         |= ssd1963_read_data ();

    *lcd_gen0p  = lcd_gen0;
    *gf0p       = gf0;
    *gr0p       = gr0;
    *gp0p       = gp0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lcd_gen1 () - Set the rise, fall, period and toggling properties of LCD signal generator 1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lcd_gen1 (uint_fast8_t lcd_gen1, uint_fast16_t gf1, uint_fast16_t gr1, uint_fast16_t gp1)
{
    ssd1963_write_command (SSD1963_SET_LCD_GEN1);
    ssd1963_write_data (lcd_gen1);
    ssd1963_write_data (gf1);
    ssd1963_write_data (gr1);
    ssd1963_write_data (gp1);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lcd_gen1 () - Get the current settings of LCD signal generator 1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lcd_gen1 (uint_fast8_t * lcd_gen1p, uint_fast16_t * gf1p, uint_fast16_t * gr1p, uint_fast16_t * gp1p)
{
    uint_fast8_t    lcd_gen1;
    uint_fast16_t   gf1;
    uint_fast16_t   gr1;
    uint_fast16_t   gp1;

    ssd1963_write_command (SSD1963_GET_LCD_GEN1);

    lcd_gen1    = ssd1963_read_data ();

    gf1         = ssd1963_read_data () << 8;
    gf1         |= ssd1963_read_data ();

    gr1         = ssd1963_read_data () << 8;
    gr1         |= ssd1963_read_data ();

    gp1         = ssd1963_read_data () << 8;
    gp1         |= ssd1963_read_data ();

    *lcd_gen1p  = lcd_gen1;
    *gf1p       = gf1;
    *gr1p       = gr1;
    *gp1p       = gp1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lcd_gen2 () - Set the rise, fall, period and toggling properties of LCD signal generator 2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lcd_gen2 (uint_fast8_t lcd_gen2, uint_fast16_t gf2, uint_fast16_t gr2, uint_fast16_t gp2)
{
    ssd1963_write_command (SSD1963_SET_LCD_GEN2);
    ssd1963_write_data (lcd_gen2);
    ssd1963_write_data (gf2);
    ssd1963_write_data (gr2);
    ssd1963_write_data (gp2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lcd_gen2 () - Get the current settings of LCD signal generator 2
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lcd_gen2 (uint_fast8_t * lcd_gen2p, uint_fast16_t * gf2p, uint_fast16_t * gr2p, uint_fast16_t * gp2p)
{
    uint_fast8_t    lcd_gen2;
    uint_fast16_t   gf2;
    uint_fast16_t   gr2;
    uint_fast16_t   gp2;

    ssd1963_write_command (SSD1963_GET_LCD_GEN2);

    lcd_gen2    = ssd1963_read_data ();

    gf2         = ssd1963_read_data () << 8;
    gf2         |= ssd1963_read_data ();

    gr2         = ssd1963_read_data () << 8;
    gr2         |= ssd1963_read_data ();

    gp2         = ssd1963_read_data () << 8;
    gp2         |= ssd1963_read_data ();

    *lcd_gen2p  = lcd_gen2;
    *gf2p       = gf2;
    *gr2p       = gr2;
    *gp2p       = gp2;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lcd_gen3 () - Set the rise, fall, period and toggling properties of LCD signal generator 3
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lcd_gen3 (uint_fast8_t lcd_gen3, uint_fast16_t gf3, uint_fast16_t gr3, uint_fast16_t gp3)
{
    ssd1963_write_command (SSD1963_SET_LCD_GEN3);
    ssd1963_write_data (lcd_gen3);
    ssd1963_write_data (gf3);
    ssd1963_write_data (gr3);
    ssd1963_write_data (gp3);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lcd_gen3 () - Get the current settings of LCD signal generator 3
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lcd_gen3 (uint_fast8_t * lcd_gen3p, uint_fast16_t * gf3p, uint_fast16_t * gr3p, uint_fast16_t * gp3p)
{
    uint_fast8_t    lcd_gen3;
    uint_fast16_t   gf3;
    uint_fast16_t   gr3;
    uint_fast16_t   gp3;

    ssd1963_write_command (SSD1963_GET_LCD_GEN3);

    lcd_gen3    = ssd1963_read_data ();

    gf3         = ssd1963_read_data () << 8;
    gf3         |= ssd1963_read_data ();

    gr3         = ssd1963_read_data () << 8;
    gr3         |= ssd1963_read_data ();

    gp3         = ssd1963_read_data () << 8;
    gp3         |= ssd1963_read_data ();

    *lcd_gen3p  = lcd_gen3;
    *gf3p       = gf3;
    *gr3p       = gr3;
    *gp3p       = gp3;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio0_rop () - Set the GPIO0 with respect to the LCD signal generators
 * using ROP operation.  No effect if the GPIO0 is configured as general GPIO.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio0_rop (uint_fast8_t gpio0_source, uint_fast8_t gpio0_rop)
{
    ssd1963_write_command (SSD1963_SET_GPIO0_ROP);
    ssd1963_write_data (gpio0_source);
    ssd1963_write_data (gpio0_rop);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio0_rop () - Get the GPIO0 properties with respect to the LCD signal generators
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio0_rop (uint_fast8_t * gpio0_sourcep, uint_fast8_t * gpio0_ropp)
{
    uint_fast8_t gpio0_source;
    uint_fast8_t gpio0_rop;

    ssd1963_write_command (SSD1963_GET_GPIO0_ROP);
    gpio0_source    = ssd1963_read_data ();
    gpio0_rop       = ssd1963_read_data ();

    *gpio0_sourcep  = gpio0_source;
    *gpio0_ropp     = gpio0_rop;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio1_rop () - Set the GPIO1 with respect to the LCD signal generators
 * using ROP operation.  No effect if the GPIO1 is configured as general GPIO.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio1_rop (uint_fast8_t gpio1_source, uint_fast8_t gpio1_rop)
{
    ssd1963_write_command (SSD1963_SET_GPIO1_ROP);
    ssd1963_write_data (gpio1_source);
    ssd1963_write_data (gpio1_rop);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio1_rop () - Get the GPIO1 properties with respect to the LCD signal generators
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio1_rop (uint_fast8_t * gpio1_sourcep, uint_fast8_t * gpio1_ropp)
{
    uint_fast8_t gpio1_source;
    uint_fast8_t gpio1_rop;

    ssd1963_write_command (SSD1963_GET_GPIO1_ROP);
    gpio1_source    = ssd1963_read_data ();
    gpio1_rop       = ssd1963_read_data ();

    *gpio1_sourcep  = gpio1_source;
    *gpio1_ropp     = gpio1_rop;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio2_rop () - Set the GPIO2 with respect to the LCD signal generators
 * using ROP operation.  No effect if the GPIO2 is configured as general GPIO.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio2_rop (uint_fast8_t gpio2_source, uint_fast8_t gpio2_rop)
{
    ssd1963_write_command (SSD1963_SET_GPIO2_ROP);
    ssd1963_write_data (gpio2_source);
    ssd1963_write_data (gpio2_rop);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio2_rop () - Get the GPIO2 properties with respect to the LCD signal generators
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio2_rop (uint_fast8_t * gpio2_sourcep, uint_fast8_t * gpio2_ropp)
{
    uint_fast8_t gpio2_source;
    uint_fast8_t gpio2_rop;

    ssd1963_write_command (SSD1963_GET_GPIO2_ROP);
    gpio2_source    = ssd1963_read_data ();
    gpio2_rop       = ssd1963_read_data ();

    *gpio2_sourcep  = gpio2_source;
    *gpio2_ropp     = gpio2_rop;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_gpio3_rop () - Set the GPIO3 with respect to the LCD signal generators
 * using ROP operation.  No effect if the GPIO3 is configured as general GPIO.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_gpio3_rop (uint_fast8_t gpio3_source, uint_fast8_t gpio3_rop)
{
    ssd1963_write_command (SSD1963_SET_GPIO3_ROP);
    ssd1963_write_data (gpio3_source);
    ssd1963_write_data (gpio3_rop);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_gpio3_rop () - Get the GPIO3 properties with respect to the LCD signal generators
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_gpio3_rop (uint_fast8_t * gpio3_sourcep, uint_fast8_t * gpio3_ropp)
{
    uint_fast8_t gpio3_source;
    uint_fast8_t gpio3_rop;

    ssd1963_write_command (SSD1963_GET_GPIO3_ROP);
    gpio3_source    = ssd1963_read_data ();
    gpio3_rop       = ssd1963_read_data ();

    *gpio3_sourcep  = gpio3_source;
    *gpio3_ropp     = gpio3_rop;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_dbc_conf () - Set the dynamic back light configuration
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_dbc_conf (uint_fast8_t dbc_conf)
{
    ssd1963_write_command (SSD1963_SET_DBC_CONF);
    ssd1963_write_data (dbc_conf);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_dbc_conf () - Get the current dynamic back light configuration
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_dbc_conf (uint_fast8_t * dbc_confp)
{
    uint_fast8_t dbc_conf;

    ssd1963_write_command (SSD1963_GET_DBC_CONF);
    dbc_conf        = ssd1963_read_data ();

    *dbc_confp      = dbc_conf;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_dbc_th () - Set the threshold for each level of power saving
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_dbc_th (uint_fast32_t dbc_th1, uint_fast32_t dbc_th2, uint_fast32_t dbc_th3)
{
    ssd1963_write_command (SSD1963_SET_DBC_TH);
    ssd1963_write_data ((dbc_th1 >> 16) & 0xFF);
    ssd1963_write_data ((dbc_th1 >>  8) & 0xFF);
    ssd1963_write_data ((dbc_th1 >>  0) & 0xFF);
    ssd1963_write_data ((dbc_th2 >> 16) & 0xFF);
    ssd1963_write_data ((dbc_th2 >>  8) & 0xFF);
    ssd1963_write_data ((dbc_th2 >>  0) & 0xFF);
    ssd1963_write_data ((dbc_th3 >> 16) & 0xFF);
    ssd1963_write_data ((dbc_th3 >>  8) & 0xFF);
    ssd1963_write_data ((dbc_th3 >>  0) & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_dbc_th () - Get the threshold for each level of power saving
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_dbc_th (uint_fast32_t * dbc_th1p, uint_fast32_t * dbc_th2p, uint_fast32_t * dbc_th3p)
{
    uint_fast32_t dbc_th1;
    uint_fast32_t dbc_th2;
    uint_fast32_t dbc_th3;

    ssd1963_write_command (SSD1963_GET_DBC_TH);

    dbc_th1     = ssd1963_read_data () << 16;
    dbc_th1     |= ssd1963_read_data () << 8;
    dbc_th1     |= ssd1963_read_data () << 0;

    dbc_th2     = ssd1963_read_data () << 16;
    dbc_th2     |= ssd1963_read_data () << 8;
    dbc_th2     |= ssd1963_read_data () << 0;

    dbc_th3     = ssd1963_read_data () << 16;
    dbc_th3     |= ssd1963_read_data () << 8;
    dbc_th3     |= ssd1963_read_data () << 0;

    *dbc_th1p   = dbc_th1;
    *dbc_th2p   = dbc_th2;
    *dbc_th3p   = dbc_th3;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_pll () - Start the PLL.
 * Before the start, the system was operated with the crystal oscillator or clock input
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_pll (uint_fast8_t pll)
{
    ssd1963_write_command (SSD1963_SET_PLL);
    ssd1963_write_data (pll);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_pll_mn () - Set the PLL
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_pll_mn (uint_fast8_t multiplier, uint_fast8_t divider, uint_fast8_t effectuate_mn)
{
    ssd1963_write_command (SSD1963_SET_PLL_MN);
    ssd1963_write_data (multiplier);
    ssd1963_write_data (divider);
    ssd1963_write_data (effectuate_mn);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_pll_mn () - Get the PLL settings
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_pll_mn (uint_fast8_t * multiplierp, uint_fast8_t * dividerp, uint_fast8_t * effectuate_mnp)
{
    uint_fast8_t multiplier;
    uint_fast8_t divider;
    uint_fast8_t effectuate_mn;

    ssd1963_write_command (SSD1963_GET_PLL_MN);
    multiplier      = ssd1963_read_data ();
    divider         = ssd1963_read_data ();
    effectuate_mn   = ssd1963_read_data ();

    *multiplierp    = multiplier;
    *dividerp       = divider;
    *effectuate_mnp = effectuate_mn;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_pll_status () - Get the current PLL status
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_pll_status (uint_fast8_t * pll_statusp)
{
    uint_fast8_t pll_status;

    ssd1963_write_command (SSD1963_GET_PLL_STATUS);
    pll_status      = ssd1963_read_data ();

    *pll_statusp    = pll_status;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_deep_sleep () - Set deep sleep mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_deep_sleep (void)
{
    ssd1963_write_command (SSD1963_SET_DEEP_SLEEP);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_lshift_freq () - Set the LSHIFT (pixel clock) frequency
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_lshift_freq (uint_fast32_t lcdc_fpr)
{
    ssd1963_write_command (SSD1963_SET_LSHIFT_FREQ);
    ssd1963_write_data ((lcdc_fpr >> 16) & 0xFF);
    ssd1963_write_data ((lcdc_fpr >>  8) & 0xFF);
    ssd1963_write_data ((lcdc_fpr >>  0) & 0xFF);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_lshift_freq () - Get current LSHIFT (pixel clock) frequency setting
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_lshift_freq (uint_fast32_t * lcdc_fprp)
{
    uint_fast32_t lcdc_fpr;

    ssd1963_write_command (SSD1963_GET_LSHIFT_FREQ);
    lcdc_fpr    = ssd1963_read_data () << 16;
    lcdc_fpr    |= ssd1963_read_data () << 8;
    lcdc_fpr    |= ssd1963_read_data () << 0;

    *lcdc_fprp  = lcdc_fpr;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_pixel_data_interface () - Set the pixel data format of the parallel host processor interface
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_pixel_data_interface (uint_fast8_t pixel_data_interface)
{
    ssd1963_write_command (SSD1963_SET_PIXEL_DATA_INTERFACE);
    ssd1963_write_data (pixel_data_interface);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_get_pixel_data_interface () - Get the current pixel data format settings
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_get_pixel_data_interface (uint_fast8_t * pixel_data_interfacep)
{
    uint_fast8_t pixel_data_interface;

    ssd1963_write_command (SSD1963_GET_PIXEL_DATA_INTERFACE);
    pixel_data_interface    = ssd1963_read_data ();

    *pixel_data_interfacep  = pixel_data_interface;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_init_address_mode () - init address mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
ssd1963_init_address_mode (void)
{
    uint_fast8_t    address_mode;

    address_mode    =   SSD1963_ADDRESS_MODE_PAGE_ADDRESS_ORDER_TOP_TO_BOTTOM               |
                        SSD1963_ADDRESS_MODE_COLUMN_ADDRESS_ORDER_LEFT_TO_RIGHT             |
                        SSD1963_ADDRESS_MODE_PAGE_COLUMN_ORDER_NORMAL_MODE                  |
                        SSD1963_ADDRESS_MODE_LINE_ADDRESS_ORDER_LCD_REFRESH_TOP_TO_BOTTOM;

    if (ssd1963.flags & SSD1963_GLOBAL_FLAGS_RGB_ORDER)
    {
        address_mode |= SSD1963_ADDRESS_MODE_RGB_BGR_ORDER_RGB;
    }
    else
    {
        address_mode |= SSD1963_ADDRESS_MODE_RGB_BGR_ORDER_BGR;
    }

    if (ssd1963.flags & SSD1963_GLOBAL_FLAGS_FLIP_HORIZONTAL)
    {
        address_mode |= SSD1963_ADDRESS_MODE_FLIP_HORIZONTAL_ON;
    }
    else
    {
        address_mode |= SSD1963_ADDRESS_MODE_FLIP_HORIZONTAL_OFF;
    }

    if (ssd1963.flags & SSD1963_GLOBAL_FLAGS_FLIP_VERTICAL)
    {
        address_mode |= SSD1963_ADDRESS_MODE_FLIP_VERTICAL_ON;
    }
    else
    {
        address_mode |= SSD1963_ADDRESS_MODE_FLIP_VERTICAL_OFF;
    }

    ssd1963_set_address_mode (address_mode);
}

#if 0
uint_fast8_t
ssd1963_read_flags_from_eep (void)
{
    uint_fast8_t    rtc = 0;
    uint8_t         flags;

    if (eep_is_up &&
        eep_read (EEPROM_DATA_OFFSET_SSD1963_FLAGS, &flags, EEPROM_DATA_SIZE_SSD1963_FLAGS))
    {
        ssd1963.flags = flags & SSD1963_GLOBAL_FLAGS_MASK;

        if (ssd1963.flags != flags)                                                     // some unused bits set?
        {                                                                               // yes, repair flags in EEPROM
            ssd1963_write_flags_to_eep ();
        }

        rtc = 1;
    }

    return rtc;
}

uint_fast8_t
ssd1963_write_flags_to_eep (void)
{
    uint_fast8_t    rtc = 0;
    uint8_t         flags = ssd1963.flags;

    if (eep_is_up &&
        eep_write (EEPROM_DATA_OFFSET_SSD1963_FLAGS, &flags, EEPROM_DATA_SIZE_SSD1963_FLAGS))
    {
        rtc = 1;
    }

    return rtc;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_set_flags ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
ssd1963_set_flags (uint_fast8_t new_flags)
{
    ssd1963.flags = new_flags;
    ssd1963_init_address_mode ();
}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963_init ()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define PLL_MULTIPLIER      (36 - 1)                                                    // PLLclk = REFclk * 36 (360MHz)
#define PLL_DIVIDER         ( 3 - 1)                                                    // SYSclk = PLLclk /  3 (120MHz)
#define LCDC_FPR            (300000)

#define HSYNC_HT            1000                                                        // = (TFT_WIDTH + HSYNC_PULSE + HSYNC_BACK_PORCH + HSYNC_FRONT_PORCH)
#define HSYNC_HPS             51
#define HSYNC_HPW              8
#define HSYNC_LPS              3
#define HSYNC_LPSPP            0

#define VSYNC_VT             530                                                        // = (TFT_HEIGHT + VSYNC_PULSE + VSYNC_BACK_PORCH + VSYNC_FRONT_PORCH)
#define VSYNC_VPS             24
#define VSYNC_VPW              3
#define VSYNC_FPS             23

void
ssd1963_init ()
{
    uint_fast8_t    lcd_mode_a;
    uint_fast8_t    lcd_mode_b;
    uint_fast8_t    seq;

    ssd1963_reset_ctrl_lines ();                                                                                    // control lines as input
    ssd1963_init_ctrl_lines ();
    ssd1963_init_fsmc ();
    ssd1963_soft_reset ();

    ssd1963_set_pll_mn (PLL_MULTIPLIER, PLL_DIVIDER, SSD1963_PLL_MN_EFFECTUATE_MULTIPLIER_AND_DIVIDER);
    ssd1963_set_pll (SSD1963_PLL_LOCK_USE_REFERENCE_CLOCK | SSD1963_PLL_LOCK_ENABLE_PLL);                           // setup PLL first
    delay_usec (100);                                                                                               // then wait 100 us
    ssd1963_set_pll (SSD1963_PLL_LOCK_USE_PLL_OUTPUT | SSD1963_PLL_LOCK_ENABLE_PLL);                                // start to lock PLL
    ssd1963_set_lshift_freq (LCDC_FPR);

    lcd_mode_a =    SSD1963_LCD_MODE_A_PANEL_DATA_WIDTH_18_BIT                              |                       // 0
                    SSD1963_LCD_MODE_A_COLOR_DEPTH_ENHANCEMENT_DISABLE                      |                       // 0
                    SSD1963_LCD_MODE_A_TFT_FRC_ENABLE_DITHERING                             |                       // 0
                    SSD1963_LCD_MODE_A_LSHIFT_POLARITY_DATA_LATCH_IN_FALLING_EDGE           |                       // 0
                    SSD1963_LCD_MODE_A_LLINE_POLARITY_ACTIVE_LOW                            |                       // 0
                    SSD1963_LCD_MODE_A_LFRAME_POLARITY_ACTIVE_LOW;                                                  // 0 = 0x00

    lcd_mode_b =    SSD1963_LCD_MODE_B_TFT_TYPE_TFT_00_MODE;                                                        // 00 = 0x00

    seq        =    SSD1963_LCD_MODE_SEQ_EVEN_LINE_RGB                                      |                       // 00
                    SSD1963_LCD_MODE_SEQ_ODD_LINE_RGB;                                                              // 00 = 0x00

    ssd1963_set_lcd_mode (lcd_mode_a, lcd_mode_b, TFT_WIDTH - 1, TFT_HEIGHT - 1, seq);                              // 0x0C, 00, 839, 399, 0x00

    ssd1963_set_hori_period (HSYNC_HT, HSYNC_HPS, HSYNC_HPW, HSYNC_LPS, HSYNC_LPSPP);
    ssd1963_set_vert_period (VSYNC_VT, VSYNC_VPS, VSYNC_VPW, VSYNC_FPS);

    ssd1963_set_pixel_data_interface (SSD1963_PIXEL_DATA_INTERFACE_16_BIT_565);
    ssd1963_set_display_on ();
    ssd1963_init_address_mode ();

//  ssd1963_set_pwm_conf (0x06, 0x00, 0x01, 0xf0, 0x00, 0x00);                                          // backlight @ 0x00
//  ssd1963_set_pwm_conf (0x06, 0xD9, 0x01, 0xF0, 0x00, 0x00);                                          // brightness @ 100%

    ssd1963_set_pwm_conf (0x06,                                                                         // pwmf, pwm frequency
                          0xFF,                                                                         // pwm duty cycle = 100%
                          SSD1963_PWM_CONF_PWM_CONTROLLED_BY_HOST | SSD1963_PWM_CONF_PWM_ENABLED,       // pwm controlled by host, enabled
                          0xFF,                                                                         // manual brightness = 100%
                          0x00,                                                                         // minimum brightness = 0%
                          SSD1963_PWM_CONF_PRESCALER_OFF);                                              // no prescaler
}

#else                                                                                                   // ISO C forbids an empty translation unit
int ssd193 = 0;
#endif // SSD1963
