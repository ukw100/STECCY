/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ssd1963.h - definitions/declarations of SSD1963 routines
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
#ifndef SSD1963_H
#define SSD1963_H

#ifdef unix
#include <stdio.h>
#endif

#include <stdint.h>
#include "ssd1963-config.h"

#ifdef STM32F10X
#  include "stm32f10x.h"
#  include "stm32f10x_gpio.h"
#  include "stm32f10x_rcc.h"
#elif defined STM32F4XX
#  include "stm32f4xx.h"
#  include "stm32f4xx_gpio.h"
#  include "stm32f4xx_rcc.h"
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SSD1963 API:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define TFT_REG                                 (*((volatile uint16_t *) 0x60000000))
#define TFT_RAM                                 (*((volatile uint16_t *) 0x60080000))   // 0x80000 for FSMC18 = /RD (SSD1963: D/C)

#define ssd1963_write_command(cmd)              do { TFT_REG = (cmd); } while (0)
#define ssd1963_write_data(val)                 do { TFT_RAM = (val); } while (0)
#define ssd1963_read_data()                     (TFT_RAM)

#define SSD1963_GLOBAL_FLAGS_RGB_ORDER          0x01
#define SSD1963_GLOBAL_FLAGS_FLIP_HORIZONTAL    0x02
#define SSD1963_GLOBAL_FLAGS_FLIP_VERTICAL      0x04

#define SSD1963_GLOBAL_FLAGS_MASK               0x07

typedef struct
{
    uint_fast8_t         flags;
} SSD1963_GLOBALS;

extern SSD1963_GLOBALS  ssd1963;

extern void             ssd1963_hard_reset (void);
extern void             ssd1963_nop (void);
extern void             ssd1963_soft_reset (void);
extern void             ssd1963_get_power_mode (uint_fast8_t *);
extern void             ssd1963_get_address_mode (uint_fast8_t *);
extern void             ssd1963_get_display_mode (uint_fast8_t *);
extern void             ssd1963_get_tear_effect_status (uint_fast8_t *);
extern void             ssd1963_enter_sleep_mode (void);
extern void             ssd1963_exit_sleep_mode (void);
extern void             ssd1963_enter_partial_mode (void);
extern void             ssd1963_enter_normal_mode (void);
extern void             ssd1963_exit_invert_mode (void);
extern void             ssd1963_enter_invert_mode (void);
extern void             ssd1963_set_gamma_curve (uint_fast8_t);
extern void             ssd1963_set_display_off (void);
extern void             ssd1963_set_display_on (void);
extern void             ssd1963_set_column_address (uint_fast16_t, uint_fast16_t);
extern void             ssd1963_set_page_address (uint_fast16_t, uint_fast16_t);
extern void             ssd1963_write_memory_start (void);
extern void             ssd1963_read_memory_start (void);
extern void             ssd1963_set_partial_area (uint_fast16_t, uint_fast16_t);
extern void             ssd1963_set_scroll_area (uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             ssd1963_set_tear_off (void);
extern void             ssd1963_set_tear_on (uint_fast8_t);
extern void             ssd1963_set_address_mode (uint_fast8_t);
extern void             ssd1963_set_scroll_start (uint_fast16_t);
extern void             ssd1963_exit_idle_mode (void);
extern void             ssd1963_enter_idle_mode (void);
extern void             ssd1963_write_memory_continue (void);
extern void             ssd1963_read_memory_continue (void);
extern void             ssd1963_set_tear_scanline (uint_fast16_t);
extern void             ssd1963_get_scanline (uint_fast16_t *);
extern void             ssd1963_read_ddb (uint_fast16_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_lcd_mode (uint_fast8_t, uint_fast8_t, uint_fast16_t, uint_fast16_t, uint_fast8_t);
extern void             ssd1963_get_lcd_mode (uint_fast8_t *, uint_fast8_t *, uint_fast16_t *, uint_fast16_t *, uint_fast8_t *);
extern void             ssd1963_set_hori_period (uint_fast16_t, uint_fast16_t, uint_fast8_t, uint_fast16_t, uint_fast8_t);
extern void             ssd1963_get_hori_period (uint_fast16_t *, uint_fast16_t *, uint_fast8_t *, uint_fast16_t *, uint_fast8_t *);
extern void             ssd1963_set_vert_period (uint_fast16_t, uint_fast16_t, uint_fast8_t, uint_fast16_t);
extern void             ssd1963_get_vert_period (uint_fast16_t *, uint_fast16_t *, uint_fast8_t *, uint_fast16_t *);
extern void             ssd1963_set_gpio_conf (uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_gpio_conf (uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_gpio_value (uint_fast8_t);
extern void             ssd1963_get_gpio_status (uint_fast8_t *);
extern void             ssd1963_set_post_proc (uint_fast8_t, uint_fast8_t, uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_post_proc (uint_fast8_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_pwm_conf (uint_fast8_t, uint_fast8_t, uint_fast8_t, uint_fast8_t, uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_pwm_conf (uint_fast8_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_lcd_gen0 (uint_fast8_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             ssd1963_get_lcd_gen0 (uint_fast8_t *, uint_fast16_t *, uint_fast16_t *, uint_fast16_t *);
extern void             ssd1963_set_lcd_gen1 (uint_fast8_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             ssd1963_get_lcd_gen1 (uint_fast8_t *, uint_fast16_t *, uint_fast16_t *, uint_fast16_t *);
extern void             ssd1963_set_lcd_gen2 (uint_fast8_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             ssd1963_get_lcd_gen2 (uint_fast8_t *, uint_fast16_t *, uint_fast16_t *, uint_fast16_t *);
extern void             ssd1963_set_lcd_gen3 (uint_fast8_t, uint_fast16_t, uint_fast16_t, uint_fast16_t);
extern void             ssd1963_get_lcd_gen3 (uint_fast8_t *, uint_fast16_t *, uint_fast16_t *, uint_fast16_t *);
extern void             ssd1963_set_gpio0_rop (uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_gpio0_rop (uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_gpio1_rop (uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_gpio1_rop (uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_gpio2_rop (uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_gpio2_rop (uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_gpio3_rop (uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_gpio3_rop (uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_set_dbc_conf (uint_fast8_t);
extern void             ssd1963_get_dbc_conf (uint_fast8_t *);
extern void             ssd1963_set_dbc_th (uint_fast32_t, uint_fast32_t, uint_fast32_t);
extern void             ssd1963_get_dbc_th (uint_fast32_t *, uint_fast32_t *, uint_fast32_t *);
extern void             ssd1963_set_pll (uint_fast8_t);
extern void             ssd1963_set_pll_mn (uint_fast8_t, uint_fast8_t, uint_fast8_t);
extern void             ssd1963_get_pll_mn (uint_fast8_t *, uint_fast8_t *, uint_fast8_t *);
extern void             ssd1963_get_pll_status (uint_fast8_t *);
extern void             ssd1963_set_deep_sleep (void);
extern void             ssd1963_set_lshift_freq (uint_fast32_t);
extern void             ssd1963_get_lshift_freq (uint_fast32_t *);
extern void             ssd1963_set_pixel_data_interface (uint_fast8_t);
extern void             ssd1963_get_pixel_data_interface (uint_fast8_t *);
extern void             ssd1963_set_flags (uint_fast8_t);

extern uint_fast8_t     ssd1963_read_flags_from_eep (void);
extern uint_fast8_t     ssd1963_write_flags_to_eep (void);

extern void             ssd1963_init (void);

#endif // SSD1963_H
