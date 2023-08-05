/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80.c - Z80 emulator functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * This file is usable for STM32 version (z80.c) and QT version (z80.cpp) of STECCY.
 *
 * Links:
 *  http://clrhome.org/table/
 *  http://z80-heaven.wikidot.com/instructions-set
 *  http://slady.net/Sinclair-ZX-Spectrum-keyboard/layout/
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "z80.h"
#include "zxram.h"
#include "zxscr.h"
#include "zxio.h"
#include "tape.h"

#if defined STM32F4XX
#include "ps2key.h"
#include "zxkbd.h"
#include "joystick.h"
#include "usb_hid_host.h"
#include "delay.h"
#include "console.h"
#include "menu.h"
volatile uint_fast8_t       update_display;
volatile uint32_t           uptime;
#elif defined FRAMEBUFFER || defined X11
#include <unistd.h>
#include <time.h>
#if defined FRAMEBUFFER
#include "lxfb.h"
#else // X11
#include "lxx11.h"
#endif
#include "lxmenu.h"
volatile uint_fast8_t       steccy_exit = 0;
static uint_fast8_t         update_display;
#endif

#define TRUE                1
#define FALSE               0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Force inlining:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef STM32F4XX                                                    // always inline functions on STM32F4
#define FORCE_INLINING      1
#else                                                               // no need to force inlining, any desktop PC is fast enough
#define FORCE_INLINING      0
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Debugging:
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#undef DEBUG
#undef DEBUG_ONLY_RAM

#ifdef DEBUG
#define INLINE                                                      // don't inline in DEBUG mode
#else
#if FORCE_INLINING == 1
#  define INLINE inline     __attribute__((always_inline))          // force compiler to inline
#else
#  define INLINE
#endif
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 emulator settings:
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
Z80_SETTINGS                        z80_settings;
uint_fast16_t                       z80_romsize;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * STECCY Hooks
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define STECCY_HOOK_ADDRESS         0x386E                          // free space in SINCLAIR ROM: 0x386E - 0x3CFF
#define SERIAL_OUTPUT               0x3CFE
#define SERIAL_INPUT                0x3CFF
static int                          hooks_active = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * T-states in the upper 32KB RAM (no ULA access to RAM) or ROM
 *
 * This emulator should calculate 33588 T-states for 10 msec.
 *
 * If some clock cycle values must be corrected below, then you
 * should rerun the Program "checktimes" in checktimes.tzx.
 * This program should display the value "1886" as result.
 *
 * Assembler listing:
 *
 *          ORG 8000h
 *          HALT
 *          LD BC,0
 *          LD HL,0
 *          LD   (5C78h),HL
 *          LD   (5C79h),HL
 *          LD   E,0
 *          LD   A,FF
 *          LD   HL,5C78h
 *  nxt:    INC  E
 *          JR   NZ,skip
 *          INC  BC
 *  skip:   CP   A,(HL)
 *          JR   NZ,nxt
 *          RET
 *
 * BC holds now the value 1886.
 *
 * ZX Spectrum Basic Program:
 *
 *     10 CLEAR 30000
 *     20 FOR i=32768 TO 32795
 *     30 READ a
 *     40 POKE i,a
 *     50 NEXT i
 *     60 PRINT "Calculating Timing"
 *     70 PRINT "Result should be: 1886"
 *     80 PRINT "Result is: ";
 *     90 PRINT USR 32768
 *    200 DATA 118,1,0,0,33,0,0
 *    210 DATA 34,120,92,34,121,92
 *    220 DATA 30,0,62,255,33,120,92
 *    230 DATA 28,32,1,3,190,32,249
 *    250 DATA 201
 *
 * This program measures the speed of incrementation of the BC register against the time spent in ZX ROM interrupt
 * routine triggered by ULA. The algorithm has been found in a ZX Spectrum game. Perhaps the game wanted to
 * determine if running on Timex or ZX-Spectrum (60Hz/50Hz). Or the debugging of the program should be prevented.
 *
 * On the STM32F4XX, the clock must be synchronised at short intervals for the speaker to produce correct frequencies.
 * Here we sync the clock each 200 usec, so we can update the speaker with a frequency of max. 5 kHz.
 * On other platforms we sync the clock each 10 msec.
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#if defined STM32F4XX
#define CLOCKCYCLES_PER_200_USEC      672                                   // 672 (75%) and 671 (25%)
#define CLOCKCYCLES_COUNT_200_USEC    100                                   // 100 * 200 usec = 20 msec = 50Hz
#else
#define CLOCKCYCLES_PER_10_MSEC     33588                                   // 33588 (33580 - 33596)
#define CLOCKCYCLES_COUNT_10_MSEC       2                                   //   2 *  10 msec = 20 msec = 50Hz
#endif

#if defined QT_CORE_LIB
#include "mywindow.h"
QElapsedTimer elapsed_timer;
#define SLEEP_MSEC                  10                                      // QT on PC
#elif defined FRAMEBUFFER || defined X11
#define SLEEP_USEC                  10000                                   // Linux
#elif defined (STM32F4XX)
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Debugging Macros
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#ifdef DEBUG
static uint8_t debug = 0;
#define debug_printf(...)           do { if (debug /* && iff1 */) { printf(__VA_ARGS__); fflush (stdout); }} while (0)
#define debug_putchar(c)            do { if (debug /* && iff1 */) { putchar(c); fflush (stdout); }} while (0)
#else
#define debug_printf(...)
#define debug_putchar(c)
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 flags
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define FLAG_IDX_C      0                                                   // CARRY - must be 1
#define FLAG_IDX_N      1                                                   // ADD (0) or SUBTRACT (1)
#define FLAG_IDX_PV     2                                                   // PARITY (count of 1) or OVERFLOW (even: set to 1, odd: set to 0)
#define FLAG_IDX_X1     3                                                   // not used
#define FLAG_IDX_H      4                                                   // HALF CARRY (carry from bit 3 into bit 4)
#define FLAG_IDX_X2     5                                                   // not used
#define FLAG_IDX_Z      6                                                   // ZERO
#define FLAG_IDX_S      7                                                   // SIGN

#define FLAG_C          (1 << FLAG_IDX_C)                                   // CARRY - must be 1
#define FLAG_N          (1 << FLAG_IDX_N)                                   // ADD (0) or SUBTRACT (1)
#define FLAG_PV         (1 << FLAG_IDX_PV)                                  // PARITY (count of 1) or OVERFLOW (even: set to 1, odd: set to 0)
#define FLAG_X1         (1 << FLAG_IDX_X1)                                  // not used
#define FLAG_H          (1 << FLAG_IDX_H)                                   // HALF CARRY (carry from bit 3 into bit 4)
#define FLAG_X2         (1 << FLAG_IDX_X2)                                  // not used
#define FLAG_Z          (1 << FLAG_IDX_Z)                                   // ZERO
#define FLAG_S          (1 << FLAG_IDX_S)                                   // SIGN

#define SET_FLAG_C()    do { reg_F |= FLAG_C;  } while (0)                  // set C
#define SET_FLAG_N()    do { reg_F |= FLAG_N;  } while (0)                  // set N
#define SET_FLAG_PV()   do { reg_F |= FLAG_PV; } while (0)                  // set P/V
#define SET_FLAG_X1()   do { reg_F |= FLAG_X1; } while (0)                  // set X1 (not used)
#define SET_FLAG_H()    do { reg_F |= FLAG_H;  } while (0)                  // set H
#define SET_FLAG_X2()   do { reg_F |= FLAG_X2; } while (0)                  // set X2 (not used)
#define SET_FLAG_Z()    do { reg_F |= FLAG_Z;  } while (0)                  // set Z
#define SET_FLAG_S()    do { reg_F |= FLAG_S;  } while (0)                  // set s

#define RES_FLAG_C()    do { reg_F &= ~FLAG_C;  } while (0)                 // reset C
#define RES_FLAG_N()    do { reg_F &= ~FLAG_N;  } while (0)                 // reset N
#define RES_FLAG_PV()   do { reg_F &= ~FLAG_PV; } while (0)                 // reset P/V
#define RES_FLAG_X1()   do { reg_F &= ~FLAG_X1; } while (0)                 // reset X1 8not used)
#define RES_FLAG_H()    do { reg_F &= ~FLAG_H;  } while (0)                 // reset H
#define RES_FLAG_X2()   do { reg_F &= ~FLAG_X2; } while (0)                 // reset X2 (not used)
#define RES_FLAG_Z()    do { reg_F &= ~FLAG_Z;  } while (0)                 // reset Z
#define RES_FLAG_S()    do { reg_F &= ~FLAG_S;  } while (0)                 // reset S

#define ISSET_FLAG_C()  (reg_F & FLAG_C)                                    // check C
#define ISSET_FLAG_N()  (reg_F & FLAG_N)                                    // check N
#define ISSET_FLAG_PV() (reg_F & FLAG_PV)                                   // check P/V
#define ISSET_FLAG_X1() (reg_F & FLAG_X1)                                   // check X1 (not used)
#define ISSET_FLAG_H()  (reg_F & FLAG_H)                                    // check H
#define ISSET_FLAG_X2() (reg_F & FLAG_X2)                                   // check X2
#define ISSET_FLAG_Z()  (reg_F & FLAG_Z)                                    // check Z
#define ISSET_FLAG_S()  (reg_F & FLAG_S)                                    // check S

#define NEG_FLAG_TRUE   1
#define NEG_FLAG_FALSE  0

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 conditions
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define COND_C          NEG_FLAG_FALSE, FLAG_IDX_C                          // condition: carry set
#define COND_NC         NEG_FLAG_TRUE,  FLAG_IDX_C                          // condition: carry not set
#define COND_N          NEG_FLAG_FALSE, FLAG_IDX_N                          // condition: subtract
#define COND_NN         NEG_FLAG_TRUE,  FLAG_IDX_N                          // condition: add
#define COND_PE         NEG_FLAG_FALSE, FLAG_IDX_PV                         // condition: even parity set
#define COND_PO         NEG_FLAG_TRUE,  FLAG_IDX_PV                         // condition: even parity not set (odd)
#define COND_H          NEG_FLAG_FALSE, FLAG_IDX_H                          // condition: half carry set
#define COND_NH         NEG_FLAG_TRUE,  FLAG_IDX_H                          // condition: half carry not set
#define COND_Z          NEG_FLAG_FALSE, FLAG_IDX_Z                          // condition: zero flag set
#define COND_NZ         NEG_FLAG_TRUE,  FLAG_IDX_Z                          // condition: zero flag not set
#define COND_M          NEG_FLAG_FALSE, FLAG_IDX_S                          // condition: sign flag set: minus
#define COND_P          NEG_FLAG_TRUE,  FLAG_IDX_S                          // condition: sign flag not set: plus

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 registers
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define REG_IND_HL_POS  6                                                   // position of HL register in BITS opcode

#define REG_IDX_B       0                                                   // index of reg B
#define REG_IDX_C       1                                                   // index of reg C
#define REG_IDX_D       2                                                   // index of reg D
#define REG_IDX_E       3                                                   // index of reg E
#define REG_IDX_H       4                                                   // index of reg H
#define REG_IDX_L       5                                                   // index of reg L
#define REG_IDX_F       6                                                   // index of reg F
#define REG_IDX_A       7                                                   // index of reg A, must be 7 to decode opcodes!
#define REG_IDX_IXH     8                                                   // index of reg IXH
#define REG_IDX_IXL     9                                                   // index of reg IXL
#define REG_IDX_IYH     10                                                  // index of reg IYH
#define REG_IDX_IYL     11                                                  // index of reg IYL
#define N_REGS          12                                                  // number of registers

static uint8_t          z80_regs[N_REGS];                                   // main registers
static uint8_t          z80_regs2[N_REGS];                                  // shadow registers

#define REG_OFFSET_IX   (REG_IDX_IXH - REG_IDX_H)                           // = 4
#define REG_OFFSET_IY   (REG_IDX_IYH - REG_IDX_H)                           // = 6

#define REG_IDX_BC      0                                                   // index of reg pair BC
#define REG_IDX_DE      1                                                   // index of reg pair DE
#define REG_IDX_HL      2                                                   // index of reg pair HL

#define reg_B           z80_regs[REG_IDX_B]                                 // content of reg B
#define reg_C           z80_regs[REG_IDX_C]                                 // content of reg C
#define reg_D           z80_regs[REG_IDX_D]                                 // content of reg D
#define reg_E           z80_regs[REG_IDX_E]                                 // content of reg E
#define reg_H           z80_regs[REG_IDX_H]                                 // content of reg H
#define reg_L           z80_regs[REG_IDX_L]                                 // content of reg L
#define reg_A           z80_regs[REG_IDX_A]                                 // content of reg A
#define reg_F           z80_regs[REG_IDX_F]                                 // content of reg F
#define reg_IXH         z80_regs[REG_IDX_IXH]                               // content of reg IXH
#define reg_IXL         z80_regs[REG_IDX_IXL]                               // content of reg IXL
#define reg_IYH         z80_regs[REG_IDX_IYH]                               // content of reg IYH
#define reg_IYL         z80_regs[REG_IDX_IYL]                               // content of reg IYL

#define reg_R(ridx)     z80_regs[ridx]                                      // indirect access to register

#define reg_B2          z80_regs2[REG_IDX_B]                                // content of shadow reg B'
#define reg_C2          z80_regs2[REG_IDX_C]                                // content of shadow reg C'
#define reg_D2          z80_regs2[REG_IDX_D]                                // content of shadow reg D'
#define reg_E2          z80_regs2[REG_IDX_E]                                // content of shadow reg E'
#define reg_H2          z80_regs2[REG_IDX_H]                                // content of shadow reg H'
#define reg_L2          z80_regs2[REG_IDX_L]                                // content of shadow reg L'
#define reg_A2          z80_regs2[REG_IDX_A]                                // content of shadow reg A'
#define reg_F2          z80_regs2[REG_IDX_F]                                // content of shadow reg F'

#define reg_R2(ridx)    z80_regs2[ridx]                                     // indirect access to shadow register (not used)

#define reg_BC          UINT16_T (((z80_regs[REG_IDX_BC << 1] << 8) | z80_regs[(REG_IDX_BC << 1) + 1]))     // content of reg pair BC
#define reg_DE          UINT16_T (((z80_regs[REG_IDX_DE << 1] << 8) | z80_regs[(REG_IDX_DE << 1) + 1]))     // content of reg pair DE
#define reg_HL          UINT16_T (((z80_regs[REG_IDX_HL << 1] << 8) | z80_regs[(REG_IDX_HL << 1) + 1]))     // content of reg pair HL
#define reg_RR(rridx)   UINT16_T (((z80_regs[rridx << 1] << 8) | z80_regs[(rridx << 1) + 1]))               // indirect access to reg pair

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * special Z80 registers
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
static uint8_t          interrupt_mode = 0;                                 // current interrupt mode
static uint8_t          reg_I;                                              // interrupt register
static uint8_t          reg_R;                                              // refresh register, yet not used
static uint16_t         reg_SP;                                             // stack pointer

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 register names - used by disassembler in debug mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#ifdef DEBUG
static const char *     z80_r_names[12] =
{
    "B", "C", "D", "E", "H", "L", "(HL)", "A", "IXH", "IXL", "IYH", "IYL",
};

static const char *     z80_rr_names[6] =
{
    "BC", "DE", "HL", "AF", "IX", "IY"
};

static const char *     z80_flagnames[16] =
{
    "C", "N", "PE", "X1", "H", "X2", "Z", "M", "NC", "NN", "PO", "NX1", "NH", "NX2", "NZ", "P"
};
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * register access macros - get
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define GET_A()             (reg_A)
#define GET_F()             (reg_F)
#define GET_B()             (reg_B)
#define GET_C()             (reg_C)
#define GET_D()             (reg_D)
#define GET_E()             (reg_E)
#define GET_H()             (reg_H)
#define GET_L()             (reg_L)
#define GET_IXH()           (reg_IXH)
#define GET_IXL()           (reg_IXL)
#define GET_IYH()           (reg_IYH)
#define GET_IYL()           (reg_IYL)
#define GET_R(ridx)         z80_regs[ridx]

#define GET_BC()            (UINT16_T (((reg_B << 8)   | reg_C)))
#define GET_DE()            (UINT16_T (((reg_D << 8)   | reg_E)))
#define GET_HL()            (UINT16_T (((reg_H << 8)   | reg_L)))
#define GET_IX()            (UINT16_T (((reg_IXH << 8) | reg_IXL)))
#define GET_IY()            (UINT16_T (((reg_IYH << 8) | reg_IYL)))
#define GET_AF()            (UINT16_T (((reg_A << 8)   | reg_F)))

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * register access macros - set
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
#define SET_A(n)            do { reg_A   = UINT8_T ((n)); } while (0)
#define SET_F(n)            do { reg_F   = UINT8_T ((n)); } while (0)
#define SET_B(n)            do { reg_B   = UINT8_T ((n)); } while (0)
#define SET_C(n)            do { reg_C   = UINT8_T ((n)); } while (0)
#define SET_D(n)            do { reg_D   = UINT8_T ((n)); } while (0)
#define SET_E(n)            do { reg_E   = UINT8_T ((n)); } while (0)
#define SET_H(n)            do { reg_H   = UINT8_T ((n)); } while (0)
#define SET_L(n)            do { reg_L   = UINT8_T ((n)); } while (0)
#define SET_IXH(n)          do { reg_IXH = UINT8_T ((n)); } while (0)
#define SET_IXL(n)          do { reg_IXL = UINT8_T ((n)); } while (0)
#define SET_IYH(n)          do { reg_IYH = UINT8_T ((n)); } while (0)
#define SET_IYL(n)          do { reg_IYL = UINT8_T ((n)); } while (0)
#define SET_R(ridx, n)      do { z80_regs[ridx] = UINT8_T ((n)); } while (0)

#define SET_BC(nn)          do { reg_B   = UINT8_T (((nn) >> 8)), reg_C   = UINT8_T (((nn) & 0xFF)); } while (0)
#define SET_DE(nn)          do { reg_D   = UINT8_T (((nn) >> 8)), reg_E   = UINT8_T (((nn) & 0xFF)); } while (0)
#define SET_HL(nn)          do { reg_H   = UINT8_T (((nn) >> 8)), reg_L   = UINT8_T (((nn) & 0xFF)); } while (0)
#define SET_AF(nn)          do { reg_A   = UINT8_T (((nn) >> 8)), reg_F   = UINT8_T (((nn) & 0xFF)); } while (0)
#define SET_IX(nn)          do { reg_IXH = UINT8_T (((nn) >> 8)), reg_IXL = UINT8_T (((nn) & 0xFF)); } while (0)
#define SET_IY(nn)          do { reg_IYH = UINT8_T (((nn) >> 8)), reg_IYL = UINT8_T (((nn) & 0xFF)); } while (0)

#define SET_RR(rridx, nn)   do { z80_regs[rridx << 1] = UINT8_T (((nn) >> 8)), z80_regs[(rridx << 1) + 1] = UINT8_T (((nn) & 0xFF)); } while (0)

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * state variables used by Z80 emulator
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
static uint16_t             cur_PC;                                         // current PC
static uint16_t             reg_PC;                                         // PC register
static uint8_t              iff1;                                           // interrupt flag #1
static uint8_t              iff2;                                           // interrupt flag #2 (only for NMI)
static uint8_t              ixflags;                                        // IX relevant opcode follows
static uint8_t              iyflags;                                        // IY relevant opcode follows
static uint8_t              last_ixiyflags;                                 // last state of ixflags / iyflags
static uint32_t             clockcycles;                                    // clock cycles

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 interrupts triggered by ZX spectrum ULA
 *-------------------------------------------------------------------------------------------------------------------------------------------
*/
static uint8_t              z80_interrupt           = 0;                    // flag: 50Hz interrupt occured

#ifdef QT_CORE_LIB
static volatile uint8_t     z80_do_pause            = 0;                    // flag: pause emulator
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ZX spectrum tape variables
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char                 fname_rom_buf[MAX_FILENAME_LEN * 2 + 1];        // name of ROM file

static char                 snapshot_save_fname[256];                       // snapshot file name (save)
static volatile int         snapshot_save_valid = 0;                        // flag: snapshot (save) file name is valid

static char                 fname_load_buf[256];                            // tape file name (load)
static volatile int         fname_load_valid;                               // flag: tape (load) file name is valid
static volatile int         fname_load_snapshot_valid;                      // flag: snapshot (load) file name is valid
static uint8_t              tape_load_format;                               // format of tape file

static char                 fname_save_buf[256];                            // tape file name (save)
static volatile int         fname_save_valid;                               // flag: (save) tape file name is valid

