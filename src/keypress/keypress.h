/*-------------------------------------------------------------------------------------------------------------------------------------------
 * keypress.c - map AT keyboard to ZX keyboard/joystick - handle key events
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
#ifndef KEYPRESS_H
#define KEYPRESS_H

#define JOYSTICK_CURSOR         0                                                   // cursor joystick
#define JOYSTICK_SINCLAIR_P1    1                                                   // sinclair joystick player1: 6-0
#define JOYSTICK_SINCLAIR_P2    2                                                   // sinclair joystick player2: 1-5
#define JOYSTICK_KEMPSTON       3                                                   // kempston joystick
#define N_JOYSTICKS             4

#ifdef QT_CORE_LIB                                                                  // QT version

#include <QWidget>
#include <QtGui>
#include <QVBoxLayout>
#include <qlabel.h>
#include <stdint.h>

class KeyPress : public QWidget
{
    Q_OBJECT
public:
    KeyPress(QWidget * parent);

protected:
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

private:
    const char *    mapkey(unsigned int keycode);
    uint8_t         key[2];
    QLabel *        keypressLabel;
    QLabel *        joystickLabel;
    uint8_t         shift_state     = 0;
    uint8_t         joystick_type   = JOYSTICK_KEMPSTON;
    const char *    jostick_names[N_JOYSTICKS] =
    {
        "Cursor Joystick",
        "Sinclair Joystick 1",
        "Sinclair Joystick 2",
        "Kempston Joystick",
    };
};

#else // not QT

#include <stdint.h>

extern uint8_t                  joystick_type;
extern const char *             joystick_names[N_JOYSTICKS];

extern void                     keypress (uint16_t);

#endif // QT

#endif // KEYPRESS_H
