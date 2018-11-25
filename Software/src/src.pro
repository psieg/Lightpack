# -------------------------------------------------
# src.pro
#
# Copyright (c) 2010,2011 Mike Shatohin, mikeshatohin [at] gmail.com
# http://lightpack.tv https://github.com/woodenshark/Lightpack
# Project created by QtCreator 2010-04-28T19:08:13
# -------------------------------------------------

TARGET      = Prismatik
CONFIG(msvc) {
    PRE_TARGETDEPS += ../lib/grab.lib
} else {
    PRE_TARGETDEPS += ../lib/libgrab.a
}
DESTDIR     = ../bin
TEMPLATE    = app
QT         += network widgets
win32 {
    QT += serialport
}
macx {
    QT += serialport
}
# QMake and GCC produce a lot of stuff
OBJECTS_DIR = stuff
MOC_DIR     = stuff
UI_DIR      = stuff
RCC_DIR     = stuff

# Find currect git revision
GIT_REVISION = $$system(git show -s --format="%h")

# For update GIT_REVISION use it:
#   $ qmake Lightpack.pro && make clean && make
#
# Or simply edit this file (add space anythere
# for cause to call qmake) and re-build project

isEmpty( GIT_REVISION ){
    # In code uses #ifdef GIT_REVISION ... #endif
    message( "GIT not found, GIT_REVISION will be undefined" )
} else {
    # Define current mercurial revision id
    # It will be show in about dialog and --help output
    DEFINES += GIT_REVISION=\\\"$${GIT_REVISION}\\\"
}

TRANSLATIONS += ../res/translations/ru_RU.ts \
       ../res/translations/uk_UA.ts
RESOURCES    = ../res/LightpackResources.qrc
RC_FILE      = ../res/Lightpack.rc

include(../build-config.prf)

# Grabber types configuration
include(../grab/configure-grabbers.prf)
DEFINES += $${SUPPORTED_GRABBERS}

LIBS    += -L../lib -lgrab -lprismatik-math

CONFIG(gcc):QMAKE_CXXFLAGS += -std=c++11
CONFIG(clang) {
    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
    LIBS += -stdlib=libc++
}

unix:!macx{
    CONFIG    += link_pkgconfig debug
    PKGCONFIG += libusb-1.0

    DESKTOP = $$(XDG_CURRENT_DESKTOP)

    equals(DESKTOP, "Unity") {
        DEFINES += UNITY_DESKTOP
        PKGCONFIG += gtk+-2.0 appindicator-0.1 libnotify
    }

    LIBS += -L../qtserialport/lib -lQt5SerialPort
    QMAKE_LFLAGS += -Wl,-rpath=/usr/lib/prismatik
}

