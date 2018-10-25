Lightpack project with Prismatik flavour
---------
*Modified version which includes various improvements for Windows, esp. a Desktop Duplication API Grabber*

**Table of Contents:** <br />
&nbsp;&nbsp;[Short Description](#lightpack-project-with-prismatik-flavour) <br />
&nbsp;&nbsp;[Useful URLs](#useful-urls) <br />
&nbsp;&nbsp;[Build Prismatik with Windows](#prismatik-build-instructions-for-windows) <br />
&nbsp;&nbsp;[Build with Linux](#build-instructions-for-linux) <br />
&nbsp;&nbsp;[Build with OS X](#build-instructions-for-os-x) <br />
&nbsp;&nbsp;[Build Firmware](#firmware-build-instructions) <br />


**Lightpack** is a fully open-source and simple hardware implementation of the backlight for any computer. It's a USB content-driven ambient lighting system.

**Prismatik** is an open-source software we buid to control Lightpack devices. It grabs the screen, analyzes the picture,
calculates resulting colors, and provides soft and gentle lighting with a Lightpack device. Moreover, you can
handle other devices with Prismatik such as Adalight, Ardulight, or even Alienware LightFX system.

##### Main Features:
* Fully open-source under GPLv3 (hardware, software, firmware)
* Cross-platform GUI (Qt)
* USB HID (no need to install any drivers)
* The device is simple to build (just Do-It-Yourself)

##### Useful URLs:
* [Project mothership](https://github.com/psieg/Lightpack/)
* [Original project mothership](https://github.com/woodenshark/Lightpack/)
* [Binary downloads](https://github.com/psieg/Lightpack/releases)
* Wiki with DIY and documentation [ENG](http://code.google.com/p/light-pack/w/list) / [RUS](http://code.google.com/p/lightpack/w/list)
* [Post new issue](https://github.com/psieg/Lightpack/issues)
* [Team](https://github.com/psieg/Lightpack/graphs/contributors)

---

### Prismatik Build Instructions for Windows
#### Prerequisites:
* [Qt SDK](http://qt-project.org/downloads), you may need to set `%QTDIR%`
* [Windows SDK](https://msdn.microsoft.com/en-us/windows/desktop/ff851942.aspx) or [Microsoft DirectX SDK](http://www.microsoft.com/en-us/download/details.aspx?id=6812)
* POSIX shell utilities [MSYS for example](http://www.mingw.org/wiki/MSYS). It may help to have the `PATH` environment variable set for the utilities (Run &rarr; sysdm.cpl &rarr; Advanced &rarr; Environment Variable &rarr; Edit `PATH` system variable (`C:\MinGW\msys\1.0\bin;` for example)
* optional [any](https://wiki.openssl.org/index.php/Binaries) [OpenSSL binaries](https://slproweb.com/products/Win32OpenSSL.html) to include them in the build. You can skip them if you modify `src.pro`
* optional [BASS and BASSWASAPI](http://www.un4seen.com/) for the Sound Visualizer. You can skip them in `build-vars.prf`

#### Build Process:
1. Go to `<repo>/Software`
2. Copy and edit `build-vars.prf` according to your machine
3. Optional: if locales changed: run `update_locales.bat` or `./update_locales.sh` (slow on Windows)
4. Run `scripts/win32/generate_sln.sh`
5. Build `Lightpack.sln` with MSBuild / VisualStudio

#### Building an Installer:
1. Run `scripts/win32/prepare_installer.sh`
2. Build `dist_windows/script.iss` (64bit) or `script32.iss` (32bit) with ISCC (the InnoSetup compiler)

---

### Build Instructions for Linux
#### Prerequisites:
You will need the following packages, usually all of them are in distro's repository:
* qt5-default
* libqt5serialport5-dev
* build-essential
* libgtk2.0-dev
* libusb-1.0-0-dev
* libnotify-dev
* libudev-dev
* qttools5-dev-tools
* if you are using Ubuntu: libappindicator-dev
* not required, but the update checker uses SSL sockets: openssl

#### Build Process:
1. Go to `<repo>/Software`
2. Optional: if locales changed: run `./update_locales.sh`
3. Run `qmake -r`
4. Run `make`

#### Building a deb Package:
1. Run `scripts/linux/prepare_installer.sh`
2. `cd dist_linux` and run `build-deb.sh`

#### Manual Deployment:
Instead of building a deb package, you can:

1. Add a rule for **UDEV**. See comments from `<repo>/Software/dist_linux/deb/etc/udev/rules.d/93-lightpack.rules` for how to do it.
2. Make sure `<repo>/Software/qtserialport/libQt5SerialPort.so.5` is available for loading by *Prismatik* (place it in appropriate dir or use *LD_LIBRARY_PATH* variable)

---

### Build Instructions for OS X
#### Prerequisites:
* Qt SDK (5.0+)
* MacOSX 10.9.sdk

###### Whole Dependencies List for Prismatik 5.10.1:
* QtCore.framework
* QtGui.framework
* QtNetwork.framework
* QtOpenGL.framework

#### Build Process:
1. Download and unpack 5.0+ **Qt SDK** from www.qt-project.org
2. Go to `<repo>/Software`
3. Optional: if locales changed: run `./update_locales.sh`
4. Run `qmake -r`
5. Run `make`


#### Building a dmg package:
1. Run `macdeployqt bin/Prismatik.app -dmg`

---

### Firmware Build Instructions

**Updating Firmware on Windows:**
If you don't want to build the firmware yourself, you can follow the [documentation](https://github.com/Atarity/Lightpack-docs/blob/master/EN/Lightpack_firmware_update_with_FLIP_utility.md) for flashing the latest firmware on Windows.

*Please note that these instructions are for Debian based systems.*

**Compiling Firmware Only:**

1. Install [AVR GCC Toolchain](http://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_GCC_Toolchain): `sudo apt-get install gcc-avr binutils-avr avr-libc`
2. Compile the firmware:
  * `cd Firmware`
  * `make LIGHTPACK_HW=7` (or any other hardware version 4-7)
  * Alternatively, you can do `./build_batch.sh` to build the firmware for all hardware versions
3. The firmware can be found in the same directory (individual build) or *Firmware/hex* (batch build).

**Compiling and Uploading Firmware to Device:**

1. Install [AVR GCC Toolchain](http://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_GCC_Toolchain) and **dfu-programmer**: `sudo apt-get install gcc-avr binutils-avr avr-libc avrdude dfu-programmer`
2. Reboot device to bootloader (via the secret button on the device)
3. Compile and upload the firmware:
  * `cd Firmware`
  * `make LIGHTPACK_HW=7 && make dfu LIGHTPACK_HW=7` (or any other hardware version 4-7)

---

Please let us know if you find mistakes, bugs or errors. Contributions are welcome.<br />
Post new issue : https://github.com/psieg/Lightpack/issues
