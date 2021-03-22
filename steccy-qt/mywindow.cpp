/*-------------------------------------------------------------------------------------------------------------------------------------------
 * mywindow.cpp - STECCY's window
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
#include "mywindow.h"
#include "zxscr.h"

MyWidget::MyWidget(QWidget *parent) : QWidget(parent)
{
    int     y;
    int     x;
    int     image_width     = 2 * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZX_SPECTRUM_BORDER_SIZE;
    int     image_height    = 2 * ZX_SPECTRUM_DISPLAY_ROWS    + 2 * ZX_SPECTRUM_BORDER_SIZE;

    this->setGeometry(20, 10, image_width + 120, image_height + 70);

    QRect screenGeometry = QGuiApplication::screens().first()->geometry();
    x = (screenGeometry.width() - this->width()) / 2;
    y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    image = new QImage(image_width, image_height, QImage::Format_RGB888);
    image->fill(QColor(Qt::white).rgb());

    keyPress = new KeyPress(this);
    keyPress->setFocusPolicy (Qt::StrongFocus);
    keyPress->show();

    y = 20;
    x = 2 * ZX_SPECTRUM_DISPLAY_COLUMNS + 2 * ZX_SPECTRUM_BORDER_SIZE + 50;

    pause_button = new QPushButton(this);
    pause_button->setText("Pause");
    pause_button->setGeometry (image_width + 40, y, 60, 20);
    pause_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(pause_button, SIGNAL (released()), this, SLOT (do_pause ()));

    y += 30;

    rom_button = new QPushButton(this);
    rom_button->setText("ROM");
    rom_button->setGeometry (image_width + 40, y, 60, 20);
    rom_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(rom_button, SIGNAL (released()), this, SLOT (openrom ()));

    y += 30;

    reset_button = new QPushButton(this);
    reset_button->setText("Reset");
    reset_button->setGeometry (image_width + 40, y, 60, 20);
    reset_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(reset_button, SIGNAL (released()), this, SLOT (do_reset()));
    reset_button->show ();

    y += 30;

    file_button = new QPushButton(this);
    file_button->setText("Load");
    file_button->setGeometry (image_width + 40, y, 60, 20);
    file_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(file_button, SIGNAL (released()), this, SLOT (openfile ()));

    y += 30;

    autostartCheckBox = new QCheckBox("Autostart", this);
    autostartCheckBox->setGeometry (image_width + 40, y, 80, 20);

    if (z80_get_autostart ())
    {
        autostartCheckBox->setChecked (true);
    }
    else
    {
        autostartCheckBox->setChecked (false);
    }

    autostartCheckBox->setFocusPolicy(Qt::NoFocus);
    connect(autostartCheckBox, SIGNAL(stateChanged(int)), this, SLOT(autostartCheckBoxClicked()));

    y += 30;

    record_button = new QPushButton(this);
    record_button->setText("Save");
    record_button->setGeometry (image_width + 40, y, 60, 20);
    record_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(record_button, SIGNAL (released()), this, SLOT (recordfile ()));

    y += 30;

    snapshot_button = new QPushButton(this);
    snapshot_button->setText("Snapshot");
    snapshot_button->setGeometry (image_width + 40, y, 60, 20);
    snapshot_button->setFocusPolicy(Qt::NoFocus);
    QObject::connect(snapshot_button, SIGNAL (released()), this, SLOT (save_snapshot ()));

    timer = new QTimer(this);

    pixmapLabel = new QLabel (this);
    pixmapLabel->setGeometry (20, 10, image_width, image_height);

    messageLabel = new QLabel("Message", this);
    messageLabel->setGeometry (350, 2 * ZX_SPECTRUM_DISPLAY_ROWS + 2 * ZX_SPECTRUM_BORDER_SIZE + 30, 100, 20);

    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(redraw_display()));
    timer->start(17); // time specified in ms (17 is ~20msec, but why?)
}

void MyWidget::redraw_display ()                                                // 50 Hz
{
    static char buf[32];
    static uint32_t counter;
    static uint32_t seconds;
    static int cnt;
    cnt++;

    if (cnt == 2)                                                               // update image with 25 Hz
    {
        cnt = 0;
        zxscr_update_display ();
        pixmapLabel->setPixmap(QPixmap::fromImage(*image));
        pixmapLabel->show();
    }

    counter++;

    if (counter == 50)
    {
        counter = 0;
        seconds++;
        sprintf (buf, "%5d", seconds);
        messageLabel->setText(buf);
    }
}

void MyWidget::do_reset()
{
    z80_reset ();
}

void MyWidget::do_pause()
{
    z80_pause ();
}

void MyWidget::openrom ()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open ROM File", z80_get_path(), "ROM Files (*.rom)");

    QByteArray ba = fileName.toLocal8Bit();
    const char * fname = ba.data();
    z80_set_fname_rom (fname);
}

void MyWidget::openfile ()
{
    static int      tape_running = FALSE;

    if (tape_running)
    {
        z80_close_fname_load ();
        file_button->setText ("Load");
        file_button->setStyleSheet("color: black");
        tape_running = FALSE;
    }
    else
    {
        QString filename = QFileDialog::getOpenFileName(this, "Open Tape File", z80_get_path(), "TAPE Files (*.tap *.tzx *.z80)");

        if (!filename.isNull())
        {
            QByteArray ba = filename.toLocal8Bit();
            const char * fname = ba.data();
            z80_set_fname_load (fname);
            tape_running = TRUE;
            file_button->setText ("Stop !");
            file_button->setStyleSheet("color: red");
        }
    }
}

void MyWidget::recordfile ()
{
    static int      tape_running = FALSE;

    if (tape_running)
    {
        z80_close_fname_save ();
        record_button->setText ("Record");
        record_button->setStyleSheet("color: black");
        tape_running = FALSE;
    }
    else
    {
        QString filename = QFileDialog::getSaveFileName(this, "Save Tape File", z80_get_path(), "TAPE Files (*.tzx)");

        if (!filename.isNull())
        {
            QByteArray ba = filename.toLocal8Bit();
            const char * fname = ba.data();
            z80_set_fname_save (fname);
            tape_running = TRUE;
            record_button->setText ("Stop !");
            record_button->setStyleSheet("color: red");
        }
    }
}

void MyWidget::save_snapshot ()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Snapshot File", z80_get_path(), "Snapshot Files (*.z80)");

    QByteArray ba = fileName.toLocal8Bit();
    const char * fname = ba.data();
    z80_set_fname_save_snapshot (fname);
}

void MyWidget::autostartCheckBoxClicked ()
{
    z80_set_autostart (autostartCheckBox->isChecked ());
}

void MyWidget::closeEvent(QCloseEvent * event)
{
    event->accept();
    exit (0);
}