#if defined STM32F4XX || defined FRAMEBUFFER || defined X11
static volatile uint8_t     z80_focus               = 1;                    // flag: focus is on emulator running
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * Z80 Parity table
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static const uint8_t        parity_table[256] =                             // parity table
{
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * macros to handle clock cycles (T-states)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define ADD_CLOCKCYCLES(x)    do { clockcycles += (x); } while (0)          // add clock cycles

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get_nn() - get unsigned word from RAM indicated by PC
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE uint16_t
get_nn (void)
{
    uint16_t    nn;

    reg_PC++;
    nn = zx_ram_get_text (reg_PC);
    reg_PC++;
    nn += zx_ram_get_text (reg_PC) << 8;
    return nn;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get_un() - get unsigned byte from RAM indicated by PC
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE uint_fast8_t
get_un (void)
{
    uint8_t n;

    reg_PC++;
    n = zx_ram_get_text (reg_PC);
    return n;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get_sn() - get signed byte from RAM indicated by PC
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE int8_t
get_sn (void)
{
    int8_t n;

    reg_PC++;
    n = INT8_T (zx_ram_get_text (reg_PC));
    return n;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_v_add () - set overflow flag indicated by addition (8 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_v_add (uint16_t result16, uint8_t sum1, uint8_t sum2)
{
    uint8_t     overflow;

    overflow = (~(sum1 ^ sum2)) & 0x80;                                     // check, if signs of sum1, sum2 are identical

    if (overflow)                                                           // if identical:
    {
        overflow = (result16 ^ sum1) & 0x80;                                // if sign of result differs: overflow
    }

    if (overflow)
    {
        SET_FLAG_PV();
    }
    else
    {
        RES_FLAG_PV();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_v_sub () - set overflow flag indicated by subtraction (8 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_v_sub (uint16_t result16, uint8_t sum1, uint8_t sum2)
{
    set_flag_v_add (result16, sum1, ~sum2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_v_add32 () - set overflow flag indicated by addition (16 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_v_add32 (uint32_t result32, uint16_t sum1, uint16_t sum2)
{
    uint16_t    overflow;

    overflow = (~(sum1 ^ sum2)) & 0x8000;                                   // check, if signs of sum1, sum2 are identical

    if (overflow)                                                           // if identical:
    {
        overflow = ((result32 ^ sum1) & 0x8000);                            // if sign of result differs: overflow
    }

    if (overflow)
    {
        SET_FLAG_PV();
    }
    else
    {
        RES_FLAG_PV();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_v_sub32 () - set overflow flag indicated by subtraction (16 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_v_sub32 (uint32_t result32, uint16_t sum1, uint16_t sum2)
{
    set_flag_v_add32 (result32, sum1, ~sum2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_h_add () - set half carry flag indicated by addition (8 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_h_add (uint8_t sum1, uint8_t sum2, uint8_t carry)
{
    uint8_t result_hc;
    result_hc = (sum1 & 0x0F) + (sum2 & 0x0F) + carry;

    if (result_hc & 0x10)                                                   // half carry set?
    {
        SET_FLAG_H();
    }
    else
    {
        RES_FLAG_H();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_h_sub () - set half carry flag indicated by subtraction (8 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_h_sub (uint8_t sub1, uint8_t sub2, uint8_t carry)
{
    uint8_t result_hc;
    result_hc = (sub1 & 0x0F) - (sub2 & 0x0F) - carry;

    if (result_hc & 0x10)                                                   // half carry set?
    {
        SET_FLAG_H();
    }
    else
    {
        RES_FLAG_H();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_h_add32 () - set half carry flag indicated by addition (16 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_h_add32 (uint16_t sum1, uint16_t sum2, uint8_t carry)
{
    uint16_t result_hc;
    result_hc = (sum1 & 0x0FFF) + (sum2 & 0x0FFF) + carry;

    if (result_hc & 0x1000)                                                 // half carry (bit11) set?
    {
        SET_FLAG_H();
    }
    else
    {
        RES_FLAG_H();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_h_sub32 () - set half carry flag indicated by subtraction (16 bit)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_h_sub32 (uint16_t sub1, uint16_t sub2, uint8_t carry)
{
    uint16_t result_hc;
    result_hc = (sub1 & 0x0FFF) - (sub2 & 0x0FFF) - carry;

    if (result_hc & 0x1000)                                                 // half carry set?
    {
        SET_FLAG_H();
    }
    else
    {
        RES_FLAG_H();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag_p () - set parity flag indicated by value
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag_p (uint8_t value)
{
    if (parity_table[value])
    {
        SET_FLAG_PV();
    }
    else
    {
        RES_FLAG_PV();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flags_c_z_s () - set carry, zero, siggned flag indicated by 16 bit result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flags_c_z_s (uint16_t result16)
{
    if (result16 & 0x0100)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    if (result16 & 0x00FF)
    {
        RES_FLAG_Z();
    }
    else
    {
        SET_FLAG_Z();
    }

    if (result16 & 0x0080)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flags_z_s () - set zero, siggned flag indicated by 16 bit result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flags_z_s (uint16_t result16)
{
    if (result16 & 0x00FF)
    {
        RES_FLAG_Z();
    }
    else
    {
        SET_FLAG_Z();
    }

    if (result16 & 0x0080)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flag32_c () - set carry flag indicated by 32 bit result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flag32_c (uint32_t result32)
{
    if (result32 & 0x00010000)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_flags32_c_z_s () - set carry, zero, siggned flag indicated by 32 bit result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
set_flags32_c_z_s (uint32_t result32)
{
    if (result32 & 0x00010000)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    if (result32 & 0x0000FFFF)
    {
        RES_FLAG_Z();
    }
    else
    {
        SET_FLAG_Z();
    }

    if (result32 & 0x00008000)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * push16 () - push 16 bit value on stack
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
push16 (uint16_t value)
{
    reg_SP--;
    zx_ram_set_8 (reg_SP, UINT8_T (value >> 8));
    reg_SP--;
    zx_ram_set_8 (reg_SP, UINT8_T (value & 0xFF));
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * pop16 () - pop 16 bit value from stack
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE uint16_t
pop16 (void)
{
    uint16_t value;

    value = zx_ram_get_8 (reg_SP);
    reg_SP++;
    value |= zx_ram_get_8 (reg_SP) << 8;
    reg_SP++;
    return value;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC A,n
 *
 * Clock cycles: 7
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_adc_a_n (void)
{
    uint16_t    result16;
    uint8_t     n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = UINT16_T (reg_A + n + (ISSET_FLAG_C()));
    set_flag_h_add (reg_A, n, ISSET_FLAG_C());
    set_flag_v_add (result16, reg_A, n);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    debug_printf ("ADC  A,%02Xh", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC A,r
 *
 * Clock cycles: 4 ADC A,r
 *               8 ADC A,IXH
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_adc_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("ADC  A,%s", z80_r_names[sridx]);
    result16 = reg_A + reg_R(sridx) + (ISSET_FLAG_C());
    set_flag_h_add (reg_A, reg_R(sridx), ISSET_FLAG_C());
    set_flag_v_add (result16, reg_A, reg_R(sridx));
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC A,(HL) / ADC A,(IX + d) / ADC A,(IY + d)
 *
 * Clock cycles:  7 ADC A,(HL)
 *               19 ADC A,(IX + d)
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */

static INLINE void
cmd_adc_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("ADC  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("ADC  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("ADC  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A + value + (ISSET_FLAG_C());
    set_flag_h_add (reg_A, value, ISSET_FLAG_C());
    set_flag_v_add (result16, reg_A, value);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC HL,BC / ADC HL,DE / ADC HL,HL
 *
 * Clock cycles: 15
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       is set by carry out of bit 11
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_adc_hl_rr (uint8_t rridx)
{
    uint32_t    result32;

    ADD_CLOCKCYCLES(15);
    result32 = UINT32_T (reg_HL + reg_RR(rridx) + (ISSET_FLAG_C()));
    set_flag_h_add32 (reg_HL, reg_RR(rridx), (ISSET_FLAG_C()));
    set_flag_v_add32 (result32, reg_HL, reg_RR(rridx));
    debug_printf ("ADC  HL,%s", z80_rr_names[rridx]);
    SET_HL(result32);
    set_flags32_c_z_s (result32);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADC HL,SP
 *
 * Clock cycles: 15
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       is set by carry out of bit 11
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_adc_hl_sp ()
{
    uint32_t    result32;

    ADD_CLOCKCYCLES(15);
    result32 = UINT32_T (reg_HL + reg_SP + (ISSET_FLAG_C()));
    set_flag_h_add32 (reg_HL, reg_SP, (ISSET_FLAG_C()));
    set_flag_v_add32 (result32, reg_HL, reg_SP);
    debug_printf ("ADC  HL,SP");
    SET_HL(result32);
    set_flags32_c_z_s(result32);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD A,r
 *
 * Clock cycles:  4 ADD A,r
 *                8 ADD A,IXH
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("ADD  A,%s", z80_r_names[sridx]);
    result16 = reg_A + reg_R(sridx);
    set_flag_h_add (reg_A, reg_R(sridx), 0);
    set_flag_v_add (result16, reg_A, reg_R(sridx));
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD A,(HL) / ADD A,(IX + d) / ADD A,(IY + d)
 *
 * Clock cycles:  7 ADD A,(HL)
 *               19 ADD A,(IX+d)
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("ADD  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("ADD  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("ADD  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A + value;
    set_flag_h_add (reg_A, value, 0);
    set_flag_v_add (result16, reg_A, value);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD A,n
 *
 * Clock cycles:  7
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_a_n (void)
{
    uint16_t    result16;
    uint8_t n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = reg_A + n;
    set_flag_h_add (reg_A, n, 0);
    set_flag_v_add (result16, reg_A, n);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    RES_FLAG_N();
    debug_printf ("ADD  %02Xh", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD HL,RR / ADD IX,RR / ADD IY,RR
 *
 * Clock cycles: 11 ADD HL,RR
 *               15 ADD IX,RR
 *
 * FLAGS:
 *  C       bit leaving on the left
 *  N       0
 *  P/V     unaffected
 *  H       is set by carry out of bit 11
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_ii_rr (uint8_t rridx)
{
    uint32_t    result32;

    result32 = UINT32_T (reg_RR(rridx));

    if (ixflags)
    {
        ADD_CLOCKCYCLES(15);
        result32 += GET_IX();
        set_flag_h_add32 (GET_IX(), reg_RR(rridx), 0);
        SET_IX(result32);
        debug_printf ("ADD  IX,%s", z80_rr_names[rridx]);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(15);
        result32 += GET_IY();
        set_flag_h_add32 (GET_IY(), reg_RR(rridx), 0);
        SET_IY(result32);
        debug_printf ("ADD  IY,%s", z80_rr_names[rridx]);
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        result32 += GET_HL();
        set_flag_h_add32 (GET_HL(), reg_RR(rridx), 0);
        SET_HL(result32);
        debug_printf ("ADD  HL,%s", z80_rr_names[rridx]);
    }

    RES_FLAG_N();
    set_flag32_c (result32);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD HL,SP / ADD IX,SP / ADD IY,SP
 *
 * Clock cycles: 11 ADD HL,RR
 *               15 ADD IX,RR
 *
 * FLAGS:
 *  C       bit leaving on the left
 *  N       0
 *  P/V     unaffected
 *  H       is set by carry out of bit 11
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_ii_sp (void)
{
    uint32_t    result32;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(15);
        debug_printf ("ADD  IX,SP");
        result32 = GET_IX() + reg_SP;
        set_flag_h_add32 (GET_IX(), reg_SP, 0);
        SET_IX(result32);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(15);
        debug_printf ("ADD  IY,SP");
        result32 = GET_IY() + reg_SP;
        set_flag_h_add32 (GET_IY(), reg_SP, 0);
        SET_IY(result32);
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        debug_printf ("ADD  HL,SP");
        result32 = GET_HL() + reg_SP;
        set_flag_h_add32 (GET_HL(), reg_SP, 0);
        SET_HL(result32);
    }

    RES_FLAG_N();
    set_flag32_c(result32);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * ADD HL,HL / ADD IX,IX / ADD IY,IY
 *
 * Clock cycles: 11 ADD HL,RR
 *               15 ADD IX,RR
 *
 * FLAGS:
 *  C       bit leaving on the left
 *  N       0
 *  P/V     unaffected
 *  H       undefined
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_add_ii_ii (void)
{
    uint32_t    result32;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(15);
        result32 = GET_IX() + GET_IX();
        set_flag_h_add32 (GET_IX(), GET_IX(), 0);
        SET_IX(result32);
        debug_printf ("ADD  IX,IX");
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(15);
        result32 = GET_IY() + GET_IY();
        set_flag_h_add32 (GET_IY(), GET_IY(), 0);
        SET_IY(result32);
        debug_printf ("ADD  IY,IY");
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        result32 = GET_HL() + GET_HL();
        set_flag_h_add32 (GET_HL(), GET_HL(), 0);
        SET_HL(result32);
        debug_printf ("ADD  HL,HL");
    }

    RES_FLAG_N();
    set_flag32_c(result32);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * AND A,r
 *
 * Clock cycles: 4 AND A,r
 *               8 AND A,IXH
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       1
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_and_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("AND  A,%s", z80_r_names[sridx]);
    result16 = reg_A & reg_R(sridx);
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    SET_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * AND A,(HL) / AND A,(IX + d) / AND A,(IY + d)
 *
 * Clock cycles:  7 AND A,(HL)
 *               19 AND A,(IX+d)
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       1
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_and_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("AND  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("AND  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("AND  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A & value;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    SET_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * AND n
 *
 * Clock cycles:  7
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       1
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_and_n (void)
{
    uint16_t    result16;
    uint16_t n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = reg_A & n;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    SET_FLAG_H();
    set_flag_p(reg_A);
    debug_printf ("AND  %02Xh", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CALL cc,nn
 *
 * Clock cycles:  17/10
 *
 * FLAGS: unaffected
 *
 * Call debug_printf() before push16()!
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_call_cond (uint8_t negate_condition, uint8_t flagidx)
{
    uint16_t    result16;
    uint16_t    pc;
    uint16_t    flag;

    pc = get_nn ();

    flag = reg_F & (1 << flagidx);

    if (negate_condition)
    {
        flag = ! flag;
        flagidx += 8;
    }

    if (flag)
    {
        ADD_CLOCKCYCLES(17);
        result16 = reg_PC + 1;
        debug_printf ("CALL %s,%04Xh ; true", z80_flagnames[flagidx], pc);
        push16(result16);
        reg_PC = pc;
    }
    else
    {
        ADD_CLOCKCYCLES(10);
        reg_PC++;
        debug_printf ("CALL %s,%04Xh ; false", z80_flagnames[flagidx], pc);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CALL nn
 *
 * Clock cycles:  17
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_call (void)
{
    uint16_t    result16;
    uint16_t pc;

    ADD_CLOCKCYCLES(17);
    pc = get_nn ();
    result16 = reg_PC + 1;
    debug_printf ("CALL %04Xh", pc);
    push16(result16);
    reg_PC = pc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CCF - complement carry flag
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       complement
 *  N       0
 *  P/V     unaffected
 *  H       undefined
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ccf (void)
{
    ADD_CLOCKCYCLES(4);

    if (ISSET_FLAG_C())
    {
        RES_FLAG_C();
    }
    else
    {
        SET_FLAG_C();
    }

    RES_FLAG_N();
    debug_printf ("CCF");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CP A,r - same as SUB r, but A is not modified
 *
 * Clock cycles:  4 CP A,r
 *                8 CP A,IXH
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cp_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("CP   A,%s", z80_r_names[sridx]);
    result16 = reg_A - reg_R(sridx);
    set_flag_h_sub (reg_A, reg_R(sridx), 0);
    set_flag_v_sub (result16, reg_A, reg_R(sridx));
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CP A,(HL) / CP A,(IX + d) / CP A,(IY + d) - same as SUB r, but A is not modified
 *
 * Clock cycles:  7 CP A,(HL)
 *               19 CP A,(IX+d)
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cp_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("CP   A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("CP   A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("CP   A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A - value;
    set_flag_h_sub (reg_A, value, 0);
    set_flag_v_sub (result16, reg_A, value);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CP A,n - same as SUB n, but A is not modified
 *
 * Clock cycles:  7
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cp_a_n (void)
{
    uint16_t    result16;
    uint8_t n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = reg_A - n;
    set_flag_h_sub (reg_A, n, 0);
    set_flag_v_sub (result16, reg_A, n);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    debug_printf ("CP   %02Xh", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CPD - compare with decrement
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     reset if BC = 0 after execution, set otherwise
 *  H       affected as defined
 *  Z       set if A = [HL]
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cpd (void)
{
    uint16_t    result16;
    uint16_t    rBC = GET_BC();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("CPD");
    ramval = zx_ram_get_8 (rHL);
    result16 = reg_A - ramval;
    set_flag_h_sub (reg_A, ramval, 0);

    if (result16 & 0x0080)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }

    rHL--;
    SET_HL(rHL);
    rBC--;
    SET_BC(rBC);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    if (result16 == 0x0000)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    SET_FLAG_N();

    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CPDR - block compare with decrement
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     reset if BC = 0 after execution, set otherwise
 *  H       affected as defined
 *  Z       set if A = [HL]
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cpdr (void)
{
    uint16_t    result16;
    uint16_t    rBC = GET_BC();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    debug_printf ("CPDR");
    RES_FLAG_Z();

    do
    {
        ramval = zx_ram_get_8 (rHL);

        rHL--;
        rBC--;

        result16 = reg_A - ramval;
        set_flag_h_sub (reg_A, ramval, 0);

        if (result16 & 0x0080)
        {
            SET_FLAG_S();
        }
        else
        {
            RES_FLAG_S();
        }

        if (result16 == 0x0000)
        {
            SET_FLAG_Z();
            ADD_CLOCKCYCLES(16);
            break;
        }

        if (rBC != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (rBC != 0);

    SET_HL(rHL);
    SET_BC(rBC);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CPI - compare with increment
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     reset if BC = 0 after execution, set otherwise
 *  H       affected as defined
 *  Z       set if A = [HL]
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cpi (void)
{
    uint16_t    result16;
    uint16_t    rBC = GET_BC();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("CPI");
    ramval = zx_ram_get_8 (rHL);
    result16 = reg_A - ramval;
    set_flag_h_sub (reg_A, ramval, 0);

    if (result16 & 0x0080)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }

    rHL++;
    SET_HL(rHL);
    rBC--;
    SET_BC(rBC);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    if (result16 == 0x0000)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CPIR - block compare with increment
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     reset if BC = 0 after execution, set otherwise
 *  H       affected as defined
 *  Z       set if A = [HL]
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cpir (void)
{
    uint16_t    result16;
    uint16_t    rBC = GET_BC();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    debug_printf ("CPIR");
    RES_FLAG_Z();

    do
    {
        ramval = zx_ram_get_8 (rHL);
        rHL++;
        rBC--;

        result16 = reg_A - ramval;
        set_flag_h_sub (reg_A, ramval, 0);

        if (result16 & 0x0080)
        {
            SET_FLAG_S();
        }
        else
        {
            RES_FLAG_S();
        }

        if (result16 == 0x0000)
        {
            SET_FLAG_Z();
            ADD_CLOCKCYCLES(16);
            break;
        }

        if (rBC != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (rBC != 0);

    SET_HL(rHL);
    SET_BC(rBC);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * CPL
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     unaffected
 *  H       1
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_cpl()
{
    ADD_CLOCKCYCLES(4);
    reg_A = ~reg_A;
    SET_FLAG_N();
    SET_FLAG_H();
    debug_printf ("CPL");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DAA
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       affected as defined
 *  N       unaffected
 *  P/V     detects parity
 *  H       exceptional
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_daa()
{
    uint16_t    result16;
    uint8_t     correction;

    ADD_CLOCKCYCLES(4);

    if ((ISSET_FLAG_H()) || (reg_A & 0x0F) > 9)
    {
        correction = 0x06;
    }
    else
    {
        correction = 0x00;
    }

    if ((ISSET_FLAG_C()) || reg_A > 0x99)
    {
        correction += 0x60;
        SET_FLAG_C();
    }

    if ((ISSET_FLAG_N()) && !(ISSET_FLAG_H()))
    {
        RES_FLAG_H();
    }
    else
    {
        if ((ISSET_FLAG_N()) && (ISSET_FLAG_H()))
        {
            if ((reg_A & 0x0F) < 6)
            {
                SET_FLAG_H();
            }
            else
            {
                RES_FLAG_H();
            }
        }
        else
        {
            if ((reg_A & 0x0F) >= 0x0A)
            {
                SET_FLAG_H();
            }
            else
            {
                RES_FLAG_H();
            }
        }
    }

    result16 = reg_A;

    if (ISSET_FLAG_N())
    {
        result16 -= correction;
    }
    else
    {
        result16 += correction;
    }

    reg_A = UINT8_T (result16);
    set_flags_z_s (result16);
    set_flag_p (reg_A);
    debug_printf ("DAA");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DEC r
 *
 * Clock cycles:  4 DEC r
 *                8 DEC IXH
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_dec_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    result16 = reg_R(sridx) - 1;
    set_flag_h_sub (reg_R(sridx), 1, 0);
    set_flag_v_sub (result16, reg_R(sridx), 1);
    reg_R(sridx) = UINT8_T (result16);
    set_flags_z_s(result16);
    SET_FLAG_N();
    debug_printf ("DEC  %s", z80_r_names[sridx]);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DEC (HL)
 *
 * Clock cycles: 11 DEC (HL)
 *               23 DEC (IX+d)
 *
 * FLAGS:
 *  C       unaffected
 *  N       affected as defined
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_dec_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     ramval;
    int8_t      offset;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(23);
        offset = get_sn ();
        addr = UINT16_T (GET_IX() + offset);
        debug_printf ("DEC  (IX + %02Xh)", offset);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(23);
        offset = get_sn ();
        addr = UINT16_T (GET_IY() + offset);
        debug_printf ("DEC  (IY + %02Xh)", offset);
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        addr = GET_HL();
        debug_printf ("DEC  (HL)");
    }

    ramval = zx_ram_get_8 (addr);
    result16 = ramval - 1;
    set_flag_h_sub (ramval, 1, 0);
    set_flag_v_sub (result16, ramval, 1);
    zx_ram_set_8 (addr, UINT8_T (result16));
    set_flags_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DEC BC / DEC DE
 *
 * Clock cycles: 6
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_dec_rr (uint8_t rridx)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(6);
    result16 = reg_RR(rridx) - 1;
    SET_RR(rridx, result16);
    debug_printf ("DEC  %s", z80_rr_names[rridx]);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DEC HL / DEC IX / DEC IY
 *
 * Clock cycles:  6 DEC HL
 *               10 DEC IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_dec_ii (void)
{
    uint16_t    value;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(10);
        value = GET_IX() - 1;
        SET_IX(value);
        debug_printf ("DEC  IX");
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(10);
        value = GET_IY() - 1;
        SET_IY(value);
        debug_printf ("DEC  IY");
    }
    else
    {
        ADD_CLOCKCYCLES(6);
        value = GET_HL() - 1;
        SET_HL(value);
        debug_printf ("DEC  HL");
    }
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DEC SP
 *
 * Clock cycles: 6
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_dec_sp (void)
{
    ADD_CLOCKCYCLES(6);
    reg_SP--;
    debug_printf ("DEC  SP");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DI
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_di (void)
{
    ADD_CLOCKCYCLES(4);
    iff1 = 0;
    debug_printf ("DI");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * DJNZ n
 *
 * Clock cycles: 13/8
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_djnz (void)
{
    int8_t     offset;
    uint16_t    pc;

    offset = get_sn ();
    pc = UINT16_T (reg_PC + offset + 1);

    reg_B--;

    if (reg_B)
    {
        ADD_CLOCKCYCLES(13);
        reg_PC = pc;
    }
    else
    {
        ADD_CLOCKCYCLES(8);
        reg_PC++;
    }
    debug_printf ("DJNZ %02Xh  ; %04Xh", UINT8_T (offset & 0x00FF) , pc);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EI
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ei (void)
{
    ADD_CLOCKCYCLES(4);
    iff1 = 1;
    debug_printf ("EI");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EX AF,AF'
 *
 * Clock cycles: 4
 *
 * FLAGS: affected by F'
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ex_af_af (void)
{
    uint8_t reg_A_temp;
    uint8_t reg_F_temp;

    ADD_CLOCKCYCLES(4);

    reg_A_temp = reg_A;
    reg_F_temp = reg_F;

    reg_A   = reg_A2;
    reg_F   = reg_F2;
    reg_A2  = reg_A_temp;
    reg_F2  = reg_F_temp;
    debug_printf ("EX   AF,AF'");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EX DE,HL
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ex_de_hl (void)
{
    uint8_t     l;
    uint8_t     h;

    ADD_CLOCKCYCLES(4);

    l = reg_L;
    h = reg_H;
    reg_L = reg_E;
    reg_H = reg_D;
    reg_E = l;
    reg_D = h;
    debug_printf ("EX   DE,HL");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EX (SP),HL / EX (SP),IX / EX (SP),IY
 *
 * Clock cycles: 19 EX (SP),HL
 *               23 EX (SP),IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ex_ind_sp_ii (void)
{
    uint8_t     l;
    uint8_t     h;
    uint8_t     rl;
    uint8_t     rh;
    uint16_t    addr;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(23);
        l = GET_IXL();
        h = GET_IXH();
        debug_printf ("EX   (SP),IX");
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(23);
        l = GET_IYL();
        h = GET_IYH();
        debug_printf ("EX   (SP),IY");
    }
    else
    {
        ADD_CLOCKCYCLES(19);
        l = GET_L();
        h = GET_H();
        debug_printf ("EX   (SP),HL");
    }

    addr = reg_SP;
    rl = zx_ram_get_8 (addr);
    rh = zx_ram_get_8 (addr + 1);

    zx_ram_set_8 (addr, l);
    zx_ram_set_8 (addr + 1, h);

    if (ixflags)
    {
        SET_IXL(rl);
        SET_IXH(rh);
    }
    else if (iyflags)
    {
        SET_IYL(rl);
        SET_IYH(rh);
    }
    else
    {
        SET_L(rl);
        SET_H(rh);
    }

    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * EXX
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_exx (void)
{
    uint8_t temp;

    ADD_CLOCKCYCLES(4);
    temp = reg_B; reg_B = reg_B2; reg_B2 = temp;
    temp = reg_C; reg_C = reg_C2; reg_C2 = temp;
    temp = reg_D; reg_D = reg_D2; reg_D2 = temp;
    temp = reg_E; reg_E = reg_E2; reg_E2 = temp;
    temp = reg_H; reg_H = reg_H2; reg_H2 = temp;
    temp = reg_L; reg_L = reg_L2; reg_L2 = temp;

    debug_printf ("EXX");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * HALT
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */

static INLINE void
cmd_halt (void)
{
    ADD_CLOCKCYCLES(4);
    debug_printf ("HALT");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IM 0
 *
 * Clock cycles: 8
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_im0 (void)
{
    ADD_CLOCKCYCLES(8);
    debug_printf ("IM   0");
    interrupt_mode = 0;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IM 1
 *
 * Clock cycles: 8
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_im1 (void)
{
    ADD_CLOCKCYCLES(8);
    debug_printf ("IM   1");
    interrupt_mode = 1;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IM 0/1
 *
 * Clock cycles: 8
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_im01 (void)
{
    ADD_CLOCKCYCLES(8);
    debug_printf ("IM   01");
    interrupt_mode = 0;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IM 2
 *
 * Clock cycles: 8
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_im2 (void)
{
    ADD_CLOCKCYCLES(8);
    debug_printf ("IM   2");
    interrupt_mode = 2;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IN r,(C)
 *
 * Clock cycles: 12
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects parity
 *  H       affected as defined     TODO
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_in_r_ind_c (uint8_t ridx)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(12);
    result16 = zxio_in_port (reg_B, reg_C);
    SET_R(ridx, result16);
    set_flags_z_s(result16);
    RES_FLAG_N();
    set_flag_p(GET_R(ridx));
    reg_PC++;
    debug_printf ("IN   %s,(C)", z80_r_names[ridx]);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IN (C) - read port, affect flags only (undocumented)
 *
 * Clock cycles: 12
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects parity
 *  H       affected as defined     TODO
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_in_ind_c (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(12);
    result16 = zxio_in_port (reg_B, reg_C);
    set_flags_z_s(result16);
    RES_FLAG_N();
    set_flag_p(UINT8_T (result16));
    reg_PC++;
    debug_printf ("IN   (C)");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IN A,(n)
 *
 * Clock cycles: 11
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_in_a_ind_n (void)
{
    uint8_t n;

    ADD_CLOCKCYCLES(11);
    n = get_un ();
    reg_A = zxio_in_port (reg_A, n);
    reg_PC++;
    debug_printf ("IN   A,(%02Xh)", n);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INC r
 *
 * Clock cycles:  4 INC r
 *                8 INC IXH
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inc_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    result16 = reg_R(sridx) + 1;
    set_flag_h_add (reg_R(sridx), 1, 0);
    set_flag_v_add (result16, reg_R(sridx), 1);
    reg_R(sridx) = UINT8_T (result16);
    RES_FLAG_N();
    set_flags_z_s(result16);
    debug_printf ("INC  %s", z80_r_names[sridx]);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INC (HL)
 *
 * Clock cycles: 11 INC (HL)
 *               23 INC (IX+d)
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inc_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     ramval;
    int8_t      offset;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(23);
        offset = get_sn ();
        addr = UINT16_T (GET_IX() + offset);
        debug_printf ("INC  (IX + %02Xh)", offset);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(23);
        offset = get_sn ();
        addr = UINT16_T (GET_IY() + offset);
        debug_printf ("INC  (IY + %02Xh)", offset);
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        addr = GET_HL();
        debug_printf ("INC  (HL)");
    }

    ramval = zx_ram_get_8 (addr);
    result16 = ramval + 1;
    set_flag_h_add (ramval, 1, 0);
    set_flag_v_add (result16, ramval, 1);
    zx_ram_set_8 (addr, UINT8_T (result16));
    RES_FLAG_N();
    set_flags_z_s(result16);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INC BC / INC DE
 *
 * Clock cycles: 6
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inc_rr (uint8_t rridx)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(6);
    result16 = reg_RR(rridx) + 1;
    SET_RR(rridx, result16);
    debug_printf ("INC  %s", z80_rr_names[rridx]);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INC HL / INC IX / INC IY
 *
 * Clock cycles:  6 INC HL
 *               10 INC IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inc_ii (void)
{
    uint16_t value;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(10);
        value = GET_IX() + 1;
        SET_IX(value);
        debug_printf ("INC  IX");
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(10);
        value = GET_IY() + 1;
        SET_IY(value);
        debug_printf ("INC  IY");
    }
    else
    {
        ADD_CLOCKCYCLES(6);
        value = GET_HL() + 1;
        SET_HL(value);
        debug_printf ("INC  HL");
    }
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INC SP
 *
 * Clock cycles: 6
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inc_sp (void)
{
    ADD_CLOCKCYCLES(6);
    reg_SP++;
    debug_printf ("INC  SP");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IND - input with decrement
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       set if B = 0 after execution
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ind (void)
{
    uint16_t    addr;

    ADD_CLOCKCYCLES(16);
    addr = GET_HL();
    zx_ram_set_8(addr, zxio_in_port (reg_B, reg_C));
    reg_B--;
    addr--;
    SET_HL(addr);

    if (reg_B == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }
    SET_FLAG_N();

    debug_printf ("IND");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INDR - block input with decrement
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       1
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_indr (void)
{
    uint16_t    addr;

    addr = GET_HL();

    do
    {
        zx_ram_set_8(addr, zxio_in_port (reg_B, reg_C));
        reg_B--;
        addr--;

        if (reg_B != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (reg_B != 0);

    SET_HL(addr);

    SET_FLAG_N();
    SET_FLAG_Z();
    debug_printf ("INDR");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INI - input with increment
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       set if B = 0 after execution
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ini (void)
{
    uint16_t    addr;

    ADD_CLOCKCYCLES(16);
    addr = GET_HL();
    zx_ram_set_8(addr, zxio_in_port (reg_B, reg_C));
    reg_B--;
    addr++;
    SET_HL(addr);

    if (reg_B == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }
    SET_FLAG_N();

    debug_printf ("INI");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * INIR - block input with increment
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       1
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_inir (void)
{
    uint16_t    addr;

    addr = GET_HL();

    do
    {
        zx_ram_set_8(addr, zxio_in_port (reg_B, reg_C));
        reg_B--;
        addr++;

        if (reg_B != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (reg_B != 0);

    SET_HL(addr);
    SET_FLAG_N();
    SET_FLAG_Z();
    debug_printf ("INIR");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IXFLAGS
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ixflags (void)
{
    ixflags = 1;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * IYFLAGS
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_iyflags (void)
{
    iyflags = 1;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * JP cc,nn
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_jp_cond (uint8_t negate_condition, uint8_t flagidx)
{
    uint16_t    pc;
    uint16_t    flag;

    ADD_CLOCKCYCLES(10);
    pc = get_nn ();

    flag = reg_F & (1 << flagidx);

    if (negate_condition)
    {
        flag = ! flag;
        flagidx += 8;
    }

    if (flag)
    {
        reg_PC = pc;
        debug_printf ("JP   %s,%04Xh ; true", z80_flagnames[flagidx], pc);
    }
    else
    {
        reg_PC++;
        debug_printf ("JP   %s,%04Xh ; false", z80_flagnames[flagidx], pc);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * JP nn
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_jp_nn (void)
{
    uint16_t    pc;

    ADD_CLOCKCYCLES(10);
    pc = get_nn ();
    reg_PC = pc;
    debug_printf ("JP   %04Xh", pc);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * JP (HL) / JP (IX) / JP (IY)
 *
 * Clock cycles: 4 JP (HL)
 *               8 JP (IX)
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_jp_ind_ii (void)
{
    uint16_t    pc;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);
        pc = GET_IX();
        debug_printf ("JP   (IX) ; %04X", pc);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);
        pc = GET_IY();
        debug_printf ("JP   (IY) ; %04X", pc);
    }
    else
    {
        ADD_CLOCKCYCLES(4);
        pc = GET_HL();
        debug_printf ("JP   (HL) ; %04X", pc);
    }

    reg_PC = pc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * JR cc,n
 *
 * Clock cycles: 12/7
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_jr_cond (uint8_t negate_condition, uint8_t flagidx)
{
    int8_t     offset;
    uint16_t    pc;
    uint16_t    flag;

    offset = get_sn ();
    pc = UINT16_T (reg_PC + offset + 1);

    flag = reg_F & (1 << flagidx);

    if (negate_condition)
    {
        flag = ! flag;
        flagidx += 8;
    }

    if (flag)
    {
        ADD_CLOCKCYCLES(12);
        reg_PC = pc;
        debug_printf ("JR   %s,%02Xh ; %04Xh true", z80_flagnames[flagidx], UINT8_T (offset & 0x00FF), pc);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        reg_PC++;
        debug_printf ("JR   %s,%02Xh ; %04Xh false", z80_flagnames[flagidx], UINT8_T (offset & 0x00FF), pc);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * JR n
 *
 * Clock cycles: 12
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_jr (void)
{
    int8_t     offset;

    ADD_CLOCKCYCLES(12);
    offset = get_sn ();
    reg_PC += offset + 1;

    debug_printf ("JR   %02Xh  ; %04Xh", UINT8_T (offset & 0x00FF), reg_PC);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD BC,(nn) / DE,(nn)
 *
 * Clock cycles: 20
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_rr_ind_nn (uint8_t rridx)
{
    uint16_t    nn;
    uint16_t    value;

    ADD_CLOCKCYCLES(20);
    nn = get_nn ();
    debug_printf ("LD   %s,(%04Xh)", z80_rr_names[rridx], nn);
    value = zx_ram_get_16 (nn);
    SET_RR(rridx, value);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD HL,(nn) / LD IX,(nn) / LD IY,(nn)
 *
 * Clock cycles: 16 LD HL,(nn)
 *               20 LD IX,(nn)
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ii_ind_nn (void)
{
    uint16_t    addr;
    uint16_t    value;

    addr = get_nn ();

    if (ixflags)
    {
        ADD_CLOCKCYCLES(20);
        debug_printf ("LD   IX,(%04Xh)", addr);
        value = zx_ram_get_16 (addr);
        SET_IX(value);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(20);
        debug_printf ("LD   IY,(%04Xh)", addr);
        value = zx_ram_get_16 (addr);
        SET_IY(value);
    }
    else
    {
        ADD_CLOCKCYCLES(16);
        debug_printf ("LD   HL,(%04Xh)", addr);
        value = zx_ram_get_16 (addr);
        SET_HL(value);
    }
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD SP,(nn)
 *
 * Clock cycles: 20
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_sp_ind_nn (void)
{
    uint16_t    nn;

    ADD_CLOCKCYCLES(20);
    nn = get_nn ();

    reg_SP = zx_ram_get_16 (nn);
    debug_printf ("LD   SP,(%04Xh)", nn);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD RR,nn
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_rr_nn (uint8_t rridx)
{
    uint16_t    nn;

    ADD_CLOCKCYCLES(10);
    nn = get_nn ();

    SET_RR(rridx, nn);
    debug_printf ("LD   %s,%04Xh", z80_rr_names[rridx], nn);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD HL,nn / LD IX,nn / LD IY,nn
 *
 * Clock cycles: 10 LD HL,nn
 *               14 LD IX,nn
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ii_nn (void)
{
    if (ixflags)
    {
        ADD_CLOCKCYCLES(14);
        reg_IXL = get_un ();
        reg_IXH = get_un ();
        debug_printf ("LD   IX,%02X%02Xh", reg_IXH, reg_IXL);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(14);
        reg_IYL = get_un ();
        reg_IYH = get_un ();
        debug_printf ("LD   IY,%02X%02Xh", reg_IYH, reg_IYL);
    }
    else
    {
        ADD_CLOCKCYCLES(10);
        reg_L = get_un ();
        reg_H = get_un ();
        debug_printf ("LD   HL,%02X%02Xh", reg_H, reg_L);
    }
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD SP,HL / LD SP,IX / LD SP,IY
 *
 * Clock cycles: 6  LD SP,HL
 *               10 LD SP,IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_sp_ii (void)
{
    if (ixflags)
    {
        ADD_CLOCKCYCLES(10);
        debug_printf ("LD   SP,IX");
        reg_SP = GET_IX();
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(10);
        debug_printf ("LD   SP,IY");
        reg_SP = GET_IY();
    }
    else
    {
        ADD_CLOCKCYCLES(6);
        debug_printf ("LD   SP,HL");
        reg_SP = GET_HL();
    }

    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD SP,nn
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_sp_nn (void)
{
    ADD_CLOCKCYCLES(10);
    reg_SP = get_nn ();
    debug_printf ("LD  SP,%04Xh", reg_SP);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD r,n / LD IXH,n / LD IYH,n / LD IXL,n / LD IYL,n
 *
 * Clock cycles:  7 LD H,n
 *               11 LD IXH,n
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_r_n (uint8_t ridx)
{
    uint8_t n;

    if (ixflags)
    {
        if (ridx == REG_IDX_H || ridx == REG_IDX_L)
        {
            ADD_CLOCKCYCLES(11);
            ridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        if (ridx == REG_IDX_H || ridx == REG_IDX_L)
        {
            ADD_CLOCKCYCLES(11);
            ridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(7);
    }

    n = get_un ();
    reg_R(ridx) = n;
    debug_printf ("LD   %s,%02Xh", z80_r_names[ridx], n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD r,r
 *
 * Clock cycles: 4 LD H,r
 *               8 LD IXH,r
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_r_r (uint8_t tridx, uint8_t sridx)
{
    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (tridx == REG_IDX_H || tridx == REG_IDX_L)
        {
            tridx += REG_OFFSET_IX;
        }
        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (tridx == REG_IDX_H || tridx == REG_IDX_L)
        {
            tridx += REG_OFFSET_IY;
        }
        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("LD   %s,%s", z80_r_names[tridx], z80_r_names[sridx]);
    reg_R(tridx) = reg_R(sridx);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD r,(HL) / LD r,(IX + d) / LD r,(IY + d)
 *
 * Clock cycles:  7 LD r,(HL)
 *               19 LD r,(IXH + d)
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_r_ind_ii (uint8_t tridx)
{
    uint16_t    addr;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("LD   %s,(IX + %02Xh)", z80_r_names[tridx], d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("LD   %s,(IY + %02Xh)", z80_r_names[tridx], d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("LD   %s,(HL)", z80_r_names[tridx]);
    }

    reg_R(tridx) = zx_ram_get_8 (addr);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (BC),A / LD (DE),A
 *
 * Clock cycles: 7
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_rr_a (uint8_t rridx)
{
    uint16_t    addr;

    ADD_CLOCKCYCLES(7);
    addr = reg_RR(rridx);
    debug_printf ("LD   (%s),A", z80_rr_names[rridx]);
    zx_ram_set_8 (addr, reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (HL),r / LD (IX + d),r / LD (IY + d),r
 *
 * Clock cycles:  7 LD (HL),r
 *               19 LD (IX + d),r
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_ii_r (uint8_t ridx)
{
    uint16_t    addr;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("LD   (IX + %02Xh),%s", d, z80_r_names[ridx]);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("LD   (IY + %02Xh),%s", d, z80_r_names[ridx]);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("LD   (HL),%s", z80_r_names[ridx]);
    }

    zx_ram_set_8 (addr, reg_R(ridx));
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (HL),n / LD (IX + d),n / LD (IY + d),n
 *
 * Clock cycles: 10 LD (HL),r
 *               19 LD (IX + d),r
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_ii_n (void)
{
    uint16_t    addr;
    int8_t      d;
    uint8_t     n;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("LD   (IX + %02Xh),", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("LD   (IY + %02Xh),", d);
    }
    else
    {
        ADD_CLOCKCYCLES(10);
        addr = GET_HL();
        debug_printf ("LD   (HL),");
    }

    n = get_un ();
    debug_printf ("%02Xh", n);
    zx_ram_set_8 (addr, n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD A,(nn)
 *
 * Clock cycles: 13
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_a_ind_nn (void)
{
    uint16_t    addr;

    ADD_CLOCKCYCLES(13);
    addr = get_nn ();
    debug_printf ("LD   A,(%04Xh)", addr);
    reg_A = zx_ram_get_8 (addr);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (nn),A
 *
 * Clock cycles: 13
 *
 * Flags: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_nn_a (void)
{
    uint16_t addr;

    ADD_CLOCKCYCLES(13);
    addr        = get_nn ();
    debug_printf ("LD   (%04Xh),A", addr);
    zx_ram_set_8 (addr, reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (nn),BC / LD (nn),DE
 *
 * Clock cycles: 20
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_nn_rr (uint8_t rridx)
{
    uint16_t    addr;
    uint16_t    value;

    ADD_CLOCKCYCLES(20);
    addr = get_nn ();
    value = reg_RR(rridx);
    debug_printf ("LD   (%04Xh),%s", addr, z80_rr_names[rridx]);
    zx_ram_set_16 (addr, value);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (nn),HL / LD (nn),IX / LD (nn),IY
 *
 * Clock cycles: 16 LD (nn),HL
 *               20 LD (nn),IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_nn_hl ()
{
    uint16_t    addr    = get_nn ();
    uint8_t     h;
    uint8_t     l;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(20);
        l = reg_IXL;
        h = reg_IXH;
        debug_printf ("LD   (%04Xh),IX", addr);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(20);
        l = reg_IYL;
        h = reg_IYH;
        debug_printf ("LD   (%04Xh),IY", addr);
    }
    else
    {
        ADD_CLOCKCYCLES(16);
        l = reg_L;
        h = reg_H;
        debug_printf ("LD   (%04Xh),HL", addr);
    }

    zx_ram_set_8 (addr, l);
    zx_ram_set_8 (addr + 1, h);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD A,(BC) / LD A,(DE) / LD A,(HL)
 *
 * Clock cycles: 7
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_a_ind_rr (uint8_t rridx)
{
    ADD_CLOCKCYCLES(7);
    debug_printf ("LD   A,(%s)", z80_rr_names[rridx]);
    reg_A = zx_ram_get_8 (reg_RR(rridx));
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD A,I
 *
 * Clock cycles: 9
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_a_i (void)
{
    ADD_CLOCKCYCLES(9);
    debug_printf ("LD   A,I");
    reg_A = reg_I;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD I,A
 *
 * Clock cycles: 9
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_i_a (void)
{
    ADD_CLOCKCYCLES(9);
    debug_printf ("LD   I,A");
    reg_I = reg_A;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD A,R
 *
 * Clock cycles: 9
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_a_r (void)
{
    ADD_CLOCKCYCLES(9);
    debug_printf ("LD   A,R (TODO)");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD R,A
 *
 * Clock cycles: 9
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_r_a (void)
{
    ADD_CLOCKCYCLES(9);
    debug_printf ("LD   R,A (TODO)");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LD (nn),SP
 *
 * Clock cycles: 20
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ld_ind_nn_sp (void)
{
    uint16_t    addr    = get_nn ();

    ADD_CLOCKCYCLES(20);
    debug_printf ("LD   (%04Xh),SP", addr);
    zx_ram_set_16 (addr, reg_SP);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LDD - load with decrement
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     exceptional
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ldd (void)
{
    uint16_t    rBC = GET_BC();
    uint16_t    rDE = GET_DE();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("LDD");

    ramval = zx_ram_get_8 (rHL);
    zx_ram_set_8 (rDE, ramval);
    rDE--;
    rHL--;
    rBC--;

    SET_BC(rBC);
    SET_DE(rDE);
    SET_HL(rHL);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    RES_FLAG_H();
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LDDR - block load with decrement
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     0
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_lddr (void)
{
    uint16_t    rBC = GET_BC();
    uint16_t    rDE = GET_DE();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    debug_printf ("LDDR");

#ifdef DEBUG
    uint8_t     save_debug = debug;
    debug = 0;                                                              // avoid excessive RAM messages from debugger
#endif

    do
    {
        ramval = zx_ram_get_8 (rHL);
        zx_ram_set_8 (rDE, ramval);
        rDE--;
        rHL--;
        rBC--;

        if (rBC != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (rBC != 0);

    SET_BC(0);
    SET_DE(rDE);
    SET_HL(rHL);

    RES_FLAG_H();
    RES_FLAG_PV();
    RES_FLAG_N();

    reg_PC++;

#ifdef DEBUG
    debug = save_debug;
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LDI - load with increment
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     exceptional
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ldi (void)
{
    uint16_t    rBC = GET_BC();
    uint16_t    rDE = GET_DE();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("LDI");

    ramval = zx_ram_get_8 (rHL);
    zx_ram_set_8 (rDE, ramval);
    rDE++;
    rHL++;
    rBC--;

    SET_BC(rBC);
    SET_DE(rDE);
    SET_HL(rHL);

    if (rBC == 0)
    {
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_PV();
    }

    RES_FLAG_H();
    RES_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * LDIR - block load with increment
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     0
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ldir (void)
{
    uint16_t    rBC = GET_BC();
    uint16_t    rDE = GET_DE();
    uint16_t    rHL = GET_HL();
    uint8_t     ramval;

    debug_printf ("LDIR");

#ifdef DEBUG
    uint8_t     save_debug = debug;
    debug = 0;                                                              // avoid excessive RAM messages from debugger
#endif

    do
    {
        ramval = zx_ram_get_8 (rHL);
        zx_ram_set_8 (rDE, ramval);
        rDE++;
        rHL++;
        rBC--;

        if (rBC != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (rBC != 0);

    SET_BC(0);
    SET_DE(rDE);
    SET_HL(rHL);

    RES_FLAG_H();
    RES_FLAG_PV();
    RES_FLAG_N();
    reg_PC++;
#ifdef DEBUG
    debug = save_debug;
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * NEG
 *
 * Clock cycles: 8
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_neg (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(8);
    debug_printf ("NEG");

    if (reg_A == 0x80)
    {
        SET_FLAG_PV();
    }

    result16 = 0 - reg_A;
    set_flag_h_sub (0, reg_A, 0);
    set_flag_v_sub (result16, 0, reg_A);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * NOP
 *
 * Clock cycles: 4
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_nop (void)
{
    ADD_CLOCKCYCLES(4);
    debug_printf ("NOP");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OR A,r
 *
 * Clock cycles: 4 OR A,r
 *               8 OR A,IXH
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_or_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("OR   A,%s", z80_r_names[sridx]);
    result16 = reg_A | reg_R(sridx);
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OR A,(HL) / OR A,(IX + d) / OR A,(IY + d)
 *
 * Clock cycles:  7 OR A,(HL)
 *               19 OR A,(IX+d)
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_or_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("OR   A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("OR   A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("OR   A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A | value;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OR A,n
 *
 * Clock cycles:  7
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_or_a_n (void)
{
    uint16_t    result16;
    uint16_t n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = reg_A | n;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
    debug_printf ("OR   A,%02Xh", n);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OUT (n),A
 *
 * Clock cycles: 11
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_out_ind_n_a (void)
{
    uint8_t n;

    ADD_CLOCKCYCLES(11);
    n = get_un ();

    zxio_out_port (reg_B, n, reg_A);
    debug_printf ("OUT  (%02Xh),A", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OUT (C),R
 *
 * Clock cycles: 12
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_out_ind_c_r (uint8_t value)
{
    ADD_CLOCKCYCLES(12);
    debug_printf ("OUT  (C),B");
    zxio_out_port (reg_B, reg_C, value);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OUTD
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       set if B = 0 after execution
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_outd (void)
{
    uint16_t    addr;
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("OUTD");
    addr = GET_HL();
    ramval = zx_ram_get_8 (addr);
    zxio_out_port (reg_B, reg_C, ramval);
    reg_B--;

    if (reg_B == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    addr--;
    SET_HL(addr);
    SET_FLAG_N ();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OTDR
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       1
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_otdr (void)
{
    uint16_t    addr;

    addr = GET_HL();

    do
    {
        zxio_out_port (reg_B, reg_C, zx_ram_get_8 (addr));
        addr--;
        reg_B--;

        if (reg_B != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (reg_B != 0);

    SET_HL(addr);
    SET_FLAG_N();
    SET_FLAG_Z();
    debug_printf ("OTDR");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OUTI
 *
 * Clock cycles: 16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       set if B = 0 after execution
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_outi (void)
{
    uint16_t    addr;
    uint8_t     ramval;

    ADD_CLOCKCYCLES(16);
    debug_printf ("OUTI");
    addr = GET_HL();
    ramval = zx_ram_get_8 (addr);
    zxio_out_port (reg_B, reg_C, ramval);
    reg_B--;

    if (reg_B == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    addr++;
    SET_HL(addr);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * OTIR
 *
 * Clock cycles: 21/16
 *
 * FLAGS:
 *  C       unaffected
 *  N       1
 *  P/V     ?
 *  H       ?
 *  Z       1
 *  S       ?
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_otir (void)
{
    uint16_t    addr;
    uint8_t     ramval;

    addr = GET_HL();

    do
    {
        ramval = zx_ram_get_8 (addr);
        zxio_out_port (reg_B, reg_C, ramval);
        reg_B--;
        addr++;

        if (reg_B != 0)
        {
            ADD_CLOCKCYCLES(21);
        }
        else
        {
            ADD_CLOCKCYCLES(16);
        }
    } while (reg_B != 0);

    SET_HL(addr);
    SET_FLAG_N();
    SET_FLAG_Z();
    debug_printf ("OTIR");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * POP AF
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 * Here we cannot use cmd_pop_rr(), because reg_A is upper half of word, but has index 7 for opcode decoding
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_pop_af (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(10);
    debug_printf ("POP  AF");
    result16 = pop16();
    reg_A = (result16 >> 8) & 0xFF;
    reg_F = result16 & 0xFF;
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * POP BC / POP DE
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_pop_rr (uint8_t rridx)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(10);
    debug_printf ("POP  %s", z80_rr_names[rridx]);
    result16 = pop16();
    SET_RR(rridx, result16);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * POP HL / POP IX / POP IY
 *
 * Clock cycles: 10 POP H
 *               14 POP IX
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_pop_ii (void)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(14);
        debug_printf ("POP  IX");
        result16 = pop16();
        SET_IX(result16);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(14);
        debug_printf ("POP  IY");
        result16 = pop16();
        SET_IY(result16);
    }
    else
    {
        ADD_CLOCKCYCLES(10);
        debug_printf ("POP  HL");
        result16 = pop16();
        SET_HL(result16);
    }
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * PUSH AF
 *
 * Clock cycles: 11
 *
 * FLAGS: unaffected
 *
 * Here we cannot use cmd_push_rr(), because reg_A is upper half of word, but has index 7 for opcode decoding
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_push_af (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(11);
    result16 = UINT16_T ((reg_A << 8) | reg_F);
    debug_printf ("PUSH AF");
    push16(result16);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * PUSH BC / DE
 *
 * Clock cycles: 11
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_push_rr (uint8_t rridx)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(11);
    result16 = reg_RR(rridx);
    debug_printf ("PUSH %s", z80_rr_names[rridx]);
    push16(result16);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * PUSH HL / PUSH IX / PUSH IY
 *
 * Clock cycles: 11 PUSH H
 *               15 PUSH IX
 *
 * FLAGS: unaffected
 *
 * Call debug_printf() before push16()!
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_push_ii (void)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(15);
        result16 = GET_IX();
        debug_printf ("PUSH IX");
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(15);
        result16 = GET_IY();
        debug_printf ("PUSH IY");
    }
    else
    {
        ADD_CLOCKCYCLES(11);
        result16 = GET_HL();
        debug_printf ("PUSH HL");
    }

    push16(result16);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RET
 *
 * Clock cycles: 10
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ret (void)
{
    ADD_CLOCKCYCLES(10);
    debug_printf ("RET");
    reg_PC = pop16();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RET cc,nn
 *
 * Clock cycles: 11/5
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_ret_cond (uint8_t negate_condition, uint8_t flagidx)
{
    uint16_t    flag;

    flag = reg_F & (1 << flagidx);

    if (negate_condition)
    {
        flag = ! flag;
        flagidx += 8;
    }

    if (flag)
    {
        ADD_CLOCKCYCLES(11);
        debug_printf ("RET  %s     ; true", z80_flagnames[flagidx]);
        reg_PC = pop16();
    }
    else
    {
        ADD_CLOCKCYCLES(5);
        debug_printf ("RET  %s     ; false", z80_flagnames[flagidx]);
        reg_PC++;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RETI
 *
 * Clock cycles: 14
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_reti (void)
{
    ADD_CLOCKCYCLES(14);
    debug_printf ("RETI");
    reg_PC = pop16();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RETN
 *
 * Clock cycles: 14
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_retn (void)
{
    ADD_CLOCKCYCLES(14);
    debug_printf ("RETN");
    reg_PC = pop16();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RLCA -   8-bit rotation to the left. The bit leaving on the left is copied into the carry, and to bit 0.
 *          Performs "RLC A" much quicker, and modifies the flags differently.
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       bit leaving on the left
 *  N       0
 *  P/V     unaffected
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rlca (void)
{
    ADD_CLOCKCYCLES(4);

    if (reg_A & 0x80)
    {
        reg_A = UINT8_T ((reg_A << 1) | 0x01);
        SET_FLAG_C();
    }
    else
    {
        reg_A = UINT8_T ((reg_A << 1));
        RES_FLAG_C();
    }

    RES_FLAG_N();
    RES_FLAG_H();
    debug_printf ("RLCA");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RRCA -   8-bit rotation to the right. the bit leaving on the right is copied into the carry, and into bit 7.
 *          Performs a "RRC A" faster and modifies the flags differently.
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       bit leaving on the right
 *  N       0
 *  P/V     unaffected
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rrca (void)
{
    ADD_CLOCKCYCLES(4);

    if (reg_A & 0x01)
    {
        reg_A = (reg_A >> 1) | 0x80;
        SET_FLAG_C();
    }
    else
    {
        reg_A = (reg_A >> 1);
        RES_FLAG_C();
    }

    RES_FLAG_N();
    RES_FLAG_H();
    debug_printf ("RRCA");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RLA - Rotate register A left arithmetic (fast, see also RL r)
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       Carry becomes the bit leaving on the right
 *  N       0
 *  P/V     unaffected
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rla (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(4);

    result16 = UINT16_T ((reg_A << 1));

    if (ISSET_FLAG_C())
    {
        result16 |= 0x01;
    }

    reg_A = UINT8_T (result16);

    if (result16 & 0x0100)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    RES_FLAG_N();
    RES_FLAG_H();
    debug_printf ("RLA");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RLD - Rotate left decimal
 *
 * Clock cycles: 18
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rld (void)
{
    uint16_t    addr    = GET_HL();
    uint8_t     value   = zx_ram_get_8 (addr);
    uint8_t     low_A   = reg_A & 0x0F;
    uint8_t     new_value = UINT8_T ((value << 4) | low_A);

    ADD_CLOCKCYCLES(18);
    debug_printf ("RLD");
    zx_ram_set_8 (addr, new_value);
    reg_A = (reg_A & 0xF0) | (value >> 4);

    if (reg_A == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    if (reg_A & 0x80)
    {
        SET_FLAG_S();
    }
    else
    {
        RES_FLAG_S();
    }
    set_flag_p(reg_A);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RRA - Rotate register A right arithmetic (fast, see also RR r)
 *
 * Clock cycles: 4
 *
 * FLAGS:
 *  C       Carry becomes the bit leaving on the right
 *  N       0
 *  P/V     unaffected
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rra (void)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(4);

    if (ISSET_FLAG_C())
    {
        result16 = (reg_A >> 1) | 0x0080;
    }
    else
    {
        result16 = (reg_A >> 1);
    }

    if (reg_A & 0x01)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    reg_A = UINT8_T (result16);
    RES_FLAG_N();
    RES_FLAG_H();
    debug_printf ("RRA");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RRD - rotate right decimal
 *
 * Clock cycles: 18
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rrd (void)
{
    uint16_t    addr    = GET_HL();
    uint8_t     value   = zx_ram_get_8 (addr);
    uint8_t     low_A   = reg_A & 0x0F;
    uint8_t     new_value = UINT8_T ((low_A << 4) | (value >> 4));

    ADD_CLOCKCYCLES(18);
    zx_ram_set_8 (addr, new_value);
    reg_A = (reg_A & 0xF0) | (value & 0x0F);

    if (reg_A == 0)
    {
        SET_FLAG_Z();
    }
    else
    {
        RES_FLAG_Z();
    }

    set_flag_p(reg_A);
    RES_FLAG_N();
    RES_FLAG_H();

    debug_printf ("RRD");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RST
 *
 * Clock cycles: 11
 *
 * FLAGS: unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rst (uint16_t newpc)
{
    uint16_t    result16;

    ADD_CLOCKCYCLES(11);
    result16 = reg_PC + 1;
    debug_printf ("RST  %04Xh", newpc);
    push16(result16);
    reg_PC = (newpc);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SCF - Set carry flag
 *
 * Clock cycles: 4
 *
 *  C       1
 *  N       0
 *  P/V     unaffected
 *  H       0
 *  Z       unaffected
 *  S       unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_scf (void)
{
    ADD_CLOCKCYCLES(4);
    SET_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    debug_printf ("SCF");
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SUB A,r
 *
 * Clock cycles:  4 SUB A,r
 *                8 SUB A,IXH
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sub_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("SUB  A,%s", z80_r_names[sridx]);
    result16 = reg_A - reg_R(sridx);
    set_flag_h_sub (reg_A, reg_R(sridx), 0);
    set_flag_v_sub (result16, reg_A, reg_R(sridx));
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SUB A,(HL) / SUB A,(IX + d) / SUB A,(IY + d)
 *
 * Clock cycles:  7 SUB A,(HL)
 *               19 SUB A,(IX+d)
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sub_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("SUB  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("SUB  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("SUB  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A - value;
    set_flag_h_sub (reg_A, value, 0);
    set_flag_v_sub (result16, reg_A, value);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SBC A,r
 *
 * Clock cycles: 4 SBC A,r
 *               8 SBC A,IXH
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sbc_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("SBC  A,%s", z80_r_names[sridx]);
    result16 = reg_A - reg_R(sridx) - (ISSET_FLAG_C());
    set_flag_h_sub (reg_A, reg_R(sridx), ISSET_FLAG_C());
    set_flag_v_sub (result16, reg_A, reg_R(sridx));
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SBC A,(HL) / SBC A,(IX + d) / SBC A,(IY + d)
 *
 * Clock cycles:  7 ADC A,(HL)
 *               19 ADC A,(IX + d)
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sbc_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("SBC  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("SBC  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("SBC  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A - value - (ISSET_FLAG_C());
    set_flag_h_sub (reg_A, value, ISSET_FLAG_C());
    set_flag_v_sub (result16, reg_A, value);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SUB A,n
 *
 * Clock cycles: 7
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sub_a_n (void)
{
    uint16_t    result16;
    uint8_t     n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = UINT16_T (reg_A - n);
    set_flag_h_sub (reg_A, n, 0);
    set_flag_v_sub (result16, reg_A, n);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    debug_printf ("SUB  A,%02Xh", UINT16_T (n));
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SBC A,n
 *
 * Clock cycles: 7
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sbc_a_n (void)
{
    uint16_t    result16;
    uint8_t n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    result16 = UINT16_T (reg_A - n - (ISSET_FLAG_C()));
    set_flag_h_sub (reg_A, n, ISSET_FLAG_C());
    set_flag_v_sub (result16, reg_A, n);
    reg_A = UINT8_T (result16);
    set_flags_c_z_s(result16);
    SET_FLAG_N();
    debug_printf ("SBC  %02Xh", n);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SBC HL,BC / SBC HL,DE / SBC HL,HL
 *
 * Clock cycles: 15
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sbc_hl_rr (uint8_t rridx)
{
    uint32_t    result32;

    ADD_CLOCKCYCLES(15);
    result32 = UINT32_T (GET_HL() - reg_RR(rridx)) - ISSET_FLAG_C();
    set_flag_h_sub32 (GET_HL(), reg_RR(rridx), ISSET_FLAG_C());
    set_flag_v_sub32 (result32, GET_HL(), reg_RR(rridx));
    debug_printf ("SBC  HL,%s", z80_rr_names[rridx]);
    SET_HL(result32);
    set_flags32_c_z_s (result32);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SBC HL,SP
 *
 * Clock cycles: 15
 *
 * FLAGS:
 *  C       affected as defined
 *  N       1
 *  P/V     detects overflow
 *  H       affected as defined
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sbc_hl_sp (void)
{
    uint32_t    result32;

    ADD_CLOCKCYCLES(15);
    result32 = GET_HL() - reg_SP - ISSET_FLAG_C();
    set_flag_h_sub32 (GET_HL(), reg_SP, ISSET_FLAG_C());
    set_flag_v_sub32 (result32, GET_HL(), reg_SP);
    debug_printf ("SBC  HL,SP");
    SET_HL(result32);
    set_flags32_c_z_s (result32);
    SET_FLAG_N();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * XOR A,r
 *
 * Clock cycles: 4 XOR A,r
 *               8 XOR A,IXH
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_xor_a_r (uint8_t sridx)
{
    uint16_t    result16;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IX;
        }
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(8);

        if (sridx == REG_IDX_H || sridx == REG_IDX_L)
        {
            sridx += REG_OFFSET_IY;
        }
    }
    else
    {
        ADD_CLOCKCYCLES(4);
    }

    debug_printf ("XOR  A,%s", z80_r_names[sridx]);
    result16 = reg_A ^ reg_R(sridx);
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * XOR A,(HL) / XOR A,(IX + d) / XOR A,(IY + d)
 *
 * Clock cycles:  7 XOR A,(HL)
 *               19 XOR A,(IX+d)
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_xor_a_ind_ii (void)
{
    uint16_t    result16;
    uint16_t    addr;
    uint8_t     value;
    int8_t      d;

    if (ixflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IX() + d);
        debug_printf ("XOR  A,(IX + %02Xh)", d);
    }
    else if (iyflags)
    {
        ADD_CLOCKCYCLES(19);
        d = get_sn ();
        addr = UINT16_T (GET_IY() + d);
        debug_printf ("XOR  A,(IY + %02Xh)", d);
    }
    else
    {
        ADD_CLOCKCYCLES(7);
        addr = GET_HL();
        debug_printf ("XOR  A,(HL)");
    }

    value = zx_ram_get_8 (addr);
    result16 = reg_A ^ value;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * XOR A,n
 *
 * Clock cycles:  7
 *
 * FLAGS:
 *  C       0
 *  N       0
 *  P/V     detects parity
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_xor_a_n (void)
{
    uint16_t    result16;
    uint16_t    n;

    ADD_CLOCKCYCLES(7);
    n = get_un ();
    debug_printf ("XOR  A,%02Xh", n);
    result16 = reg_A ^ n;
    reg_A = UINT8_T (result16);
    set_flags_z_s(result16);
    RES_FLAG_C();
    RES_FLAG_N();
    RES_FLAG_H();
    set_flag_p(reg_A);
    reg_PC++;
}

typedef struct
{
    uint16_t    addr;
    uint8_t     value;
    uint8_t     paddingdummy;
} Z80_BIT_VALUES;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_bits_prologue () - prepare operands
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef DEBUG
#define Z80_BITS_PROLOGUE(p1,p2,p3,p4,p5,p6,p7)    z80_bits_prologue (p1,p2,p3,p4,p5,p6,p7)
#else
#define Z80_BITS_PROLOGUE(p1,p2,p3,p4,p5,p6,p7)    z80_bits_prologue (p1,p2,p3,p4,p5,p6)
#endif

static INLINE void
Z80_BITS_PROLOGUE (Z80_BIT_VALUES * z80_bit_values, uint8_t tridx, uint8_t sridx, int8_t d, uint16_t bit, uint8_t is_bit, const char * opstring)
{
    if (ixflags)
    {
        if (is_bit)                 // BIT ...
        {
            ADD_CLOCKCYCLES(20);
        }
        else                        // other
        {
            ADD_CLOCKCYCLES(23);
        }

        if (tridx != sridx)
        {
            debug_printf ("%-5s%s,(IX + %d)", opstring, z80_r_names[tridx], d);
        }
        else
        {
            if (bit != 0xFFFF)
            {
                debug_printf ("%-5s%d,(IX + %d)", opstring, bit, d);
            }
            else
            {
                debug_printf ("%-5s(IX + %d)", opstring, d);
            }
        }

        z80_bit_values->addr = UINT16_T (GET_IX() + d);
        z80_bit_values->value = zx_ram_get_8 (z80_bit_values->addr);
    }
    else if (iyflags)
    {
        if (is_bit)                 // BIT ...
        {
            ADD_CLOCKCYCLES(20);
        }
        else                        // other
        {
            ADD_CLOCKCYCLES(23);
        }

        if (tridx != sridx)
        {
            debug_printf ("%-5s%s,(IY + %d)", opstring, z80_r_names[tridx], d);
        }
        else
        {
            if (bit != 0xFFFF)
            {
                debug_printf ("%-5s%d,(IY + %d)", opstring, bit, d);
            }
            else
            {
                debug_printf ("%-5s(IY + %d)", opstring, d);
            }
        }

        z80_bit_values->addr = UINT16_T (GET_IY() + d);
        z80_bit_values->value = zx_ram_get_8 (z80_bit_values->addr);
    }
    else if (sridx == REG_IND_HL_POS)
    {
        if (is_bit)                 // BIT ...
        {
            ADD_CLOCKCYCLES(12);
        }
        else                        // other
        {
            ADD_CLOCKCYCLES(15);
        }

        if (bit != 0xFFFF)
        {
            debug_printf ("%-5s%d,%s", opstring, bit, z80_r_names[sridx]);
        }
        else
        {
            debug_printf ("%-5s%s", opstring, z80_r_names[sridx]);
        }

        z80_bit_values->addr = GET_HL();
        z80_bit_values->value = zx_ram_get_8 (z80_bit_values->addr);
    }
    else
    {
        ADD_CLOCKCYCLES(8);

        if (bit != 0xFFFF)
        {
            debug_printf ("%-5s%d,%s", opstring, bit, z80_r_names[sridx]);
        }
        else
        {
            debug_printf ("%-5s%s", opstring, z80_r_names[sridx]);
        }

        z80_bit_values->value = reg_R(sridx);
        z80_bit_values->addr  = 0x0000;                                     // not used
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_bits_epilogue () - store result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
z80_bits_epilogue (uint16_t result16, Z80_BIT_VALUES * z80_bit_values, uint8_t tridx)
{
    if (tridx == REG_IND_HL_POS)
    {
        zx_ram_set_8 (z80_bit_values->addr, UINT8_T (result16));
    }
    else
    {
        reg_R(tridx) = UINT8_T (result16);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RLC r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rlc_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t    result16;

    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "RLC");

    if (z80_bit_values.value & 0x80)
    {
        result16 = UINT8_T ((z80_bit_values.value << 1) | 0x01);
        SET_FLAG_C();
    }
    else
    {
        result16 = UINT8_T ((z80_bit_values.value << 1));
        RES_FLAG_C();
    }

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RRC r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rrc_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "RRC");

    if (z80_bit_values.value & 0x01)
    {
        result16 = (z80_bit_values.value >> 1) | 0x80;
        SET_FLAG_C();
    }
    else
    {
        result16 = (z80_bit_values.value >> 1);
        RES_FLAG_C();
    }

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RL r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rl_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "RL");
    result16 = UINT16_T ((z80_bit_values.value << 1) | (ISSET_FLAG_C()));
    z80_bits_epilogue (result16, &z80_bit_values, tridx);

    set_flags_c_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RR r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_rr_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF,  0, "RR");

    if (ISSET_FLAG_C())
    {
        result16 = (z80_bit_values.value >> 1) | 0x80;
    }
    else
    {
        result16 = (z80_bit_values.value >> 1);
    }

    if (z80_bit_values.value & 0x01)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SLA r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sla_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "SLA");

    if (z80_bit_values.value & 0x80)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    result16 = UINT16_T(z80_bit_values.value << 1);

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SRA r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sra_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "SRA");

    if (z80_bit_values.value & 0x01)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    if (z80_bit_values.value & 0x80)
    {
        result16 = 0x80 | (z80_bit_values.value >> 1);
    }
    else
    {
        result16 = z80_bit_values.value >> 1;
    }

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s (result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SLL r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_sll_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "SLL");

    if (z80_bit_values.value & 0x80)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    result16 = UINT16_T((z80_bit_values.value << 1) | 0x01);                // always set bit 0!

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SRL r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       affected as defined
 *  N       0
 *  P/V     ?
 *  H       0
 *  Z       affected as defined
 *  S       affected as defined
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_srl_r (uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, 0xFFFF, 0, "SRL");

    if (z80_bit_values.value & 0x01)
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }

    result16 = z80_bit_values.value >> 1;

    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    set_flags_z_s(result16);
    RES_FLAG_N();
    RES_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * BIT b,r
 *
 * Clock cycles:  8 / 12 / 20 - see also z80_bits_prologue()
 *
 * FLAGS:
 *  C       unaffected
 *  N       0
 *  P/V     unknown (set as Z?)
 *  H       1
 *  Z       affected as defined
 *  S       unknown
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_bit_x_r (uint8_t bit, uint8_t sridx, int8_t d)
{
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, sridx, sridx, d, bit, 1, "BIT");       // ! tridx not available here

    if (z80_bit_values.value & (1 << bit))
    {
        RES_FLAG_Z();
        RES_FLAG_PV();
    }
    else
    {
        SET_FLAG_Z();
        SET_FLAG_PV();
    }

    RES_FLAG_N();
    SET_FLAG_H();
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * RES b,r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:   unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_res_x_r (uint8_t bit, uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, bit, 0, "RES");
    result16 = z80_bit_values.value & ~(1 << bit);
    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * SET b,r
 *
 * Clock cycles:  8 / 15 / 23 - see also z80_bits_prologue()
 *
 * FLAGS:   unaffected
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
cmd_set_x_r (uint8_t bit, uint8_t tridx, uint8_t sridx, int8_t d)
{
    uint16_t        result16;
    Z80_BIT_VALUES  z80_bit_values;

    Z80_BITS_PROLOGUE (&z80_bit_values, tridx, sridx, d, bit, 0, "SET");
    result16 = UINT16_T (z80_bit_values.value | (1 << bit));
    z80_bits_epilogue (result16, &z80_bit_values, tridx);
    reg_PC++;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_bits - Z80 bit opcodes
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void z80_bits (void)
{
    uint8_t         opcode;
    uint8_t         sridx;
    uint8_t         tridx;
    int8_t          d = 0;

    if (ixflags || iyflags)
    {
        d = get_sn ();
        sridx = REG_IND_HL_POS;                                             // many undocumented opcodes here ;-)
        reg_PC++;
        opcode = zx_ram_get_text(reg_PC);
        tridx = opcode & 0x07;
    }
    else
    {
        reg_PC++;
        opcode = zx_ram_get_text(reg_PC);
        sridx = opcode & 0x07;
        tridx = sridx;
    }

    switch (opcode & 0xF8)
    {
        case 0x00:  cmd_rlc_r (tridx, sridx, d);                break;      // 0x00 - 0x07 : 0000 0rrr - RLC r
        case 0x08:  cmd_rrc_r (tridx, sridx, d);                break;      // 0x08 - 0x0F : 000001rrr - RRC r
        case 0x10:  cmd_rl_r  (tridx, sridx, d);                break;      // 0x10 - 0x17 : 0001 0rrr - RL  r
        case 0x18:  cmd_rr_r  (tridx, sridx, d);                break;      // 0x18 - 0x1F : 0001 1rrr - RR  r
        case 0x20:  cmd_sla_r (tridx, sridx, d);                break;      // 0x20 - 0x27 : 0010 0rrr - SLA r
        case 0x28:  cmd_sra_r (tridx, sridx, d);                break;      // 0x28 - 0x2F : 0010 1rrr - SRA r
        case 0x30:  cmd_sll_r (tridx, sridx, d);                break;      // 0x30 - 0x37 : 0011 0rrr - SLL r (undocumented)
        case 0x38:  cmd_srl_r (tridx, sridx, d);                break;      // 0x38 - 0x3F : 0011 1rrr - SRL r
        default:
        {
            uint8_t bit = (opcode >> 3) & 0x07;

            switch (opcode & 0xC0)
            {
                case 0x40:  cmd_bit_x_r (bit, sridx, d);        break;      // 0x40 - 0x7F - 01bb brrr - BIT X,r
                case 0x80:  cmd_res_x_r (bit, tridx, sridx, d); break;      // 0x80 - 0xBF - 10bb brrr - RES X,r
                case 0xC0:  cmd_set_x_r (bit, tridx, sridx, d); break;      // 0xC0 - 0xFF - 11bb brrr - SET X,r
            }
            break;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_extd - Z80 extended opcodes
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
z80_extd (void)
{
    uint8_t opcode;

    reg_PC++;

    opcode = zx_ram_get_text(reg_PC);

    switch (opcode)
    {
        case 0x40:  cmd_in_r_ind_c (REG_IDX_B);             break;              // IN B,(C)
        case 0x41:  cmd_out_ind_c_r (reg_B);                break;              // OUT (C),B
        case 0x42:  cmd_sbc_hl_rr (REG_IDX_BC);             break;              // SBC HL,BC
        case 0x43:  cmd_ld_ind_nn_rr (REG_IDX_BC);          break;              // LD (nn),BC

        case 0x44:                                                              // NEG
        case 0x4C:
        case 0x54:
        case 0x5C:
        case 0x64:
        case 0x6C:
        case 0x74:
        case 0x7C:  cmd_neg ();                             break;

        case 0x45:                                                              // RETN - return from NMI
        case 0x55:
        case 0x5D:
        case 0x65:
        case 0x6D:
        case 0x75:
        case 0x7D:  cmd_retn ();                            break;

        case 0x46:  cmd_im0 ();                             break;              // IM 0
        case 0x47:  cmd_ld_i_a ();                          break;              // LD I,A
        case 0x48:  cmd_in_r_ind_c (REG_IDX_C);             break;              // IN C,(C)
        case 0x49:  cmd_out_ind_c_r (reg_C);                break;              // OUT (C),C
        case 0x4A:  cmd_adc_hl_rr (REG_IDX_BC);             break;              // ADC HL,BC
        case 0x4B:  cmd_ld_rr_ind_nn (REG_IDX_BC);          break;              // LD BC,(nn)
        case 0x4D:  cmd_reti ();                            break;              // RETI - return from (maskable) interrupt
        case 0x4E:  cmd_im01 ();                            break;              // IM 0/1
        case 0x4F:  cmd_ld_r_a ();                          break;              // LD R,A - load refresh register
        case 0x50:  cmd_in_r_ind_c (REG_IDX_D);             break;              // IN D,(C)
        case 0x51:  cmd_out_ind_c_r (reg_D);                break;              // OUT (C),D
        case 0x52:  cmd_sbc_hl_rr (REG_IDX_DE);             break;              // SBC HL,DE
        case 0x53:  cmd_ld_ind_nn_rr (REG_IDX_DE);          break;              // LD (nn),DE
        case 0x56:  cmd_im1 ();                             break;              // IM 1
        case 0x57:  cmd_ld_a_i ();                          break;              // LD A,I (interrupt register)
        case 0x58:  cmd_in_r_ind_c (REG_IDX_E);             break;              // IN E,(C)
        case 0x59:  cmd_out_ind_c_r (reg_E);                break;              // OUT (C),E
        case 0x5A:  cmd_adc_hl_rr (REG_IDX_DE);             break;              // ADC HL,DE
        case 0x5B:  cmd_ld_rr_ind_nn (REG_IDX_DE);          break;              // LD DE,(nn)
        case 0x5E:  cmd_im2 ();                             break;              // IM 2
        case 0x5F:  cmd_ld_a_r ();                          break;              // LD A,R - refresh register
        case 0x60:  cmd_in_r_ind_c (REG_IDX_H);             break;              // IN H,(C)
        case 0x61:  cmd_out_ind_c_r (reg_H);                break;              // OUT (C),H
        case 0x62:  cmd_sbc_hl_rr (REG_IDX_HL);             break;              // SBC HL,HL
        case 0x63:  cmd_ld_ind_nn_rr (REG_IDX_HL);          break;              // LD (nn),HL
        case 0x66:  cmd_im0 ();                             break;              // IM 0
        case 0x67:  cmd_rrd ();                             break;              // RRD
        case 0x68:  cmd_in_r_ind_c (REG_IDX_L);             break;              // IN L,(C)
        case 0x69:  cmd_out_ind_c_r (reg_L);                break;              // OUT (C),L
        case 0x6A:  cmd_adc_hl_rr(REG_IDX_HL);              break;              // ADC HL,HL
        case 0x6B:  cmd_ld_ii_ind_nn ();                    break;              // LD HL,(nn)   (undocumented)
        case 0x6E:  cmd_im01 ();                            break;              // IM 0/1
        case 0x6F:  cmd_rld ();                             break;              // RLD
        case 0x70:  cmd_in_ind_c ();                        break;              // IN (C)
        case 0x71:  cmd_out_ind_c_r (0);                    break;              // OUT (C),0
        case 0x72:  cmd_sbc_hl_sp ();                       break;              // SBC HL,SP
        case 0x73:  cmd_ld_ind_nn_sp ();                    break;              // LD (nn),SP
        case 0x76:  cmd_im1 ();                             break;              // IM 1
        case 0x78:  cmd_in_r_ind_c (REG_IDX_A);             break;              // IN A,(C)
        case 0x79:  cmd_out_ind_c_r (reg_A);                break;              // OUT (C),A
        case 0x7A:  cmd_adc_hl_sp ();                       break;              // ADC HL,SP
        case 0x7B:  cmd_ld_sp_ind_nn ();                    break;              // LD SP,(nn)
        case 0x7E:  cmd_im2 ();                             break;              // IM 2
        case 0xA0:  cmd_ldi ();                             break;              // LDI
        case 0xA1:  cmd_cpi ();                             break;              // CPI
        case 0xA2:  cmd_ini ();                             break;              // INI
        case 0xA3:  cmd_outi ();                            break;              // OUTI
        case 0xA8:  cmd_ldd ();                             break;              // LDD
        case 0xA9:  cmd_cpd ();                             break;              // CPD
        case 0xAA:  cmd_ind ();                             break;              // IND
        case 0xAB:  cmd_outd ();                            break;              // OUTD
        case 0xB0:  cmd_ldir ();                            break;              // LDIR
        case 0xB1:  cmd_cpir ();                            break;              // CPIR - search byte
        case 0xB2:  cmd_inir ();                            break;              // INIR
        case 0xB3:  cmd_otir ();                            break;              // OTIR
        case 0xB8:  cmd_lddr ();                            break;              // LDDR
        case 0xB9:  cmd_cpdr ();                            break;              // CPDR - block compare with decrement
        case 0xBA:  cmd_indr ();                            break;              // INDR - block input with decrement
        case 0xBB:  cmd_otdr ();                            break;              // OTDR
        default:
        {
            printf ("z80_extd: invalid opcode: 0x%02X\n", opcode);
            fflush (stdout);
            break;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_reset() - reset Z80
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_reset (void)
{
    memset (z80_regs, 0, N_REGS);
    memset (z80_regs2, 0, N_REGS);

    reg_SP                  = 0x0000;
    reg_PC                  = 0x0000;
    cur_PC                  = 0x0000;
    iff1                    = 0;
    ixflags                 = 0;
    iyflags                 = 0;
    last_ixiyflags          = 0;
    interrupt_mode          = 0;
    clockcycles             = 0;

    zx_border_color         = 0;
    zx_ram_init (z80_romsize);

    debug_printf ("RESET\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_pause() - pause Z80 (toggle)
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined QT_CORE_LIB
void
z80_pause (void)
{
    if (z80_do_pause)
    {
        z80_do_pause = 0;
    }
    else
    {
        z80_do_pause = 1;
    }
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_leave_focus () - leave focus and enter menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F4XX || defined FRAMEBUFFER || defined X11
void
z80_leave_focus (void)
{
    z80_focus = FALSE;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_next_turbo_mode () - set next turbo mode
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#if defined STM32F4XX || defined FRAMEBUFFER || defined X11
void
z80_next_turbo_mode (void)
{
    z80_settings.turbo_mode++;

    if (z80_settings.turbo_mode == 2)
    {
        z80_settings.turbo_mode = 0;
    }
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_set_autostart() - set flag: autostart Basic programs
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_set_autostart (uint8_t do_set)
{
    z80_settings.autostart = do_set;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_get_autostart() - get flag: autostart Basic programs
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
z80_get_autostart (void)
{
    return z80_settings.autostart;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_get_path() - get path of files
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef STM32F4XX
char *
z80_get_path (void)
{
    return z80_settings.path;
}
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set_fname_rom_buf () - set file name of ROM file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
set_fname_rom_buf (const char * romfile)
{
#ifdef STM32F4XX
    (void) strncpy (fname_rom_buf, romfile, sizeof (fname_rom_buf) - 1);
#else
    if (z80_settings.path[0])
    {
        if (snprintf (fname_rom_buf, sizeof (fname_rom_buf), "%s/%s", z80_settings.path, romfile) < 0)
        {
            return;
        }
    }
    else
    {
        (void) strncpy (fname_rom_buf, romfile, sizeof (fname_rom_buf) - 1);
    }
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * load_rom() - load ROM data
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
load_rom (void)
{
    FILE *      fp;
    int         ch;

    fp = fopen (fname_rom_buf, "rb");

    if (fp)
    {
        uint8_t *   p   = steccy_rombankptr[0];
        z80_romsize     = 0x0000;

        while ((ch = getc (fp)) != EOF)
        {
            if (z80_romsize == 0x4000)
            {
                p = steccy_rombankptr[1];
            }

            *p++ = UINT8_T (ch);
            z80_romsize++;
        }

#ifdef STM32F4XX
        zxscr_update_status ();
#endif

        debug_printf ("ROM size: %04Xh\n", z80_romsize);
        fclose (fp);

        if (z80_romsize == 0x4000 && ! memcmp (steccy_rombankptr[0] + STECCY_HOOK_ADDRESS, "STECCY", 6))
        {
            hooks_active = 1;
        }
    }
    else
    {
        perror (fname_rom_buf);
        return;
    }
}

#define SNAPSHOT_PAGE_SIZE      0x4000

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_write_byte() - write a byte into snapshot file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
snap_write_byte (FILE * fp, uint8_t ch)
{
    fputc (ch, fp);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_write_byte() - write a byte into snapshot file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
snap_write_word (FILE * fp, uint16_t w)
{
    fputc (w & 0xFF, fp);
    fputc ((w >> 8) & 0xFF, fp);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_write_uncompressed_page () - write snapshot page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
snap_write_uncompressed_page (FILE * fp, uint8_t * steccy_ram_ptr)
{
    uint32_t    idx;

    for (idx = 0; idx < SNAPSHOT_PAGE_SIZE; idx++)
    {
        snap_write_byte (fp, *(steccy_ram_ptr + idx));
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_write_page() - write snapshot page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
snap_write_page (FILE * fp, uint8_t page_no)
{
    uint8_t * steccy_ram_ptr;

    snap_write_word (fp, 0xFFFF);                                           // 0xFFFF means: uncompressed 16K block
    snap_write_byte (fp, page_no + 3);

    if (z80_romsize == 0x4000)
    {
        switch (page_no)
        {
            case 1:                                                         // 1 + 3 = 4: write 16K page 8000 - BFFFF
            {
                steccy_ram_ptr = steccy_rambankptr[2];                      // 8000 - BFFFF is bank 2
                break;
            }
            case 2:                                                         // 2 + 3 = 5: write 16K page C000 - FFFFF
            {
                steccy_ram_ptr = steccy_rambankptr[0];                      // C000 - FFFFF is bank 0
                break;
            }
            default:                                                        // 5 + 3 = 8: write 16K page 4000 - 7FFFF
            {
                steccy_ram_ptr = steccy_rambankptr[5];                      // 4000 - 7FFFF is bank 5
                break;
            }
        }
    }
    else
    {
        steccy_ram_ptr = steccy_rambankptr[page_no];
    }

    snap_write_uncompressed_page (fp, steccy_ram_ptr);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * save_snapshot() - save snapshot
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
save_snapshot (void)
{
    FILE *      fp;

    fp = fopen (snapshot_save_fname, "wb");

    if (fp)
    {
        uint16_t    idx;
        uint8_t     val;
        uint8_t     flag = UINT8_T (((reg_R >> 7) & 0x01) | (zx_border_color << 1));

        snap_write_byte (fp, reg_A);                            // 0 A
        snap_write_byte (fp, reg_F);                            // 1 F
        snap_write_byte (fp, reg_C);                            // 2 C
        snap_write_byte (fp, reg_B);                            // 3 B
        snap_write_byte (fp, reg_L);                            // 4 L
        snap_write_byte (fp, reg_H);                            // 5 H
        snap_write_word (fp, 0x0000);                           // 6+7 PC (snapshot version 3: write 0x0000)
        snap_write_word (fp, reg_SP);                           // 8+9 stack pointer
        snap_write_byte (fp, reg_I);                            // 10 Interrupt register
        snap_write_byte (fp, reg_R & 0x7F);                     // 11 Refresh register (Bit 7 is not significant!)
        snap_write_byte (fp, flag);                             // 12 Bit 0  : Bit 7 of the R-register
                                                                //    Bit 1-3: Border colour
                                                                //    Bit 4  : 1=Basic SamRom switched in
                                                                //    Bit 5  : 1=Block of data is compressed (only version 1)
                                                                //    Bit 6-7: No meaning
        snap_write_byte (fp, reg_E);                            // 13 E
        snap_write_byte (fp, reg_D);                            // 14 D
        snap_write_byte (fp, reg_C2);                           // 15 C'
        snap_write_byte (fp, reg_B2);                           // 16 B'
        snap_write_byte (fp, reg_E2);                           // 17 E'
        snap_write_byte (fp, reg_D2);                           // 18 D'
        snap_write_byte (fp, reg_L2);                           // 19 L'
        snap_write_byte (fp, reg_H2);                           // 20 H'
        snap_write_byte (fp, reg_A2);                           // 21 A'
        snap_write_byte (fp, reg_F2);                           // 22 F'
        snap_write_byte (fp, reg_IYL);                          // 23 IYL
        snap_write_byte (fp, reg_IYH);                          // 24 IYH
        snap_write_byte (fp, reg_IXL);                          // 25 IXL
        snap_write_byte (fp, reg_IXH);                          // 26 IXH
        snap_write_byte (fp, iff1);                             // 27 IFF1
        snap_write_byte (fp, iff2);                             // 28 IFF2
        snap_write_byte (fp, interrupt_mode);                   // 29 Bit 0-1:  Interrupt mode (0, 1 or 2)
                                                                //    Bit 2  :  1=Issue 2 emulation
                                                                //    Bit 3  :  1=Double interrupt frequency
                                                                //    Bit 4-5:  1=High video synchronisation
                                                                //              3=Low video synchronisation
                                                                //              0,2=Normal
                                                                //    Bit 6-7:  0=Cursor/Protek/AGF joystick
                                                                //              1=Kempston joystick
                                                                //              2=Sinclair 2 Left joystick
                                                                //              (or user defined, for version 3 .z80 files)
                                                                //              3=Sinclair 2 Right joystick
        uint16_t additional_header_len = 54;                    //    version 3 uses an additional header of 54 bytes
        snap_write_word (fp, additional_header_len);            // 30+31 length of additional header following
        snap_write_word (fp, reg_PC);                           // 32+33 PC
        additional_header_len -= 2;                             // adjust len (-2 for PC already written)

        for (idx = 0; idx < additional_header_len; idx++ )      // write empty additional header (52 bytes)
        {
            if (idx == 0)                                       // 34: hardware mode
            {
                                                                // Value:          Meaning in v2           Meaning in v3
                                                                // -----------------------------------------------------
                                                                //  0             48k                     48k
                                                                //  1             48k + If.1              48k + If.1
                                                                //  2             SamRam                  SamRam
                                                                //  3             128k                    48k + M.G.T.
                                                                //  4             128k + If.1             128k
                                                                //  5             -                       128k + If.1
                                                                //  6             -                       128k + M.G.T.
                if (z80_romsize == 0x4000)
                {
                    val = 0;
                }
                else
                {
                    val = 4;
                }
            }
            else if (idx == 2)                                  // 35: memory paging
            {
                                                                // If in SamRam mode, bitwise state of 74ls259.
                                                                // For example, bit 6=1 after an OUT 31,13 (=2*6+1)
                                                                // If in 128 mode, contains last OUT to 0x7ffd
                                                                // If in Timex mode, contains last OUT to 0xf4

                if (z80_romsize != 0x4000)                      // we are in 128K mode
                {
                    val = zxio_7ffd_value;
                }
                else
                {
                    val = 0;
                }
            }
            else
            {
                val = 0;
            }

            snap_write_byte (fp, val);
        }

        if (z80_romsize == 0x4000)
        {
            snap_write_page (fp, 1);                            // 1 + 3 = 4: write 16K page 8000 - BFFFF
            snap_write_page (fp, 2);                            // 2 + 3 = 5: write 16K page C000 - FFFFF
            snap_write_page (fp, 5);                            // 5 + 3 = 8: write 16K page 4000 - 7FFFF
        }
        else
        {
            uint_fast8_t    page;

            for (page = 0; page < 8; page++)
            {
                snap_write_page (fp, page);                     // write 8 x 16K pages
            }
        }

        fclose (fp);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_read_byte() - load a byte from snapshot file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
snap_read_byte (FILE * fp, uint8_t * chp)
{
    int ch  = getc (fp);

    if (ch == EOF)
    {
        return 0;
    }

    *chp = UINT8_T (ch);
    return 1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * snap_read_word() - load a word from snapshot file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
snap_read_word (FILE * fp, uint16_t * wp)
{
    uint8_t     lo;
    uint8_t     hi;
    uint8_t     rtc;

    if (snap_read_byte (fp, &lo) && snap_read_byte (fp, &hi))
    {
        *wp = UINT16_T (lo | (hi << 8));
        rtc = 1;
    }
    else
    {
        rtc = 0;
    }
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * load_snapshot() - load snapshot file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
load_snapshot ()
{
    FILE *      fp;

    fp = fopen (fname_load_buf, "rb");

    if (fp)
    {
        int         ch;
        uint8_t     flag            = 0;
        uint8_t     val             = 0;
        uint8_t     dummy           = 0;
        uint8_t     data_compressed = 0;
        uint8_t *   steccy_ram_ptr;
        uint8_t     version = 1;

        zxio_reset ();

        snap_read_byte (fp, &reg_A);                            // 0 A
        snap_read_byte (fp, &reg_F);                            // 1 F
        snap_read_byte (fp, &reg_C);                            // 2 C
        snap_read_byte (fp, &reg_B);                            // 3 B
        snap_read_byte (fp, &reg_L);                            // 4 L
        snap_read_byte (fp, &reg_H);                            // 5 H
        snap_read_word (fp, &reg_PC);                           // 6+7 PC (if PC = 0x0000, see below: snap version 2/3)
        snap_read_word (fp, &reg_SP);                           // 8+9 stack pointer
        snap_read_byte (fp, &reg_I);                            // 10 Interrupt register
        snap_read_byte (fp, &dummy);                            // 11 Refresh register (Bit 7 is not significant!)
        snap_read_byte (fp, &flag);                             // 12 Bit 0  : Bit 7 of the R-register
                                                                //    Bit 1-3: Border colour
                                                                //    Bit 4  : 1=Basic SamRom switched in
                                                                //    Bit 5  : 1=Block of data is compressed
                                                                //    Bit 6-7: No meaning
        if (flag == 0xFF)                                       // if byte 12 is 255, it has to be regarded as being 1.
        {
            flag = 1;
        }

        if (flag & (1<<5))
        {
            data_compressed = 1;
            printf ("version 1: data_compressed=%d\n", data_compressed);
            fflush (stdout);
        }

        zx_border_color = (flag >> 1) & 0x07;

        snap_read_byte (fp, &reg_E);                            // 13 E
        snap_read_byte (fp, &reg_D);                            // 14 D
        snap_read_byte (fp, &reg_C2);                           // 15 C'
        snap_read_byte (fp, &reg_B2);                           // 16 B'
        snap_read_byte (fp, &reg_E2);                           // 17 E'
        snap_read_byte (fp, &reg_D2);                           // 18 D'
        snap_read_byte (fp, &reg_L2);                           // 19 L'
        snap_read_byte (fp, &reg_H2);                           // 20 H'
        snap_read_byte (fp, &reg_A2);                           // 21 A'
        snap_read_byte (fp, &reg_F2);                           // 22 F'
        snap_read_byte (fp, &reg_IYL);                          // 23 IYL
        snap_read_byte (fp, &reg_IYH);                          // 24 IYH
        snap_read_byte (fp, &reg_IXL);                          // 25 IXL
        snap_read_byte (fp, &reg_IXH);                          // 26 IXH

        snap_read_byte (fp, &val);                              // 27 IFF1

        if (val)
        {
            iff1 = 1;
        }
        else
        {
            iff1 = 0;
        }

        snap_read_byte (fp, &iff2);                             // 28 IFF2
        snap_read_byte (fp, &val);                              // 29 Bit 0-1:  Interrupt mode (0, 1 or 2)
                                                                //    Bit 2  :  1=Issue 2 emulation
                                                                //    Bit 3  :  1=Double interrupt frequency
                                                                //    Bit 4-5:  1=High video synchronisation
                                                                //              3=Low video synchronisation
                                                                //              0,2=Normal
                                                                //    Bit 6-7:  0=Cursor/Protek/AGF joystick
                                                                //              1=Kempston joystick
                                                                //              2=Sinclair 2 Left joystick
                                                                //                (or user defined, for version 3 .z80 files)
                                                                //              3=Sinclair 2 Right joystick

        interrupt_mode = val & 0x03;
        uint16_t    idx;

        if (reg_PC == 0x0000)
        {
            uint16_t    additional_header_len;

            snap_read_word (fp, &additional_header_len);        // 30+31 length of additional header following

            if (additional_header_len == 23)
            {
                debug_printf ("snapshot version: 2\n");
                version = 2;
            }
            else
            {
                debug_printf ("snapshot version: 3\n");
                version = 3;
            }

            debug_printf ("additional_header_len=%d\n", additional_header_len);

            snap_read_word (fp, &reg_PC);                       // 32+33 PC
            additional_header_len -= 2;

            for (idx = 0; idx < additional_header_len; idx++ )
            {
                snap_read_byte (fp, &val);

                if (idx == 0)                                   // 34: hardware mode
                {
                                                                // Value:          Meaning in v2           Meaning in v3
                                                                // -----------------------------------------------------
                                                                //  0             48k                     48k
                                                                //  1             48k + If.1              48k + If.1
                                                                //  2             SamRam                  SamRam
                                                                //  3             128k                    48k + M.G.T.
                                                                //  4             128k + If.1             128k
                                                                //  5             -                       128k + If.1
                                                                //  6             -                       128k + M.G.T.
                    if (val == 0 || val == 1)
                    {
                        if (z80_romsize != 0x4000)
                        {
                            debug_printf ("switching to machine: ZX48K\n");
                            set_fname_rom_buf ("48.rom");
                            load_rom ();
                            zx_ram_init (z80_romsize);
#ifdef unix
                            menu_redraw ();
#endif
                        }
                    }
                    else if ((version == 2 && (val == 3 || val == 4)) ||
                             (version == 3 && (val == 4 || val == 5)))
                    {
                        if (z80_romsize == 0x4000)
                        {
                            debug_printf ("switching to machine: ZX128K\n");
                            set_fname_rom_buf ("128.rom");
                            load_rom ();
                            zx_ram_init (z80_romsize);
#ifdef unix
                            menu_redraw ();
#endif
                        }
                    }
                }
                else if (idx == 2)                              // 35: memory paging
                {
                                                                // If in SamRam mode, bitwise state of 74ls259.
                                                                // For example, bit 6=1 after an OUT 31,13 (=2*6+1)
                                                                // If in 128 mode, contains last OUT to 0x7ffd
                                                                // If in Timex mode, contains last OUT to 0xf4

                    if (z80_romsize == 0x8000)                  // we are in 128K mode
                    {
                        zxio_out_port (0x7F, 0xFD, val);        // adjust memory banks
                    }
                }
            }
        }

        if (version == 1)
        {
            uint32_t base_addr = ZX_RAM_BEGIN;

            if (data_compressed)
            {
            }
            else
            {
                for (idx = 0; idx < 49152; idx++)
                {
                    snap_read_byte (fp, &val);
                    zx_ram_set_8(base_addr + idx, val);
                }
            }
        }
        else
        {
            uint8_t     factor      = 0;
            uint16_t    data_len;
            uint8_t     page_no     = 0;
            uint16_t    pos;

            do
            {
                snap_read_word (fp, &data_len);                         // 0+1 length of compressed data following
                                                                        //     0xFFFF means 16384 bytes uncompressed
                snap_read_byte (fp, &page_no);                          // 2   page number

                if (data_len == 0xFFFF)
                {
                    data_compressed = 0;
                    data_len = SNAPSHOT_PAGE_SIZE;
                }
                else
                {
                    data_compressed = 1;
                }

                if (z80_romsize == 0x4000)
                {
                    switch (page_no)
                    {
                        case 4:     steccy_ram_ptr = steccy_rambankptr[2];      break;          // 0x8000
                        case 5:     steccy_ram_ptr = steccy_rambankptr[0];      break;          // 0xC000
                        case 8:     steccy_ram_ptr = steccy_rambankptr[5];      break;          // 0x4000
                        default:    steccy_ram_ptr = NULL;                      break;          // invalid page
                    }

                    debug_printf ("ZX48K: data_len=%d, page number=%d\n", data_len, page_no);
                }
                else
                {
                    steccy_ram_ptr = steccy_rambankptr[page_no - 3];
                    debug_printf ("ZX128K: data_len=%d, page number=%d\n", data_len, page_no);
                }

                pos = 0;

                for (idx = 0; idx < data_len; idx++)
                {
                    snap_read_byte (fp, &val);

                    if (data_compressed)
                    {
                        if (val == 0xED)
                        {
                            snap_read_byte (fp, &val);
                            idx++;

                            if (val == 0xED)
                            {
                                int i;

                                snap_read_byte (fp, &factor);                       // factor
                                idx++;
                                snap_read_byte (fp, &val);                          // value
                                idx++;

                                for (i = 0; i < factor; i++)
                                {
                                    steccy_ram_ptr[pos] = val;
                                    pos++;
                                }
                            }
                            else
                            {
                                steccy_ram_ptr[pos] = 0xED;
                                pos++;
                                steccy_ram_ptr[pos] = val;
                                pos++;
                            }
                        }
                        else
                        {
                            steccy_ram_ptr[pos] = val;
                            pos++;
                        }
                    }
                    else
                    {
                        steccy_ram_ptr[pos] = val;
                        pos++;
                    }
                }

                if ((ch = getc (fp)) != EOF)
                {
                    ungetc (ch, fp);
                }
            } while (ch != EOF);
        }

        cur_PC                  = reg_PC;
        last_ixiyflags          = 0;
        ixflags                 = 0;
        iyflags                 = 0;
        clockcycles             = 0;

        fclose (fp);
        z80_interrupt = 0;
    }
    else
    {
        perror (fname_rom_buf);
        return;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tape_prepare_load() - prepare LOAD Tape (at 0x0556 or later)
 *
 * Parameters:
 * use_regs2 = 0    use_regs2 = 1
 * IX               IX                  IX        - base address
 * DE               DE                  DE        - length of data
 * register F       register F'         Carry = 0 - verify
 * register F       register F'         Carry = 1 - load
 * register A       register A'         A=0x00    - header
 * register A       register A'         A=0xFF    - data block
 *
 * Return values:
 * Carry = 1 - OK
 * Carry = 0 - ERROR
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
tape_prepare_load (uint8_t use_regs2)
{
    uint16_t    base_addr   = GET_IX();                                 // base address
    uint16_t    maxlen      = GET_DE();                                 // maximum length
    uint8_t     load;                                                   // load or verify
    uint8_t     load_data;                                              // load data or load header
    uint8_t     rtc = 0;                                                // 1 = success, 0 = failure

    if (use_regs2)
    {
        load        = (reg_F2 & FLAG_C) ? 1 : 0;                        // load or verify
        load_data   = (reg_A2 != 0x00) ? 1 : 0;                         // load data or load header
    }
    else
    {
        load        = ISSET_FLAG_C() ? 1 : 0;                             // load or verify
        load_data   = (reg_A != 0x00) ? 1 : 0;                          // load data or load header
    }

#ifdef DEBUG
    uint8_t     req_type;
    char        req_name[11];
    uint16_t    req_len;

    if (ram[reg_SP] + (ram[reg_SP + 1] << 8) == 0x771)                  // called from 0x0771
    {
        uint8_t *   ptr         = ram + base_addr - 0x11;               // pointer to requested data

        req_type    = ptr[0];

        if (ptr[1] == 0xFF)
        {
            req_name[0] = '\0';
        }
        else
        {
            int idx;
            memcpy (req_name, ptr + 1, 10);
            req_name[10] = '\0';

            for (idx = 9; idx >= 0; idx--)
            {
                if (req_name[idx] == ' ')
                {
                    req_name[idx] = '\0';
                }
                else
                {
                    break;
                }
            }
        }

        req_len = UINT16_T (ptr[11] + (ptr[12] << 8));

        debug_printf ("req_type = %d\n", req_type);
        debug_printf ("req_name = '%s'\n", req_name);
        debug_printf ("req_len = %d\n", req_len);
        debug_printf ("p1=%02X p2=%02X p3=%02X p4=%02X\n", ptr[13], ptr[14], ptr[15], ptr[16]);
    }
#endif

    if (fname_load_valid)
    {
        rtc = tape_load (fname_load_buf, tape_load_format, base_addr, maxlen, load, load_data);
    }

    if (rtc)                                                    // successful
    {
        reg_D = 0;                                              // reset length
        reg_E = 0;
        SET_FLAG_C();                                           // set carry
    }
    else
    {
        RES_FLAG_C();
    }
    reg_PC = pop16();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tape_prepare_save() - prepare SAVE Tape (at 0x04C2):
 *
 * Parameters:
 * IX        - base address
 * DE        - length of data
 * A=0x00    - header
 * A=0xFF    - data block
 *
 * Return values:
 * Carry = 1 - OK
 * Carry = 0 - ERROR
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
tape_prepare_save (void)
{
    uint16_t    base_addr   = GET_IX();                                 // base address
    uint16_t    maxlen      = GET_DE();                                 // maximum length
    uint8_t     save_data   = (reg_A == 0xFF) ? 1 : 0;                  // save data or save header

    if (fname_save_valid && tape_save (fname_save_buf, base_addr, maxlen, save_data))
    {
        SET_FLAG_C();
    }
    else
    {
        RES_FLAG_C();
    }
    reg_PC = pop16();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_set_fname_load () - set filename for LOAD Tape
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_set_fname_load (const char * fname)
{
    size_t len = strlen (fname);

    if (len > 4)
    {
        len -= 4;

        z80_close_fname_load ();

        if (! strcasecmp (fname + len, ".tap"))
        {
            tape_load_format = TAPE_FORMAT_TAP;
            strncpy (fname_load_buf, fname, 255);
            fname_load_valid = 1;
        }
        else if (! strcasecmp (fname + len, ".tzx"))
        {
            tape_load_format = TAPE_FORMAT_TZX;
            strncpy (fname_load_buf, fname, 255);
            fname_load_valid = 1;
        }
        else if (! strcasecmp (fname + len, ".z80"))
        {
            tape_load_format = TAPE_FORMAT_Z80;
            strncpy (fname_load_buf, fname, 255);
            fname_load_snapshot_valid = 1;
        }
    }
}

void
z80_close_fname_load (void)
{
    fname_load_valid = FALSE;
    fname_load_snapshot_valid = FALSE;
    fname_load_buf[0] = '\0';
    tape_load_close ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_load_rom () - load new rom
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_load_rom (const char * fname)
{
    if (strlen (fname) > 4)
    {
#ifdef QT_CORE_LIB  // TODO: QT includes complete path in fname
        (void) strncpy (fname_rom_buf, fname, sizeof (fname_rom_buf) - 1);
#else
        set_fname_rom_buf (fname);
#endif
        load_rom ();
        zxio_reset ();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_set_fname_save_snapshot() - Set save snapshot file name
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_set_fname_save_snapshot (const char * fname)
{
    strcpy (snapshot_save_fname, fname);
    snapshot_save_valid = 1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_set_fname_save() - set name of TZX file for SAVEing
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_set_fname_save (const char * fname)
{
    z80_close_fname_save ();

    if (strlen (fname) > 4)
    {
        strncpy (fname_save_buf, fname, 255);
        fname_save_valid = 1;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_close_fname_save() - close TZX file for SAVEing
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
z80_close_fname_save (void)
{
    fname_save_valid    = 0;
    fname_save_buf[0]   = '\0';
    tape_save_close ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * serial_output () - output reg_A as character to UART
 *
 * STM32: USART 2 will be used
 * QT:    stdout will be used
 *
 * See also: https://faqwiki.zxnet.co.uk/wiki/Channels_and_streams
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
serial_output (void)
{
    int ch = reg_A;

    if (ch == 0x06)                                     // PRINT #4,"Hello": convert Sinclair TAB into ASCII TAB
    {
        ch = '\t';
    }

    if (ch == '\r')                                     // got CR
    {
#ifdef STM32F4XX
        putchar ('\r');                                 // on STM32F4: convert it to CRNL
#endif
        putchar ('\n');                                 // on unix like systems or QT: convert it to NL
    }
    else
    {
        putchar (ch);
    }

    fflush (stdout);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * serial_input () - read a character from UART
 *
 * STM32: USART 2 will be used
 * QT:    returns dummy characters "1234\r"
 *
 * If data is available, set reg_A to character and set carry flag.
 * If data is not available, reset CARRY flag.
 * If data is End-of-File, reset ZERO and CARRY flag.
 * The basic command INPUT then results in a loop until Carry is set.
 *
 * A CR indicates end of input string. To avoid a bug, we should set bit 5 in TV_FLAG
 *
 * See also: https://faqwiki.zxnet.co.uk/wiki/Channels_and_streams
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define TV_FLAG     23612
static void
serial_input (void)
{
#ifdef STM32F4XX                                                // on STM32F4:
    uint_fast8_t ch;

    if (console_poll (&ch))                                     // poll character
    {
        reg_A = ch;                                             // store chac in register A
        SET_FLAG_C();                                           // carry flag: indicate that character is available

        if (reg_A == '\r')                                      // if char is a CR:
        {
            zx_ram_set_8(TV_FLAG, (1<<5));                      // ZX ROM bugfix: set only bit 5: clear screen on next command input
        }
    }
    else
    {
        RES_FLAG_C();                                           // reset carry: indicate that no character available
    }
#else                                                           // on Linux/Unix/QT: receive dummy string "12345\r"
    static uint8_t ch = '1';                                    // begin with '1'

    reg_A = ch;                                                 // set character
    SET_FLAG_C();                                               // indicate that character is available

    if (ch == '5')                                              // we return only 1234, then CR
    {
        ch = '1';                                               // reset char to '1'
        reg_A = '\r';
        zx_ram_set_8(TV_FLAG, (1<<5));                          // ZX ROM bugfix: set only bit 5: clear screen on next command input
    }
    else
    {
        ch++;                                                   // next char: 2345
    }
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80_idle_time () - use idle time to do something other, or just wait
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static INLINE void
z80_idle_time ()
{
#ifdef QT_CORE_LIB                                              // QT: sleep 10 msec, set z80 interrupt flag every 20msec
    if (clockcycles >= CLOCKCYCLES_PER_10_MSEC)
    {
        static int      cnt = 0;
        static int      last_elapsed = 0;
        int             elapsed = elapsed_timer.elapsed();

        clockcycles -= CLOCKCYCLES_PER_10_MSEC;

        if (elapsed - last_elapsed < SLEEP_MSEC)
        {
            QThread::msleep(static_cast<unsigned long> (SLEEP_MSEC - (elapsed - last_elapsed)));
        }
        else
        {
            QThread::msleep(SLEEP_MSEC);
        }

        last_elapsed = elapsed;
        cnt++;

        if (cnt == 2)                                           // interrupt every 20 msec
        {
            cnt = 0;
            z80_interrupt = 1;
        }
    }

#elif defined FRAMEBUFFER || defined X11                        // unix/linux: sleep 10 msec, set z80 interrupt flag every 20msec
    if (clockcycles >= CLOCKCYCLES_PER_10_MSEC)
    {
        static int cnt = 0;
        clockcycles -= CLOCKCYCLES_PER_10_MSEC;

        struct timespec         elapsed;
        static unsigned long    last_usec;
        unsigned long           usec;

        clock_gettime(CLOCK_REALTIME, &elapsed);
        usec = 1000000 * elapsed.tv_sec + (elapsed.tv_nsec / 1000);

        if (! z80_settings.turbo_mode)
        {
            if (usec - last_usec < SLEEP_USEC)
            {
                usleep (SLEEP_USEC - (usec - last_usec));
            }
            else
            {
                usleep (SLEEP_USEC);
            }
        }

        last_usec = usec;
        cnt++;

        if (cnt == 2)                                                   // interrupt every 20 msec
        {
            cnt = 0;
            z80_interrupt = 1;
            update_display = 1;                                         // only if FRAMEBUFFER or X11
        }
    }

#elif defined STM32F4XX                                                 // STM32: handle devices, set z80 interrupt flag every 20msec

    static int  cnt = 0;
    uint32_t    clockcycles_per_200_usec = CLOCKCYCLES_PER_200_USEC;

    if (! (cnt % 4))
    {
        clockcycles_per_200_usec--;                                     // 75% = 672, 25% = 671 => (75*672 + 25*671) / 2 => 67175 / 2 = 33587
    }

    if (clockcycles >= clockcycles_per_200_usec)
    {
        clockcycles -= clockcycles_per_200_usec;

        static uint32_t     last_uptime;                                // in units of 20us
        uint32_t            stop_time = last_uptime + 10;               // 200 usec (10 * 20us) later...

        if (z80_settings.keyboard & KEYBOARD_ZX)                        // use idle time to call ZX keyboard statemachine
        {
            zxkbd_poll ();
        }

        if (z80_settings.keyboard & KEYBOARD_USB)                       // use ídle time to call USB statemachine
        {
            usb_hid_host_process (FALSE);
        }

        if (joystick_is_online)                                         // use idle time to call joystick statemachine
        {
            if (joystick_state == JOYSTICK_STATE_IDLE)                  // joystick is idle, start new transmission
            {
                static uint32_t jcnt = 0;
                jcnt++;

                if (jcnt == 100)                                        // 100 * 200usec = 20msec
                {
                    jcnt = 0;
                    joystick_start_transmission ();                     // start a transmission only every 20msec
                }
            }
            else                                                        // joystick ist busy, try to check if transmission finished
            {
                (void) joystick_finished_transmission (TRUE);           // ignore rtc, just read in joystick values
            }
        }

        if (z80_settings.turbo_mode)                                    // turbo mode
        {
            uptime = stop_time;                                         // skip pause time
        }
        else
        {
            while (uptime < stop_time)                                  // wait until stop_time reached
            {
                // wait
            }
        }

        last_uptime = uptime;                                           // restore last uptime

        cnt++;

        if (cnt == CLOCKCYCLES_COUNT_200_USEC)                          // Z80 interrupt every 20 msec
        {
            cnt = 0;
            z80_interrupt = 1;
        }
    }
#else
#error "unexpected platform"
#endif
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * z80()
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
#ifdef QT_CORE_LIB
z80Thread::run()
#else
z80 (void)
#endif
{
    uint8_t     opcode;

    while (1)
    {
#if defined STM32F4XX
        if (update_display)
        {
            update_display = 0;
            zxscr_update_display ();
        }
#elif defined FRAMEBUFFER || defined X11
        if (update_display)
        {
            update_display = 0;
            zxscr_update_display ();

#if defined X11
            x11_event ();
#endif
        }

        if (steccy_exit)
        {
            return;
        }
#endif

        if (fname_load_snapshot_valid)
        {
            fname_load_snapshot_valid = 0;
            load_snapshot ();
        }

        if (!ixflags && !iyflags)                                           // M1 cycle
        {
            cur_PC = reg_PC;
#ifdef DEBUG
            if (! debug && iff1 && z80_interrupt)                           // don't call interrupts during debugging
#else
            if (iff1 && z80_interrupt)
#endif
            {
                z80_interrupt = 0;
                iff1 = 0;

                if (zx_ram_get_8(reg_PC) == 0x76)                           // sleeping on HALT
                {
                    reg_PC++;
                }

                push16 (reg_PC);

                if (interrupt_mode == 0 || interrupt_mode == 1)
                {
                    ADD_CLOCKCYCLES (13);
                    reg_PC = 0x0038;
                }
                else if (interrupt_mode == 2)
                {
                    uint16_t vector_addr = UINT16_T ((reg_I << 8) | 0);
                    ADD_CLOCKCYCLES (19);
                    reg_PC = zx_ram_get_16 (vector_addr);
                }
            }

            if (reg_PC == 0x0562)           // official tape loader address is 0x0556,
            {                               // but some games call DI; EX AF,AF' on their own and jump to 0x055A or 0x562
                tape_prepare_load (1);
            }
            else if (reg_PC == 0x4C2)
            {
                tape_prepare_save ();
            }

#if defined QT_CORE_LIB
            while (z80_do_pause)
            {
                QThread::msleep(10);
            }
#elif defined FRAMEBUFFER || defined X11
            if (! z80_focus)
            {
                menu (z80_settings.path);
                z80_focus = TRUE;
            }
#elif defined STM32F4XX
            if (! z80_focus)
            {
                menu ();
#ifdef SSD1963
                zxscr_update_status ();
#endif
                z80_focus = TRUE;
#ifdef ILI9341
                zxscr_display_cached = 0;                                          // redraw ZX screen
#endif
            }
#endif

            if (snapshot_save_valid)
            {
                save_snapshot ();
                snapshot_save_valid = 0;
            }

#ifdef DEBUG_ONLY_RAM
            static uint8_t save_debug = 0;
            if (reg_PC < 0x4000)
            {
                save_debug = debug;
                debug = 0;
            }
            else if (save_debug)
            {
                debug = save_debug;
                save_debug = 0;
            }
#endif
            debug_printf ("\r\nPC=%04X SP=%04X [%02X%02X]  ", reg_PC, reg_SP, ram[reg_SP + 1], ram[reg_SP]);
            debug_printf ("%02X %02X %02X %02X   ", ram[reg_PC], ram[reg_PC + 1], ram[reg_PC + 2], ram[reg_PC + 3]);
            debug_printf ("AF=%02X%02X BC=%02X%02X DE=%02X%02X HL=%02X%02X IX=%02X%02X IY=%02X%02X ",
                          reg_A, reg_F, reg_B, reg_C, reg_D, reg_E, reg_H, reg_L, reg_IXH, reg_IXL, reg_IYH, reg_IYL);
            debug_printf ("S=%d Z=%d C=%d PV=%d  ", ISSET_FLAG_S() ? 1 : 0, ISSET_FLAG_Z() ? 1 : 0, ISSET_FLAG_C() ? 1 : 0, ISSET_FLAG_PV() ? 1 : 0);
        }

        opcode = zx_ram_get_text (reg_PC);

        if (hooks_active)
        {
            switch (reg_PC)
            {
                case SERIAL_OUTPUT: serial_output (); break;
                case SERIAL_INPUT:  serial_input (); break;
            }
        }

        switch (opcode)
        {
            case 0x00:  cmd_nop ();                             break;          // NOP
            case 0x01:  cmd_ld_rr_nn (REG_IDX_BC);              break;          // LD BC,nn
            case 0x02:  cmd_ld_ind_rr_a (REG_IDX_BC);           break;          // LD (BC),A
            case 0x03:  cmd_inc_rr (REG_IDX_BC);                break;          // INC BC
            case 0x04:  cmd_inc_r (REG_IDX_B);                  break;          // INC B
            case 0x05:  cmd_dec_r (REG_IDX_B);                  break;          // DEC B
            case 0x06:  cmd_ld_r_n (REG_IDX_B);                 break;          // LD B,n
            case 0x07:  cmd_rlca ();                            break;          // RLCA
            case 0x08:  cmd_ex_af_af ();                        break;          // EX AF,AF'
            case 0x09:  cmd_add_ii_rr (REG_IDX_BC);             break;          // ADD HL,BC / ADD IX,BC / ADD IY,BC
            case 0x0A:  cmd_ld_a_ind_rr (REG_IDX_BC);           break;          // LD A,(BC)
            case 0x0B:  cmd_dec_rr (REG_IDX_BC);                break;          // DEC BC
            case 0x0C:  cmd_inc_r (REG_IDX_C);                  break;          // INC C
            case 0x0D:  cmd_dec_r (REG_IDX_C);                  break;          // DEC C
            case 0x0E:  cmd_ld_r_n (REG_IDX_C);                 break;          // LD C,n
            case 0x0F:  cmd_rrca ();                            break;          // RRCA

            case 0x10:  cmd_djnz ();                            break;          // DJNZ n
            case 0x11:  cmd_ld_rr_nn (REG_IDX_DE);              break;          // LD DE,nn
            case 0x12:  cmd_ld_ind_rr_a (REG_IDX_DE);           break;          // LD (DE),A
            case 0x13:  cmd_inc_rr (REG_IDX_DE);                break;          // INC DE
            case 0x14:  cmd_inc_r (REG_IDX_D);                  break;          // INC D
            case 0x15:  cmd_dec_r (REG_IDX_D);                  break;          // DEC D
            case 0x16:  cmd_ld_r_n (REG_IDX_D);                 break;          // LD D,n
            case 0x17:  cmd_rla ();                             break;          // RLA
            case 0x18:  cmd_jr ();                              break;          // JR n
            case 0x19:  cmd_add_ii_rr (REG_IDX_DE);             break;          // ADD HL,DE / ADD IX,DE / ADD IY,DE
            case 0x1A:  cmd_ld_a_ind_rr (REG_IDX_DE);           break;          // LD A,(DE)
            case 0x1B:  cmd_dec_rr (REG_IDX_DE);                break;          // DEC DE
            case 0x1C:  cmd_inc_r (REG_IDX_E);                  break;          // INC E
            case 0x1D:  cmd_dec_r (REG_IDX_E);                  break;          // DEC E
            case 0x1E:  cmd_ld_r_n (REG_IDX_E);                 break;          // LD E,n
            case 0x1F:  cmd_rra ();                             break;          // RRA

            case 0x20:  cmd_jr_cond (COND_NZ);                  break;          // JR NZ,n
            case 0x21:  cmd_ld_ii_nn ();                        break;          // LD HL,nn / LD IX,nn / LD IY,nn
            case 0x22:  cmd_ld_ind_nn_hl ();                    break;          // LD (nn),HL
            case 0x23:  cmd_inc_ii ();                          break;          // INC HL / INC IX / INC IY
            case 0x24:  cmd_inc_r (REG_IDX_H);                  break;          // INC H
            case 0x25:  cmd_dec_r (REG_IDX_H);                  break;          // DEC H / DEC IXH / DEC IYH
            case 0x26:  cmd_ld_r_n (REG_IDX_H);                 break;          // LD H,n / LD IXH,n / LD IYH,n
            case 0x27:  cmd_daa ();                             break;          // DAA
            case 0x28:  cmd_jr_cond (COND_Z);                   break;          // JR Z,n
            case 0x29:  cmd_add_ii_ii();                        break;          // ADD HL,HL
            case 0x2A:  cmd_ld_ii_ind_nn ();                    break;          // LD HL,(nn)
            case 0x2B:  cmd_dec_ii ();                          break;          // DEC HL / DEC IX / DEC IY
            case 0x2C:  cmd_inc_r (REG_IDX_L);                  break;          // INC L / INC IXL / INC IYL
            case 0x2D:  cmd_dec_r (REG_IDX_L);                  break;          // DEC L / DEC IXL / DEC IYL
            case 0x2E:  cmd_ld_r_n (REG_IDX_L);                 break;          // LD L,n / LD IXL,n / LD IYL,n
            case 0x2F:  cmd_cpl ();                             break;          // CPL

            case 0x30:  cmd_jr_cond (COND_NC);                  break;          // JR NC,n
            case 0x31:  cmd_ld_sp_nn ();                        break;          // LD SP,nn
            case 0x32:  cmd_ld_ind_nn_a ();                     break;          // LD (nn),A
            case 0x33:  cmd_inc_sp ();                          break;          // INC SP
            case 0x34:  cmd_inc_ind_ii ();                      break;          // INC (HL) / INC (IX + d) / INC (IY + d)
            case 0x35:  cmd_dec_ind_ii ();                      break;          // DEC (HL) / DEC (IX + d) / DEC (IY + d)
            case 0x36:  cmd_ld_ind_ii_n ();                     break;          // LD (HL),n / LD (IX + d),n / LD (IY + d),n
            case 0x37:  cmd_scf ();                             break;          // SCF
            case 0x38:  cmd_jr_cond (COND_C);                   break;          // JR C,n
            case 0x39:  cmd_add_ii_sp();                        break;          // ADD HL,SP
            case 0x3A:  cmd_ld_a_ind_nn ();                     break;          // LD A,(nn)
            case 0x3B:  cmd_dec_sp ();                          break;          // DEC SP
            case 0x3C:  cmd_inc_r (REG_IDX_A);                  break;          // INC A
            case 0x3D:  cmd_dec_r (REG_IDX_A);                  break;          // DEC A
            case 0x3E:  cmd_ld_r_n (REG_IDX_A);                 break;          // LD A,n
            case 0x3F:  cmd_ccf ();                             break;          // CCF

            case 0x40:                                                          // LD B,B
            case 0x41:                                                          // LD B,C
            case 0x42:                                                          // LD B,D
            case 0x43:                                                          // LD B,E
            case 0x44:                                                          // LD B,H
            case 0x45:                                                          // LD B,L
            case 0x47:  cmd_ld_r_r (REG_IDX_B, opcode & 0x07);  break;          // LD B,A
            case 0x46:  cmd_ld_r_ind_ii (REG_IDX_B);            break;          // LD B,(HL)

            case 0x48:                                                          // LD C,B
            case 0x49:                                                          // LD C,C
            case 0x4A:                                                          // LD C,D
            case 0x4B:                                                          // LD C,E
            case 0x4C:                                                          // LD C,H
            case 0x4D:                                                          // LD C,L
            case 0x4F:  cmd_ld_r_r (REG_IDX_C, opcode & 0x07);  break;          // LD C,A
            case 0x4E:  cmd_ld_r_ind_ii (REG_IDX_C);            break;          // LD C,(HL)

            case 0x50:                                                          // LD D,B
            case 0x51:                                                          // LD D,C
            case 0x52:                                                          // LD D,D
            case 0x53:                                                          // LD D,E
            case 0x54:                                                          // LD D,H
            case 0x55:                                                          // LD D,L
            case 0x57:  cmd_ld_r_r (REG_IDX_D, opcode & 0x07);  break;          // LD D,A
            case 0x56:  cmd_ld_r_ind_ii (REG_IDX_D);            break;          // LD D,(HL)

            case 0x58:                                                          // LD E,B
            case 0x59:                                                          // LD E,C
            case 0x5A:                                                          // LD E,D
            case 0x5B:                                                          // LD E,E
            case 0x5C:                                                          // LD E,H
            case 0x5D:                                                          // LD E,L
            case 0x5F:  cmd_ld_r_r (REG_IDX_E, opcode & 0x07);  break;          // LD E,A
            case 0x5E:  cmd_ld_r_ind_ii (REG_IDX_E);            break;          // LD E,(HL)

            case 0x60:                                                          // LD H,B
            case 0x61:                                                          // LD H,C
            case 0x62:                                                          // LD H,D
            case 0x63:                                                          // LD H,E
            case 0x64:                                                          // LD H,H
            case 0x65:                                                          // LD H,L
            case 0x67:  cmd_ld_r_r (REG_IDX_H, opcode & 0x07);  break;          // LD H,A
            case 0x66:  cmd_ld_r_ind_ii (REG_IDX_H);            break;          // LD H,(HL)

            case 0x68:                                                          // LD L,B
            case 0x69:                                                          // LD L,C
            case 0x6A:                                                          // LD L,D
            case 0x6B:                                                          // LD L,E
            case 0x6C:                                                          // LD L,H
            case 0x6D:                                                          // LD L,L
            case 0x6F:  cmd_ld_r_r (REG_IDX_L, opcode & 0x07);  break;          // LD L,A
            case 0x6E:  cmd_ld_r_ind_ii (REG_IDX_L);            break;          // LD L,(HL)

            case 0x70:                                                          // LD (HL),B
            case 0x71:                                                          // LD (HL),C
            case 0x72:                                                          // LD (HL),D
            case 0x73:                                                          // LD (HL),E
            case 0x74:                                                          // LD (HL),H
            case 0x75:                                                          // LD (HL),L
            case 0x77:  cmd_ld_ind_ii_r (opcode & 0x07);        break;          // LD (HL),A
            case 0x76:  cmd_halt ();                            break;          // HALT

            case 0x78:                                                          // LD A,B
            case 0x79:                                                          // LD A,C
            case 0x7A:                                                          // LD A,D
            case 0x7B:                                                          // LD A,E
            case 0x7C:                                                          // LD A,H
            case 0x7D:                                                          // LD A,L
            case 0x7F:  cmd_ld_r_r (REG_IDX_A, opcode & 0x07);  break;          // LD A,A
            case 0x7E:  cmd_ld_r_ind_ii (REG_IDX_A);            break;          // LD A,(HL)

            case 0x80:                                                          // ADD A,B
            case 0x81:                                                          // ADD A,C
            case 0x82:                                                          // ADD A,D
            case 0x83:                                                          // ADD A,E
            case 0x84:                                                          // ADD A,H
            case 0x85:                                                          // ADD A,L
            case 0x87:  cmd_add_a_r (opcode & 0x07);            break;          // ADD A,A
            case 0x86:  cmd_add_a_ind_ii ();                    break;          // ADD A,(HL)

            case 0x88:                                                          // ADC A,B
            case 0x89:                                                          // ADC A,C
            case 0x8A:                                                          // ADC A,D
            case 0x8B:                                                          // ADC A,E
            case 0x8C:                                                          // ADC A,H
            case 0x8D:                                                          // ADC A,L
            case 0x8F:  cmd_adc_a_r (opcode & 0x07);            break;          // ADC A,A
            case 0x8E:  cmd_adc_a_ind_ii ();                    break;          // ADC A,(HL)

            case 0x90:                                                          // SUB A,B
            case 0x91:                                                          // SUB A,C
            case 0x92:                                                          // SUB A,D
            case 0x93:                                                          // SUB A,E
            case 0x94:                                                          // SUB A,H
            case 0x95:                                                          // SUB A,L
            case 0x97:  cmd_sub_a_r (opcode & 0x07);            break;          // SUB A,A
            case 0x96:  cmd_sub_a_ind_ii ();                    break;          // SUB A,(HL)

            case 0x98:                                                          // SBC A,B
            case 0x99:                                                          // SBC A,C
            case 0x9A:                                                          // SBC A,D
            case 0x9B:                                                          // SBC A,E
            case 0x9C:                                                          // SBC A,H
            case 0x9D:                                                          // SBC A,L
            case 0x9F:  cmd_sbc_a_r (opcode & 0x07);            break;          // SBC A,A
            case 0x9E:  cmd_sbc_a_ind_ii ();                    break;          // SBC A,(HL)

            case 0xA0:                                                          // AND A,B
            case 0xA1:                                                          // AND A,C
            case 0xA2:                                                          // AND A,D
            case 0xA3:                                                          // AND A,E
            case 0xA4:                                                          // AND A,H
            case 0xA5:                                                          // AND A,L
            case 0xA7:  cmd_and_a_r (opcode & 0x07);            break;          // AND A,A
            case 0xA6:  cmd_and_a_ind_ii ();                    break;          // AND A,(HL)

            case 0xA8:                                                          // XOR A,B
            case 0xA9:                                                          // XOR A,C
            case 0xAA:                                                          // XOR A,D
            case 0xAB:                                                          // XOR A,E
            case 0xAC:                                                          // XOR A,H
            case 0xAD:                                                          // XOR A,L
            case 0xAF:  cmd_xor_a_r (opcode & 0x07);            break;          // XOR A,A
            case 0xAE:  cmd_xor_a_ind_ii ();                    break;          // XOR A,(HL)

            case 0xB0:                                                          // OR A,B
            case 0xB1:                                                          // OR A,C
            case 0xB2:                                                          // OR A,D
            case 0xB3:                                                          // OR A,E
            case 0xB4:                                                          // OR A,H
            case 0xB5:                                                          // OR A,L
            case 0xB7:  cmd_or_a_r (opcode & 0x07);             break;          // OR A,A
            case 0xB6:  cmd_or_a_ind_ii ();                     break;          // OR A,(HL)

            case 0xB8:                                                          // CP A,B
            case 0xB9:                                                          // CP A,C
            case 0xBA:                                                          // CP A,D
            case 0xBB:                                                          // CP A,E
            case 0xBC:                                                          // CP A,H
            case 0xBD:                                                          // CP A,L
            case 0xBF:  cmd_cp_a_r (opcode & 0x07);             break;          // CP A,A
            case 0xBE:  cmd_cp_a_ind_ii ();                     break;          // CP A,(HL)

            case 0xC0:  cmd_ret_cond (COND_NZ);                 break;          // RET NZ
            case 0xC1:  cmd_pop_rr (REG_IDX_BC);                break;          // POP BC
            case 0xC2:  cmd_jp_cond (COND_NZ);                  break;          // JP NZ,nn
            case 0xC3:  cmd_jp_nn ();                           break;          // JP nn
            case 0xC4:  cmd_call_cond (COND_NZ);                break;          // CALL NZ,nn
            case 0xC5:  cmd_push_rr (REG_IDX_BC);               break;          // PUSH BC
            case 0xC6:  cmd_add_a_n();                          break;          // ADD A,n
            case 0xC7:  cmd_rst (0x0000);                       break;          // RST 00h
            case 0xC8:  cmd_ret_cond (COND_Z);                  break;          // RET Z
            case 0xC9:  cmd_ret ();                             break;          // RET
            case 0xCA:  cmd_jp_cond (COND_Z);                   break;          // JP Z,nn
            case 0xCB:  z80_bits ();                            break;          // BITS
            case 0xCC:  cmd_call_cond (COND_Z);                 break;          // CALL Z,nn
            case 0xCD:  cmd_call ();                            break;          // CALL nn
            case 0xCE:  cmd_adc_a_n();                          break;          // ADC A,n
            case 0xCF:  cmd_rst (0x0008);                       break;          // RST 08h

            case 0xD0:  cmd_ret_cond (COND_NC);                 break;          // RET NC
            case 0xD1:  cmd_pop_rr (REG_IDX_DE);                break;          // POP DE
            case 0xD2:  cmd_jp_cond (COND_NC);                  break;          // JP NC,nn
            case 0xD3:  cmd_out_ind_n_a ();                     break;          // OUT (n),A
            case 0xD4:  cmd_call_cond (COND_NC);                break;          // CALL NC,nn
            case 0xD5:  cmd_push_rr (REG_IDX_DE);               break;          // PUSH DE
            case 0xD6:  cmd_sub_a_n ();                         break;          // SUB A,n
            case 0xD7:  cmd_rst (0x0010);                       break;          // RST 10h
            case 0xD8:  cmd_ret_cond (COND_C);                  break;          // RET C
            case 0xD9:  cmd_exx ();                             break;          // EXX
            case 0xDA:  cmd_jp_cond (COND_C);                   break;          // JP C,nn
            case 0xDB:  cmd_in_a_ind_n ();                      break;          // IN A,(n)
            case 0xDC:  cmd_call_cond (COND_C);                 break;          // CALL C,nn
            case 0xDD:  cmd_ixflags ();                         break;          // IXFLAGS
            case 0xDE:  cmd_sbc_a_n ();                         break;          // SBC A,n
            case 0xDF:  cmd_rst (0x0018);                       break;          // RST 18h

            case 0xE0:  cmd_ret_cond (COND_PO);                 break;          // RET PO
            case 0xE1:  cmd_pop_ii ();                          break;          // POP HL / POP IX / POP IY
            case 0xE2:  cmd_jp_cond (COND_PO);                  break;          // JP PO,nn
            case 0xE3:  cmd_ex_ind_sp_ii ();                    break;          // EX (SP),HL / EX (SP),IX / EX (SP),IY
            case 0xE4:  cmd_call_cond (COND_PO);                break;          // CALL PO,nn
            case 0xE5:  cmd_push_ii ();                         break;          // PUSH HL / PUSH IX / PUSH IY
            case 0xE6:  cmd_and_n ();                           break;          // AND n
            case 0xE7:  cmd_rst (0x0020);                       break;          // RST 20h
            case 0xE8:  cmd_ret_cond (COND_PE);                 break;          // RET PE
            case 0xE9:  cmd_jp_ind_ii ();                       break;          // JP (HL) / JP (IX) / JP (IY)
            case 0xEA:  cmd_jp_cond (COND_PE);                  break;          // JP PE,nn
            case 0xEB:  cmd_ex_de_hl ();                        break;          // EX DE,HL
            case 0xEC:  cmd_call_cond (COND_PE);                break;          // CALL PE,nn
            case 0xED:  z80_extd();                             break;          // EXTD
            case 0xEE:  cmd_xor_a_n ();                         break;          // XOR n
            case 0xEF:  cmd_rst (0x0028);                       break;          // RST 28h

            case 0xF0:  cmd_ret_cond (COND_P);                  break;          // RET P
            case 0xF1:  cmd_pop_af ();                          break;          // POP AF
            case 0xF2:  cmd_jp_cond (COND_P);                   break;          // JP P,nn
            case 0xF3:  cmd_di ();                              break;          // DI
            case 0xF4:  cmd_call_cond (COND_P);                 break;          // CALL P,nn
            case 0xF5:  cmd_push_af ();                         break;          // PUSH AF
            case 0xF6:  cmd_or_a_n ();                          break;          // OR A,n
            case 0xF7:  cmd_rst (0x0030);                       break;          // RST 30h
            case 0xF8:  cmd_ret_cond (COND_M);                  break;          // RET M
            case 0xF9:  cmd_ld_sp_ii ();                        break;          // LD SP,HL / LD SP,IX / LD SP,IY
            case 0xFA:  cmd_jp_cond (COND_M);                   break;          // JP M,nn
            case 0xFB:  cmd_ei ();                              break;          // EI
            case 0xFC:  cmd_call_cond (COND_M);                 break;          // CALL M,nn
            case 0xFD:  cmd_iyflags ();                         break;          // IYFLAGS
            case 0xFE:  cmd_cp_a_n ();                          break;          // CP A,n
            case 0xFF:  cmd_rst (0x0038);                       break;          // RST 38h
            default:
            {
                printf ("z80: Unhandled opcode: %02X PC=%04X\n", opcode, cur_PC);
                fflush (stdout);
            }
        }

        if (ixflags || iyflags)
        {
            if (last_ixiyflags)
            {
                ixflags = 0;
                iyflags = 0;
                last_ixiyflags = 0;
            }
            else
            {
                last_ixiyflags = 1;
            }
        }

        z80_idle_time ();
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * load_ini_file() - load ini file
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
load_ini_file (void)
{
    char    buf[256];
    FILE *  fp = nullptr;
    char *  p;

#ifndef STM32F4XX
    z80_settings.path[0]            = '\0';
#endif
    strcpy (z80_settings.romfile, "128.rom");
    z80_settings.autostart = 1;
    z80_settings.autoload[0]        = '\0';
    z80_settings.keyboard           = KEYBOARD_NONE;
    z80_settings.orientation        = 0;
    z80_settings.rgb_order          = 0;

#ifdef DEBUG
    QByteArray      ba = QDir::currentPath().toLocal8Bit();
    const char *    c_str2 = ba.data();
    printf ("currentpath = %s\n", c_str2);
#endif

#if defined FRAMEBUFFER || defined X11
    char * home = getenv ("HOME");

    if (home)
    {
        sprintf (buf, "%s/.steccy.ini", home);
        fp = fopen (buf, "r");
    }

    if (! fp)
    {
        fp = fopen ("steccy.ini", "r");
    }

#else
    fp = fopen ("steccy.ini", "r");
#endif

    if (fp)
    {
#ifdef DEBUG
        printf ("ini file found!\n");
        fflush (stdout);
#endif

        while (fgets (buf, 256, fp))
        {
            p = strchr (buf, '#');

            if (p)
            {
                *p = '\0';
            }
            else
            {
                p = strchr (buf, ';');

                if (p)
                {
                    *p = '\0';
                }
            }

            size_t len = strlen (buf);

            if (len > 0)
            {
                for (p = buf + len - 1; len > 0 && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); p--, len--)   // trim line
                {
                    *p = '\0';
                }

                p = strchr (buf, '=');

                if (p)
                {
                    *p++ = '\0';

                    if (! strcasecmp (buf, "PATH"))
                    {
#ifndef STM32F4XX                                                                       // ignore path on STM32F4XX
                        strncpy (z80_settings.path, p, MAX_FILENAME_LEN - 1);
                        z80_settings.path[MAX_FILENAME_LEN - 1] = '\0';
#endif
                    }
                    else if (! strcasecmp (buf, "ROM"))
                    {
                        strncpy (z80_settings.romfile, p, MAX_FILENAME_LEN - 1);
                        z80_settings.romfile[MAX_FILENAME_LEN - 1] = '\0';
                    }
                    else if (! strcasecmp (buf, "AUTOSTART"))
                    {
                        if (! strcasecmp (p, "YES"))
                        {
                            z80_settings.autostart = 1;
                        }
                        else if (! strcasecmp (p, "NO"))
                        {
                            z80_settings.autostart = 0;
                        }
                    }
                    else if (! strcasecmp (buf, "AUTOLOAD"))
                    {
                        strncpy (z80_settings.autoload, p, MAX_FILENAME_LEN - 1);
                        z80_settings.autoload[MAX_FILENAME_LEN - 1] = '\0';
                    }
                    else if (! strcasecmp (buf, "KEYBOARD"))                // there can be more than one KEYBOARD line!
                    {
                        if (! strcasecmp (p, "PS2"))
                        {
                            z80_settings.keyboard |= KEYBOARD_PS2;
                        }
                        else if (! strcasecmp (p, "USB"))
                        {
                            z80_settings.keyboard |= KEYBOARD_USB;
                        }
                        else if (! strcasecmp (p, "ZX"))
                        {
                            z80_settings.keyboard |= KEYBOARD_ZX;
                        }
                    }
                    else if (! strcasecmp (buf, "ORIENTATION"))
                    {
                        z80_settings.orientation = atoi (p) % 4;
                    }
                    else if (! strcasecmp (buf, "RGB"))
                    {
                        z80_settings.rgb_order = atoi (p) % 2;
                    }
                }
            }
        }
#if defined (STM32F4XX)
        zxscr_set_display_flags ();
        fclose (fp);
#endif
    }
#ifdef DEBUG
    else
    {
        printf ("ini file not found!\n");
        fflush (stdout);
    }
#endif

    if (z80_settings.keyboard == KEYBOARD_NONE)
    {
        z80_settings.keyboard = KEYBOARD_PS2;                                                       // this is the default
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zx_spectrum() - zx spectrum emulator entry function
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#ifdef QT_CORE_LIB
int
zx_spectrum (int argc, char **argv)
{
    int rtc;

    QApplication app(argc, argv);

    load_ini_file ();
    set_fname_rom_buf (z80_settings.romfile);
    load_rom ();
    zxio_reset ();

    zxscr_init_display ();
    z80Thread thread;
    thread.start ();

    elapsed_timer.start();

    rtc = app.exec ();
    thread.quit ();
    thread.wait ();
    return rtc;
}

#elif defined FRAMEBUFFER || defined X11

void
zx_spectrum (void)
{
    load_ini_file ();
    set_fname_rom_buf (z80_settings.romfile);
    load_rom ();
    zxio_reset ();
    menu_init ();
    z80 ();
}

#elif defined (STM32F4XX)

void
zx_spectrum (void)
{
    load_ini_file ();
    set_fname_rom_buf (z80_settings.romfile);
    load_rom ();
    zxio_reset ();

    if (z80_settings.keyboard & KEYBOARD_PS2)
    {
        ps2key_init ();
    }

    if (z80_settings.keyboard & KEYBOARD_USB)
    {
        usb_hid_host_init ();
    }

    if (z80_settings.keyboard & KEYBOARD_ZX)
    {
        zxkbd_init ();
    }

    z80 ();
}

#endif