win32 {
    CONFIG(msvc) {
        # This will suppress many MSVC warnings about 'unsecure' CRT functions.
        DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
        # Parallel build
        QMAKE_CXXFLAGS += /MP
        # Place *.lib and *.exp files in ../lib
        QMAKE_LFLAGS += /IMPLIB:..\\lib\\$(TargetName).lib
    }

    # Windows version using WinAPI for HID
    LIBS    += -lsetupapi
    # For QSerialDevice
    LIBS    += -luuid -ladvapi32

    !isEmpty( DIRECTX_SDK_DIR ) {
        LIBS += -L$${DIRECTX_SDK_DIR}/Lib/x86
    }
    LIBS    += -lwsock32 -lshlwapi -lole32 -ldxguid

    SOURCES += hidapi/windows/hid.c

    #DX9 grab
    LIBS    += -lgdi32 -ld3d9

    QMAKE_CFLAGS += -O2 -ggdb
    # Windows version using WinAPI + GDI + DirectX for grab colors

    LIBS    += -lwsock32 -lshlwapi -lole32

    LIBS    += -lpsapi
    LIBS    += -lwtsapi32

    CONFIG(msvc) {
        QMAKE_POST_LINK = cd $(TargetDir) $$escape_expand(\r\n)\
            $$[QT_INSTALL_BINS]/windeployqt --no-angle --no-svg --no-translations --no-compiler-runtime \"$(TargetName)$(TargetExt)\" $$escape_expand(\r\n)\
            if $(PlatformToolsetVersion) LEQ 140 copy /y \"$(VcInstallDir)redist\\$(PlatformTarget)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\msvcp$(PlatformToolsetVersion).dll\" .\ $$escape_expand(\r\n)\
            if $(PlatformToolsetVersion) GTR 140 copy /y \"$(VcInstallDir)Redist\\MSVC\\$(VCToolsRedistVersion)\\$(PlatformTarget)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\msvcp*.dll\" .\ $$escape_expand(\r\n)\
            if $(PlatformToolsetVersion) LSS 140 copy /y \"$(VcInstallDir)redist\\$(PlatformTarget)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\msvcr$(PlatformToolsetVersion).dll\" .\ $$escape_expand(\r\n)\
            if $(PlatformToolsetVersion) EQU 140 copy /y \"$(VcInstallDir)redist\\$(PlatformTarget)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\vcruntime$(PlatformToolsetVersion).dll\" .\ $$escape_expand(\r\n)\
            if $(PlatformToolsetVersion) GTR 140 copy /y \"$(VcInstallDir)Redist\\MSVC\\$(VCToolsRedistVersion)\\$(PlatformTarget)\\Microsoft.VC$(PlatformToolsetVersion).CRT\\vcruntime*.dll\" .\ $$escape_expand(\r\n)
        !contains(DEFINES, NO_OPENSSL) {
            QMAKE_POST_LINK += copy /y \"$${OPENSSL_DIR}\\ssleay32.dll\" .\ $$escape_expand(\r\n)\
                copy /y \"$${OPENSSL_DIR}\\libeay32.dll\" .\ $$escape_expand(\r\n)
        }

    } else {
        warning("unsupported setup - update src.pro to copy dependencies")
    }
	
	contains(DEFINES,BASS_SOUND_SUPPORT) {
		INCLUDEPATH += $${BASS_DIR}/c/ \
			$${BASSWASAPI_DIR}/c/
		
		contains(QMAKE_TARGET.arch, x86_64) {
			LIBS += -L$${BASS_DIR}/c/x64/ -L$${BASSWASAPI_DIR}/c/x64/
		} else {
			LIBS += -L$${BASS_DIR}/c/ -L$${BASSWASAPI_DIR}/c/		
		}
		
		LIBS	+= -lbass -lbasswasapi
		
		contains(QMAKE_TARGET.arch, x86_64) {
			QMAKE_POST_LINK += cd $(TargetDir) $$escape_expand(\r\n)\
				copy /y \"$${BASS_DIR}\\x64\\bass.dll\" .\ $$escape_expand(\r\n)\
				copy /y \"$${BASSWASAPI_DIR}\\x64\\basswasapi.dll\" .\
		} else {
			QMAKE_POST_LINK += cd $(TargetDir) $$escape_expand(\r\n)\
				copy /y \"$${BASS_DIR}\\bass.dll\" .\ $$escape_expand(\r\n)\
				copy /y \"$${BASSWASAPI_DIR}\\basswasapi.dll\" .\	
		}
	}
}

unix:!macx{
    # Linux version using libusb and hidapi codes
    SOURCES += hidapi/linux/hid-libusb.c
    # For X11 grabber
    LIBS +=-lXext -lX11

    QMAKE_CXXFLAGS += -std=c++11
}

macx{
    QMAKE_LFLAGS += -F/System/Library/Frameworks
    # MacOS version using libusb and hidapi codes
    SOURCES += hidapi/mac/hid.c
    LIBS += \
            -framework Cocoa \
            -framework Carbon \
            -framework CoreFoundation \
            #-framework CoreServices \
            -framework Foundation \
 #           -framework CoreGraphics \
            -framework ApplicationServices \
            -framework OpenGL \
            -framework IOKit \

    ICON = ../res/icons/Prismatik.icns

    QMAKE_INFO_PLIST = ./Info.plist

	#see build-vars.prf
    #isEmpty( QMAKE_MAC_SDK_OVERRIDE ) {
    #    # Default value
    #    # For build universal binaries (native on Intel and PowerPC)
    #    QMAKE_MAC_SDK = macosx10.9
    #} else {
    #    message( "Overriding default QMAKE_MAC_SDK with value $${QMAKE_MAC_SDK_OVERRIDE}" )
    #    QMAKE_MAC_SDK = $${QMAKE_MAC_SDK_OVERRIDE}
    #}

    CONFIG(clang) {
        QMAKE_CXXFLAGS += -x objective-c++
    }
}

# Generate .qm language files
QMAKE_MAC_SDK = macosx
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
system($$[QT_INSTALL_BINS]/lrelease src.pro)

INCLUDEPATH += . \
               .. \
               ./hidapi \
               ../grab \
               ../alienfx \
               ../grab/include \
               ../math/include \
               ./stuff \

