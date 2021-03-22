#-------------------------------------------------
#
# Project created by QtCreator 2019-08-28T12:54:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = steccy
TEMPLATE = app


SOURCES += main.cpp\
    keypress.cpp \
        mainwindow.cpp \
    mywindow.cpp \
    tape.cpp \
    z80.cpp \
    zxio.cpp \
    zxram.cpp \
    zxscr.cpp

HEADERS  += mainwindow.h \
    keypress.h \
    mywindow.h \
    tape.h \
    z80.h \
    zxio.h \
    zxram.h \
    zxscr.h

FORMS    += mainwindow.ui
