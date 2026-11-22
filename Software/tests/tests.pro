#-------------------------------------------------
#
# Project created by QtCreator 2011-09-09T12:09:24
#
#-------------------------------------------------

QT         += widgets network testlib serialport

TARGET      = LightpackTests
DESTDIR     = bin

CONFIG     += console c++17
CONFIG     -= app_bundle

include(../build-config.prf)

CONFIG(clang) {
    QMAKE_CXXFLAGS += -stdlib=libc++
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
    LIBS += -ladvapi32 -lsetupapi -luser32 -lshlwapi
}

INCLUDEPATH += . \
               ../src \
               ../src/hidapi \
               ../alienfx \
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
    AppVersionTest.hpp \
    ../src/UpdatesProcessor.hpp \
    ../src/AbstractLedDevice.hpp \
    ../src/AbstractLedDeviceUdp.hpp \
    ../src/LedDeviceLightpack.hpp \
    ../src/LedDeviceAdalight.hpp \
    ../src/LedDeviceArdulight.hpp \
    ../src/LedDeviceDrgb.hpp \
    ../src/LedDeviceDnrgb.hpp \
    ../src/LedDeviceWarls.hpp \
    ../src/LedDeviceVirtual.hpp \
    ../src/LedDeviceManager.hpp \
    ../src/SystemSession.hpp \
    ../src/TimeEvaluations.hpp \
    LightpackCommandLineParserTest.hpp \
    SettingsMigrationTest.hpp \
    LedDeviceManagerTest.hpp \
    PluginHardeningTest.hpp

SOURCES += \
    ../src/ApiServerSetColorTask.cpp \
    ../src/ApiServer.cpp \
    ../src/Settings.cpp \
    ../src/Plugin.cpp \
    ../src/LightpackPluginInterface.cpp \
    ../src/LightpackCommandLineParser.cpp \
    ../src/AbstractLedDevice.cpp \
    ../src/AbstractLedDeviceUdp.cpp \
    ../src/LedDeviceLightpack.cpp \
    ../src/LedDeviceAdalight.cpp \
    ../src/LedDeviceArdulight.cpp \
    ../src/LedDeviceDrgb.cpp \
    ../src/LedDeviceDnrgb.cpp \
    ../src/LedDeviceWarls.cpp \
    ../src/LedDeviceVirtual.cpp \
    ../src/LedDeviceManager.cpp \
    ../src/TimeEvaluations.cpp \
    LightpackApiTest.cpp \
    SettingsWindowMockup.cpp \
    GrabCalculationTest.cpp \
    lightpackmathtest.cpp \
    TestsMain.cpp \
    AppVersionTest.cpp \
    ../src/UpdatesProcessor.cpp \
    LightpackCommandLineParserTest.cpp \
    SettingsMigrationTest.cpp \
    LedDeviceManagerTest.cpp

win32{
    HEADERS += \
        ../src/LedDeviceAlienFx.hpp \
        HooksTest.h \
        ../hooks/ProxyFuncJmp.hpp \
        ../hooks/ProxyFunc.hpp \
        ../hooks/hooksutils.h \
        ../hooks/ProxyFuncVFTable.hpp \
        ../hooks/Logger.hpp

    SOURCES += \
        ../src/LedDeviceAlienFx.cpp \
        ../src/hidapi/windows/hid.c \
        HooksTest.cpp \
        ../hooks/ProxyFuncJmp.cpp \
        ../hooks/hooksutils.cpp \
        ../hooks/ProxyFuncVFTable.cpp \
        ../hooks/Logger.cpp
}