SOURCES += \
    LightpackApplication.cpp  main.cpp   SettingsWindow.cpp  Settings.cpp \
    GrabWidget.cpp  GrabConfigWidget.cpp \
    LogWriter.cpp \
    LedDeviceLightpack.cpp \
    LedDeviceAdalight.cpp \
    LedDeviceArdulight.cpp \
    LedDeviceVirtual.cpp \
    ColorButton.cpp \
    ApiServer.cpp \
    ApiServerSetColorTask.cpp \
    MoodLampManager.cpp \
	LiquidColorGenerator.cpp \
    LedDeviceManager.cpp \
    SelectWidget.cpp \
    GrabManager.cpp \
    AbstractLedDevice.cpp \
    PluginsManager.cpp \
    Plugin.cpp \
    LightpackPluginInterface.cpp \
    TimeEvaluations.cpp \
    SessionChangeDetector.cpp \
    wizard/ZonePlacementPage.cpp \
    wizard/Wizard.cpp \
    wizard/WizardPageUsingDevice.cpp \
    wizard/SelectProfilePage.cpp \
    wizard/MonitorIdForm.cpp \
    wizard/MonitorConfigurationPage.cpp \
    wizard/LightpackDiscoveryPage.cpp \
    wizard/ConfigureDevicePage.cpp \
    wizard/SelectDevicePage.cpp \
    wizard/GlobalColorCoefPage.cpp \
    wizard/CustomDistributor.cpp \
    systrayicon/SysTrayIcon.cpp \
    UpdatesProcessor.cpp \
    LightpackCommandLineParser.cpp

HEADERS += \
    LightpackApplication.hpp \
    SettingsWindow.hpp \
    Settings.hpp \
    SettingsDefaults.hpp \
    version.h \
    TimeEvaluations.hpp \
    GrabManager.hpp \
    GrabWidget.hpp \
    GrabConfigWidget.hpp \
    debug.h \
    LogWriter.hpp \
    alienfx/LFXDecl.h \
    alienfx/LFX2.h \
    LedDeviceLightpack.hpp \
    LedDeviceAdalight.hpp \
    LedDeviceArdulight.hpp \
    LedDeviceVirtual.hpp \
    ColorButton.hpp \
    ../common/defs.h \
    enums.hpp         ApiServer.hpp     ApiServerSetColorTask.hpp \
    hidapi/hidapi.h \
    ../../CommonHeaders/COMMANDS.h \
    ../../CommonHeaders/USB_ID.h \
    MoodLampManager.hpp \
	LiquidColorGenerator.hpp \
    LedDeviceManager.hpp \
    SelectWidget.hpp \
    ../common/D3D10GrabberDefs.hpp \
    AbstractLedDevice.hpp \
    PluginsManager.hpp \
    Plugin.hpp \
    LightpackPluginInterface.hpp \
    SessionChangeDetector.hpp \
    wizard/ZonePlacementPage.hpp \
    wizard/Wizard.hpp \
    wizard/WizardPageUsingDevice.hpp \
    wizard/SettingsAwareTrait.hpp \
    wizard/SelectProfilePage.hpp \
    wizard/MonitorIdForm.hpp \
    wizard/MonitorConfigurationPage.hpp \
    wizard/LightpackDiscoveryPage.hpp \
    wizard/ConfigureDevicePage.hpp \
    wizard/SelectDevicePage.hpp \
    wizard/GlobalColorCoefPage.hpp \
    types.h \
    wizard/AreaDistributor.hpp \
    wizard/CustomDistributor.hpp \
    systrayicon/SysTrayIcon.hpp \
    UpdatesProcessor.hpp \
    LightpackCommandLineParser.hpp

contains(DEFINES,BASS_SOUND_SUPPORT) {
    SOURCES += SoundManager.cpp
    HEADERS += SoundManager.hpp
}

win32 {
    SOURCES += LedDeviceAlienFx.cpp
    HEADERS += LedDeviceAlienFx.hpp
}

FORMS += SettingsWindow.ui \
    GrabWidget.ui \
    GrabConfigWidget.ui \
    wizard/ZonePlacementPage.ui \
    wizard/Wizard.ui \
    wizard/SelectProfilePage.ui \
    wizard/MonitorIdForm.ui \
    wizard/MonitorConfigurationPage.ui \
    wizard/LightpackDiscoveryPage.ui \
    wizard/ConfigureDevicePage.ui \
    wizard/SelectDevicePage.ui \
    wizard/GlobalColorCoefPage.ui

#
# QtSingleApplication
#
include(qtsingleapplication/src/qtsingleapplication.pri)

OTHER_FILES += \
    Info.plist
