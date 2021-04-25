/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxjoystick.h - definitions for ZX joystick emulation
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
#ifndef LXJOYSTICK_H
#define LXJOYSTICK_H

#define JOYSTICK_CURSOR         0                                                       // cursor joystick
#define JOYSTICK_SINCLAIR_P1    1                                                       // sinclair joystick player1: 6-0
#define JOYSTICK_SINCLAIR_P2    2                                                       // sinclair joystick player2: 1-5
#define JOYSTICK_KEMPSTON       3                                                       // kempston joystick
#define N_JOYSTICKS             4

extern int                      joystick_type;
extern const char *             joystick_names[N_JOYSTICKS];

#endif
