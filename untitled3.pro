QT       += core gui widgets multimedia

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated.
DEFINES += QT_DEPRECATED_WARNINGS

# Исходные файлы C++
SOURCES += \
    main.cpp \
    mainwindow.cpp

# Заголовочные файлы
HEADERS += \
    mainwindow.h

# Файлы ресурсов (для звуков)
RESOURCES += \
    resources.qrc
