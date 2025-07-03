QT += core gui widgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = AlgebraClient
TEMPLATE = app
SOURCES += main.cpp \
           mainwindow.cpp
HEADERS += mainwindow.h
