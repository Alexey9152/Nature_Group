QT += core network
CONFIG += console
CONFIG -= app_bundle
TARGET = ExpressionServer
SOURCES += expression_server.cpp \
           server_main.cpp
HEADERS += expression_server.h
