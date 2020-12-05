#-------------------------------------------------
#
# Project created by QtCreator 2011-09-09T12:09:24
#
#-------------------------------------------------

QT         += widgets network testlib

TARGET      = LightpackTests
DESTDIR     = bin

CONFIG     += console
CONFIG     -= app_bundle

include(../build-config.prf)

CONFIG(gcc):QMAKE_CXXFLAGS += -std=c++11
CONFIG(clang) {
    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
    LIBS += -stdlib=libc++
}

# QMake and GCC produce a lot of stuff
OBJECTS_DIR = stuff
MOC_DIR     = stuff
UI_DIR      = stuff
RCC_DIR     = stuff


DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L../lib -lprismatik-math -lgrab

win32 {
    CONFIG(msvc):DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
    LIBS += -ladvapi32
}

INCLUDEPATH += . \
               ../src \
               ../hooks \
               ../grab/include \
               ../math/include \
               ..


HEADERS += \
    ../common/defs.h \
    ../src/enums.hpp \
    ../src/ApiServerSetColorTask.hpp \
    ../src/ApiServer.hpp \
    ../src/debug.h \
    ../src/Settings.hpp \
    ../src/Plugin.hpp \
    ../src/LightpackPluginInterface.hpp \
    ../src/LightpackCommandLineParser.hpp \
    ../grab/include/calculations.hpp \
    ../math/include/PrismatikMath.hpp \
    SettingsWindowMockup.hpp \
    GrabCalculationTest.hpp \
    LightpackApiTest.hpp \
    lightpackmathtest.hpp \
    ../src/UpdatesProcessor.hpp \
    LightpackCommandLineParserTest.hpp

SOURCES += \
    ../src/ApiServerSetColorTask.cpp \
    ../src/ApiServer.cpp \
    ../src/Settings.cpp \
    ../src/Plugin.cpp \
    ../src/LightpackPluginInterface.cpp \
    ../src/LightpackCommandLineParser.cpp \
    LightpackApiTest.cpp \
    SettingsWindowMockup.cpp \
    GrabCalculationTest.cpp \
    lightpackmathtest.cpp \
    TestsMain.cpp \
    ../src/UpdatesProcessor.cpp \
    LightpackCommandLineParserTest.cpp

win32{
    HEADERS += \
        HooksTest.h \
        ../hooks/ProxyFuncJmp.hpp \
        ../hooks/ProxyFunc.hpp \
        ../hooks/hooksutils.h \
        ../hooks/ProxyFuncVFTable.hpp \
        ../hooks/Logger.hpp

    SOURCES += \
        HooksTest.cpp \
        ../hooks/ProxyFuncJmp.cpp \
        ../hooks/hooksutils.cpp \
        ../hooks/ProxyFuncVFTable.cpp \
        ../hooks/Logger.cpp
}
