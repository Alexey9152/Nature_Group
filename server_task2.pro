QT += core gui network widgets

CONFIG += c++17

TARGET = Server
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp

HEADERS += mainwindow.h

RESOURCES += resources.qrc
