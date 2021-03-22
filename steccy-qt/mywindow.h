/*-------------------------------------------------------------------------------------------------------------------------------------------
 * mywindow.h - STECCY's window
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2019-2020 Frank Meyer - frank(at)fli4l.de
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
#ifndef MYWINDOW_H
#define MYWINDOW_H

#include <QApplication>
#include <QWidget>
#include <QtGui>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QColorDialog>
#include <qobject.h>
#include <qlabel.h>
#include "keypress.h"
#include "z80.h"

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    MyWidget(QWidget *parent = nullptr);
    QImage *                image;
    KeyPress *              keyPress;

protected:
    void closeEvent(QCloseEvent *);

public slots:
    void redraw_display ();
    void do_pause();
    void do_reset();
    void openfile();
    void openrom();
    void recordfile();
    void save_snapshot();
    void autostartCheckBoxClicked();

private:
    QPushButton *   rom_button;
    QPushButton *   snapshot_button;
    QPushButton *   reset_button;
    QPushButton *   file_button;
    QPushButton *   record_button;
    QPushButton *   pause_button;
    QTimer *        timer;
    QLabel *        pixmapLabel;
    QLabel *        messageLabel;
    QCheckBox *     autostartCheckBox;
};

#endif // MYWINDOW_H
