Lightpack project with Prismatik flavour
---------
Just a test of exernal PRs being built by my jenkins
*Modified version which includes various improvements for Windows, esp. a Desktop Duplication API Grabber*

**Table of content:** <br />
&nbsp;&nbsp;[Short description] (#lightpack-project-with-prismatik-flavour) <br />
&nbsp;&nbsp;[Useful URLs] (#useful-urls) <br />
&nbsp;&nbsp;[Build Prismatik with Windows] (#prismatik-build-instructions-for-windows) <br />
&nbsp;&nbsp;[Build with Linux] (#build-instructions-for-linux) <br />
&nbsp;&nbsp;[Build with OS X] (#build-instructions-for-os-x) <br />
&nbsp;&nbsp;[Lightpack Firmware building] (#fimware-building-instructions) <br />


**Lightpack** is an fully open-source and simple hardware implementation of the backlight for any computer. It's USB content-driving ambient lighting system.

**Prismatik** is an open-source software we buid to control Lightpack device. It grabs screen, analize picture,
calculate resulting colors and provide soft and gentle lighting with Lightpack device. Moreother, you can 
handle another devices with Prismatik such as Adalight, Ardulight or even Alienware LightFX system.

#####Main features:
* Fully open-source under GPLv3 (hardware, software, firmware)
* Cross-platform GUI (Qt)
* USB HID (no need to install any drivers)
* The device is simple to build (just Do-It-Yourself) 

#####Useful URLs:
* [Project mothership] (https://github.com/woodenshark/Lightpack/)
* [Original project mothership] (https://code.google.com/p/lightpack/)
* [Binary downloads] (https://github.com/woodenshark/Lightpack/releases)
* Wiki with DIY and documentation [ENG] (http://code.google.com/p/light-pack/w/list) / [RUS] (http://code.google.com/p/lightpack/w/list)
* [Post new issue] (https://github.com/woodenshark/Lightpack/issues)
* [Team] (https://github.com/woodenshark/Lightpack/graphs/contributors)

---

###Prismatik build instructions for Windows
####Prerequisites:
* [Qt SDK](http://qt-project.org/downloads)
* [Microsoft DirectX SDK](http://www.microsoft.com/en-us/download/details.aspx?id=6812)
* POSIX shell utilities [MSYS for example](http://www.mingw.org/wiki/MSYS). Make sure `PATH` environment variable is set for the utilities (Run &rarr; sysdm.cpl &rarr; Advanced &rarr; Environment Variable &rarr; Edit `PATH` system variable (`C:\MinGW\msys\1.0\bin;` for example), path should points directly on the utilities so utilities are available without any subdirectories)

####Build process:
1. build **Prismatik** project
With MSBuild:
1. generate solution with `qmake -tp vc -r .`
2. build with Visual Studio or MSBuild

---

###Build instructions for Linux
####Prerequisites:
You will need the following packages, usually all of them are in distro's repository:
* qt5-default
* libqt5serialport5-dev
* build-essential
* libgtk2.0-dev
* libusb-1.0-0-dev
* libnotify-dev
* libudev-dev
* if you are using Ubuntu: libappindicator-dev

####Build process:
1. go to `<repo>/Software`
2. run ```qmake -r && make```
3. Add a rule for **UDEV**. See comments from `<repo>/Software/dist_linux/deb/etc/udev/rules.d/93-lightpack.rules` for how to do it.
4. Make sure `<repo>/Software/qtserialport/libQt5SerialPort.so.5` is available for loading by *Prismatik* (place it in appropriate dir or use *LD_LIBRARY_PATH* variable)

---

###Build instructions for OS X
####Prerequisites:
* Qt SDK (5.0+)
* MacOSX 10.9.sdk

###### Whole dependencies list for Prismatik 5.10.1:
* QtCore.framework
* QtGui.framework
* QtNetwork.framework
* QtOpenGL.framework

####Build process:
1. Download and unpack 5.0+ **Qt SDK** from www.qt-project.org
4. Build **Prismatik** project

to run Prismatik please make sure PythonQt libs are available for load at runtime 

---

###Firmware build instructions
1. Install [AVR GCC Toolchain] (http://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_GCC_Toolchain)
2. Install **dfu-programmer** for firmware upload with `$ sudo apt-get install dfu-programmer`
3. Compile Prismatik using command line:
    * cd $Lightpack/Firmware
    * make LIGHTPACK_HW=7
4. Reboot device to bootloader and type `make dfu`

---

Please let us know if you find mistakes, bugs or errors.<br />
Post new issue : https://github.com/woodenshark/Lightpack/issues
