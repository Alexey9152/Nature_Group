QT       += core gui network widgets

TARGET = Server
TEMPLATE = app

SOURCES += \
    server_main.cpp \
    Date.cpp \
    expression_server.cpp \
    serverwindow.cpp

HEADERS += \
    Date.h \
    expression_server.h \
    serverwindow.h
