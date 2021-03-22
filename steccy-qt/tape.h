/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tape.h - tape functions
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
#ifndef TAPE_H
#define TAPE_H
#include <stdint.h>

/*------------------------------------------------------------------------------------------------------------------------
 * tape constants
 *------------------------------------------------------------------------------------------------------------------------
 */
#define TAPE_FORMAT_TAP     1                                               // format of selected file is TAP
#define TAPE_FORMAT_TZX     2                                               // format of selected file is TZX
#define TAPE_FORMAT_Z80     3                                               // format of selected file is Z80 snapshot

extern uint8_t              tape_load (const char * fname, uint8_t tape_format, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data);
extern uint8_t              tape_save (const char * fname, uint16_t base_addr, uint16_t len, uint8_t save_data);
extern void                 tape_load_close (void);
extern void                 tape_save_close (void);

#endif // TAPE_H
