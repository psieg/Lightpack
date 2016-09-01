#-------------------------------------------------
#
# Project created by QtCreator 2013-06-11T16:47:52
#
#-------------------------------------------------

DESTDIR = ../lib
TARGET = prismatik-math
TEMPLATE = lib
CONFIG += staticlib

include(../build-config.prf)

INCLUDEPATH += ./include \
               ../ \

SOURCES += \
    PrismatikMath.cpp

HEADERS += \
    include/colorspace_types.h \
    include/PrismatikMath.hpp
