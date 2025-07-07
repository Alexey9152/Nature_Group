QT += core gui widgets multimedia

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

TARGET = ClockApp
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc \
    resources.qrc
