QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileModifier
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           worker.cpp

HEADERS += mainwindow.h \
           worker.h
