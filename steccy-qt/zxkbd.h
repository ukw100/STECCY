/*-------------------------------------------------------------------------------------------------------------------------------------------
 * zxkbd.h - ZX Spectrum emulator ZX keyboard functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 Frank Meyer - frank(at)fli4l.de
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

#define ZX_KBD_ROWS         8                                                   // 8 keyboard rows
#define ZX_KBD_COLS         5                                                   // 5 keyboard columns
#define ZX_KBD_COLMASK      0x1F                                                // lower 5 bits of byte
#define ZX_KBD_EXT_COLMASK  0x3F                                                // lower (5 + 1) bits of byte for extended ZX keyboard

extern uint8_t              zxkbd_matrix[ZX_KBD_ROWS];                          // ZX keyboard matrix: 0 = pressed, 1 = released

extern void                 zxkbd_init (void);
extern void                 zxkbd_poll (void);
extern void                 zxkbd_emulate_ps2keys (void);
