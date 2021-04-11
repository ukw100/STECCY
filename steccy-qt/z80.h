/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80.h - definitions for Z80 emulator STECCY
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
#ifndef Z80_H
#define Z80_H
#include <stdint.h>

#ifdef QT_CORE_LIB                                                  // C++
#define CHAR(x)             char(x)
#define INT8_T(x)           int8_t(x)
#define UINT8_T(x)          uint8_t(x)
#define INT_FAST8_T(x)      int_fast8_t(x)
#define UINT_FAST8_T(x)     uint_fast8_t(x)
#define UINT16_T(x)         uint16_t(x)
#define UINT32_T(x)         uint32_t(x)
#else                                                               // pure C
#define CHAR(x)             ((char) (x))
#define INT8_T(x)           ((int8_t) (x))
#define UINT8_T(x)          ((uint8_t) (x))
#define INT_FAST8_T(x)      ((int_fast8_t) (x))
#define UINT_FAST8_T(x)     ((uint_fast8_t) (x))
#define UINT16_T(x)         ((uint16_t) (x))
#define UINT32_T(x)         ((uint32_t) (x))
#define nullptr             ((void *) 0)
#endif

// high nibble: address line, low nibble: column
#define MATRIX_KEY_SHIFT_IDX        0x00
#define MATRIX_KEY_Z_IDX            0x01
#define MATRIX_KEY_X_IDX            0x02
#define MATRIX_KEY_C_IDX            0x03
#define MATRIX_KEY_V_IDX            0x04

#define MATRIX_KEY_A_IDX            0x10
#define MATRIX_KEY_S_IDX            0x11
#define MATRIX_KEY_D_IDX            0x12
#define MATRIX_KEY_F_IDX            0x13
#define MATRIX_KEY_G_IDX            0x14

#define MATRIX_KEY_Q_IDX            0x20
#define MATRIX_KEY_W_IDX            0x21
#define MATRIX_KEY_E_IDX            0x22
#define MATRIX_KEY_R_IDX            0x23
#define MATRIX_KEY_T_IDX            0x24

#define MATRIX_KEY_1_IDX            0x30
#define MATRIX_KEY_2_IDX            0x31
#define MATRIX_KEY_3_IDX            0x32
#define MATRIX_KEY_4_IDX            0x33
#define MATRIX_KEY_5_IDX            0x34

#define MATRIX_KEY_0_IDX            0x40
#define MATRIX_KEY_9_IDX            0x41
#define MATRIX_KEY_8_IDX            0x42
#define MATRIX_KEY_7_IDX            0x43
#define MATRIX_KEY_6_IDX            0x44

#define MATRIX_KEY_P_IDX            0x50
#define MATRIX_KEY_O_IDX            0x51
#define MATRIX_KEY_I_IDX            0x52
#define MATRIX_KEY_U_IDX            0x53
#define MATRIX_KEY_Y_IDX            0x54

#define MATRIX_KEY_ENTER_IDX        0x60
#define MATRIX_KEY_L_IDX            0x61
#define MATRIX_KEY_K_IDX            0x62
#define MATRIX_KEY_J_IDX            0x63
#define MATRIX_KEY_H_IDX            0x64

#define MATRIX_KEY_SPACE_IDX        0x70
#define MATRIX_KEY_SYM_IDX          0x71
#define MATRIX_KEY_M_IDX            0x72
#define MATRIX_KEY_N_IDX            0x73
#define MATRIX_KEY_B_IDX            0x74

#define MATRIX_KEMPSTON_RIGHT_IDX   0x80
#define MATRIX_KEMPSTON_LEFT_IDX    0x81
#define MATRIX_KEMPSTON_DOWN_IDX    0x82
#define MATRIX_KEMPSTON_UP_IDX      0x83
#define MATRIX_KEMPSTON_FIRE_IDX    0x84

/*------------------------------------------------------------------------------------------------------------------------
 * Z80 emulator settings:
 *------------------------------------------------------------------------------------------------------------------------
*/
// possible flags for keyboard:
#if defined STM32F4XX
#define MAX_FILENAME_LEN        64
#else
#define MAX_FILENAME_LEN        128
#endif

// possible flags for keyboard:
#define KEYBOARD_NONE           0x00
#define KEYBOARD_PS2            0x01
#define KEYBOARD_USB            0x02
#define KEYBOARD_ZX             0x04

typedef struct
{
#ifndef STM32F4XX
    char                        path[MAX_FILENAME_LEN];                         // search path for files
#endif
    char                        romfile[MAX_FILENAME_LEN];                      // ROM file to load
    uint8_t                     autostart;                                      // flag: autostart BASIC programs
    char                        autoload[MAX_FILENAME_LEN];                     // TAPE file to autoload
    uint_fast8_t                keyboard;                                       // keyboard type, can be combined
    uint8_t                     rgb_order;                                      // rgb_order
    uint8_t                     orientation;                                    // orientation
    uint_fast8_t                turbo_mode;                                     // turbo mode
} Z80_SETTINGS;

extern Z80_SETTINGS             z80_settings;
extern uint_fast16_t            z80_romsize;

/*------------------------------------------------------------------------------------------------------------------------
 * Some flags
 *------------------------------------------------------------------------------------------------------------------------
*/
#if defined STM32F4XX
extern volatile uint_fast8_t    update_display;
extern volatile uint32_t        uptime;                                 // uptime in units of 20us
#elif defined FRAMEBUFFER || defined X11
extern volatile                 uint_fast8_t steccy_exit;
extern uint_fast8_t             steccy_uses_x11;
extern uint_fast8_t             z80_display_cached;
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * QT specific stuff
 *------------------------------------------------------------------------------------------------------------------------
*/
#ifdef QT_CORE_LIB
#include <QThread>

class z80Thread : public QThread
{
    Q_OBJECT
private:
    void run();
};

extern int              zx_spectrum (int argc, char **argv);

#else
extern void             zx_spectrum (void);
extern void             z80 (void);
#endif

/*------------------------------------------------------------------------------------------------------------------------
 * Public functions
 *------------------------------------------------------------------------------------------------------------------------
*/
extern void             z80_reset (void);
extern void             z80_pause (void);
extern void             z80_leave_focus (void);
extern void             z80_next_display_orientation (void);
extern void             z80_next_display_rgb_order (void);
extern void             z80_next_turbo_mode (void);
extern void             z80_load_rom (const char * fname);
extern void             z80_set_fname_load (const char * fname);
extern void             z80_close_fname_load (void);
extern void             z80_set_fname_save (const char * fname);
extern void             z80_close_fname_save (void);
extern void             z80_set_fname_save_snapshot (const char * fname);
extern void             z80_set_autostart (uint8_t do_set);
extern uint8_t          z80_get_autostart (void);
extern char *           z80_get_path (void);
extern void             z80_press_key (uint8_t kb_idx);
extern void             z80_release_key (uint8_t kb_idx);

#endif // Z80_H
