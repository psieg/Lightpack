*In compliance with the [GPL-3.0](https://opensource.org/licenses/GPL-3.0) license: I declare that this version of the program contains my modifications, which can be seen through the usual "git" mechanism.*  


2022-11  
Contributor(s):  
Patrick Siegler  
>Resume grabbing also on successful device callsThe LedDeviceManager emits ioDeviceSuccess for ledDeviceCommandCompleted, but from there it does not trigger a re-open because it isn't actually an I/O failure.I'd rather not stop emitting the signal there as it has been like this for 11 years, so instead we simply resume grabbing if an error was only temporary.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-02  
Contributor(s):  
Patrick Siegler  
>Qt6 fixes for things not compiled by default  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2022-01  
Contributor(s):  
Patrick  
Patrick Siegler  
Roman Kalukiewicz  
>Replace qt5-default with qtbase5-dev and qtchooser as it's being deprecated.  
>Merge pull request #511 from romkal/hdrFix HDR flashing  
>Fix and add zh-CN translation file  
>Bump to 5.11.2.30  
>Added Polish translation.Updated one file to use correct plural messages, as in Polish itactually matters.  
>Merge pull request #516 from romkal/polishAdded Polish translation.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-12  
Contributor(s):  
Roman Kalukiewicz  
>Update README.mdThe build system assumes OpenSSL 1.1. Installation of 3.0 alone won't work.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-09  
Contributor(s):  
Patrick Siegler  
>Bump to 5.11.2.30  
>Fix how signals are disconnected in LedDeviceManager (#331)This was introduced in b46461185e9934cd4d8aef713535e841c07fe5b3. Instead of disconnecting this=>m_ledDevice, it disconnects this=>*, breaking recovery logic  
>Open device before updating settings, which will call setColors, writing to the device (#331)  
>Fix default state of Session bools (#331)  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-07  
Contributor(s):  
Patrick  
zomfg  
Patrick Siegler  
>grabwidget: "position" typo  
>Stick to space indentation in in .pro files  
>qt6: UpdatesProcessor: Remove QStringRef, wrap comparisons in QStrings  
>Merge pull request #479 from psieg/iosuccessSend ioDeviceSuccess from non-Lightpack devices  
>qt6: Update windeployqt command for qt6  
>qt6: Fix OS version logging  
>Merge pull request #485 from psieg/qt6Qt6 Windows tweaks  
>Merge pull request #482 from zomfg/hotfix/typograbwidget: "position" typo  
>qt6: Fix qt __cplusplus detection/value in msvc  
>Bump to 5.11.2.29  
>Merge pull request #483 from zomfg/feature/qt6-housekeepingqt6 housekeeping  
>qt6: UpdatesProcessor: Use QStringLiteral instead of QString  
>minor compiler warnings  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-06  
Contributor(s):  
Patrick  
zomfg  
Patrick Siegler  
>udp led devices: allow bypassing color modifications  
>Merge pull request #473 from zomfg/hotfix/allow-skipping-color-modificationudp led devices: allow bypassing color modifications  
>Send ioDeviceSuccess from non-Lightpack devicesThis should trigger the recovery mechanics in LedDeviceManager  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-05  
Contributor(s):  
Patrick  
Patrick Siegler  
>Merge pull request #462 from psieg/multi_instanceMulti instance mode  
>Bump to 5.11.2.28  
>Add multi-instance mode through config-dir command line argument  
>Mention multiple monitors in README  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2021-01  
Contributor(s):  
Patrick  
>Merge pull request #428 from zomfg/hotfix/api-string-formatapi server: fixed string format with ints  
>Merge pull request #426 from zomfg/feature/compile-time-x86-separationcompile time x86 separation  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-12  
Contributor(s):  
Patrick Siegler  
zomfg  
Sergio  
Patrick  
>grab widget: removed old feature default  
>zone placement page: _x0,_y0 using wrong screen  
>api server: fixed string format with ints  
>grab: calculations: ifdef annotations  
>Merge pull request #363 from zomfg/feature/led-wizard-tweaksled wizard tweaks  
>Merge remote-tracking branch 'origin/pr/413'# Conflicts:#	Software/src/GrabWidget.cpp#	Software/src/LedDeviceManager.cpp#	Software/src/LightpackApplication.cpp#	Software/src/src.pro#	Software/src/wizard/ConfigureUdpDevicePage.cpp#	Software/src/wizard/MonitorConfigurationPage.cpp#	Software/src/wizard/SelectDevicePage.cpp  
>Merge pull request #424 from zomfg/hotfix/macos-xcode-pathsrc.pro: use selected xcode path  
>grab: calculations tabs  
>Merge pull request #419 from zomfg/hotfix/configure-device-skip-udpWizard hotfix  
>Merge pull request #418 from zomfg/feature/clazy2housekeeping 2  
>Merge pull request #423 from zomfg/hotfix/m1hotfix Rosetta 2  
>wizard: gcc complaints  
>Merge pull request #411 from zomfg/feature/nogui-warninglightpack application: show warnings in nogui mode when receiving messages  
>zone placement page: one shot timer for number of leds changes  
>grab widget: setId(), setFellows()  
>wizard: per display zone placement / coef config  
>grab widget: typo  
>clazy: remaining new connects  
>Merge pull request #412 from zomfg/feature/enable-log-buttonenable open logs button when folder exists  
>clazy: minor fix  
>zone placement page: led number timer comments  
>wizard: skip UDP page from serial page  
>wizard: less iterators on qmaps  
>readme: building on linux  
>Merge pull request #420 from zomfg/feature/linux-buildsci linux builds  
>zone placement page: reusable zone pool  
>wizard: zone placement page: bring window forward  
>grab: macos: updated simd detection  
>qmake: check for pkg-config  
>Bump to 5.11.2.25  
>Bump to 5.11.2.27  
>grab: calculations: re-arranged simd ifdef  
>grab: calculations: ifdef sse/avx  
>clazy: comments  
>clazy: .keys()/.values() to iterators  
>src.pro: use selected xcode path  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-11  
Contributor(s):  
Patrick  
zomfg  
Patrick Siegler  
>Merge pull request #381 from zomfg/feature/unhide-expert-tabExpert tab  
>grab widget: split move / resize setting  
>qt6: grab widget: mouse event position  
>settings window: less strict system locale  
>grab widget: split move and resize setting  
>grab widget: wba + enabled  
>clazy: qstrings  
>abstract device: init member vars with SettingsDefaults  
>clazy: signals/slots, devices, assert fixes  
>settings window: enable open logs button if the folder exists  
>qt6: text codec  
>qdesktopwidget includes  
>led device manager: longer command timeout  
>wizard: clazy cleanup  
>qt6: regexp class  
>Add a setting for dithering, default false.  
>Add Dithering setting to device initialization  
>clazy: signals/slots  
>clazy: const-char-pointer  
>clazy: msvc  
>clazy: more chrono literals  
>qt6: include Plugin (does not like fw declaration)  
>clazy: Q_DISABLE_COPY  
>clazy: chrono-literals  
>log writer: set logs dir when disabled too  
>clazy: mixed const iters  
>clazy: d3d10grabber: new connects  
>remaining QDesktopWidgets  
>clazy: c++17  
>lightpack application: show warnings in nogui mode when receivingmessages  
>unused qtmetaobject  
>Merge pull request #404 from psieg/dithering_settingAdd a setting for dithering, default false.  
>clazy: device: bad emits  
>qdesktopwidget include  
>drgb/virtual devices: const& loop  
>clazy: lightpack application / grab manager: new connects  
>clazy: new connects  
>Merge pull request #403 from zomfg/feature/plugin-verbosityplugins fix/logs  
>zone placement page: show disabled widgets  
>Bump to 5.11.2.25  
>qt includes  
>clazy: qstringliterals, casts, signals/slots, pass by ref  
>clazy: settings window: new connects  
>clazy: short qstringliterals with args  
>clazy: some reserve()  
>clazy: minor qmake fixes  
>qmake c++17  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-10  
Contributor(s):  
zomfg  
Patrick Siegler  
>Bump to 5.11.2.24  
>readme: plugin template link  
>setting defaults: send data only if colors changes default to false  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-09  
Contributor(s):  
Michael Fahrbach  
Patrick  
>Merge pull request #394 from psieg/dithering_cleanDithering  
>Move dithering to its own method  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-08  
Contributor(s):  
Michael Fahrbach  
zomfg  
Patrick  
Denis Klychkov  
>wizard: zone placement: update bottom leds on top/side number change  
>Fix LED device creationAfter some changes `break` statements were lost in the `switch`.  
>Merge pull request #377 from kdsx/fix_led_device_creationFix LED device creation  
>settings window: hide ping device for non-Lighpacks  
>grab widget: white "OFF" text  
>Merge pull request #386 from zomfg/hotfix/startup-framesstartup frames  
>settings defaults: disable API  
>settings window: open logs button: don't hide, better tooltip text  
>log writer: return log dir  
>settings window: expert: fill tooltips with actual defaults  
>grab widget: removed unused white color index  
>settings window: expert: open log dir button  
>settings window: removed checkOutdatedGrabber  
>Move dithering to AbstractLedDevice  
>settings window: renamed Experimental to Expert  
>moodlamp manager: check lamp enabled on set colors  
>settings: removed expert mode setting, experimental tab always shown  
>led device manager: dont update colors when lights off  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-07  
Contributor(s):  
zomfg  
Patrick  
>Merge pull request #361 from hashworks/linkerVarsRespect host linker variables  
>Merge pull request #370 from zomfg/hotfix/nogui-crashnogui crash  
>lightpack application: more nogui checks  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-06  
Contributor(s):  
zomfg  
Sergio  
Patrick  
hashworks  
>readme: screenshots formatting  
>device wizard: zone placement: presets take into account order/offset  
>zone placement page: removed some pointers  
>grab config widget: allow hiding of enable/coef elements  
>device wizard: zone placement: keep stand width/thickness betweenpresets  
>device wizard: select device: preselect current device  
>zone placement page: top/side/bottom margins  
>readme: logo  
>readme: features with screenshots, device/protocol list  
>Respect host linker variablesNot doing so can result in problems during build, see [1]Just for the record, without this patch a build resulted in the followingnamcap warnings on Arch Linux:* ELF file ('usr/bin/prismatik') lacks FULL RELRO, check LDFLAGS* Unused shared library '/usr/lib/libGL.so.1'[1] https://bugs.archlinux.org/task/26435  
>wizard indents  
>grab widget: 16px icon  
>wizard: transient settings: added zoneEnable map  
>zone placement: distributor: use led count instead of coordinates forchanging direction  
>zone placement page: skip corners  
>device wizard: zone placement: rename Custom to Apply  
>Merge pull request #352 from zomfg/feature/power-limitbrightness/power limit  
>readme: minor formatting fix  
>settings window: brightness cap help  
>power limit wizard UI  
>grab widget: better color handling  
>zone placement page: init grab widget with AllowEnableConfig  
>grab widget: replaced arrow with gear icon  
>grab widget: lower settings button size  
>grab widget: split AllowEnableCoefsConfig  
>readme: version shields  
>zone placement page: fixed numberOfLeds signal error  
>Merge pull request #358 from zomfg/readme-updateReadme update  
>power limit no-UI  
>readme: arch version shield update  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-05  
Contributor(s):  
Patrick  
zomfg  
Patrick Siegler  
>Merge pull request #340 from zomfg/feature/allow-random-width-widgetsgrabberbase: don't trim preparedRect width to multiple of 4  
>grab widget: QWheelEvent angleDelta  
>widget wizard: andromeda: make bottom even number  
>device brightness cap  
>hex to Qt::hex  
>led device manager: set brightness cap fix  
>grab winapi: set bytesPerRow  
>x11 grabber: use AllPlanes instead of 0x00ffff  
>widget wizard: zone config: typo  
>settigns window: build time qt version  
>Merge pull request #317 from zomfg/feature/simd-color-averagingSIMD color averaging  
>Bump to 5.11.2.23  
>grab calculations: slightly faster SIMD averaging with shuffle  
>Merge pull request #338 from zomfg/feature/widget-wizard-improvementwidget wizard improvement  
>grab config widget: QPainterPath include  
>widget wizard: zone placement: more homogenous sizes  
>Merge pull request #349 from zomfg/feature/about-qt-versionbuild time Qt version in About tab  
>Merge pull request #345 from zomfg/hotfix/qt-5.15-build-errorQt 5.15 build failing  
>Merge pull request #329 from zomfg/feature/device-config-text-updatedevice config wizard text update  
>settings window: cleaner wizard launch  
>Merge pull request #339 from zomfg/feature/winapi-grabber-set-bytesperrowwinapi grabber: set bytesPerRow  
>Merge pull request #341 from psieg/gammaleakReload the gamma ramp every 15 seconds to slow memory leak  
>Merge pull request #346 from zomfg/hotfix/qt-5.15-deprecationsQt 5.15 deprecation  
>grab calculation: fixed windows cpuidex param cast  
>grab calculations: averaging support for random rect widths  
>grab calculations: default to non-SIMD method  
>x11 grabber: XSync after XShmAttach  
>Mention fixed NightLight in changelog  
>grab calculations: avx2 averaging: fixed loadmask  
>settings window: QProcess::startDetached with separate args param  
>QTextStream: pre-5.14 compatibility  
>grab: don't trim preparedRect width to multiple of 4  
>Merge pull request #333 from zomfg/feature/x11-grabber-tweakx11 grabber tweak  
>Reload the gamma ramp every 15 seconds to slow memory leak (#277, #308)  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-04  
Contributor(s):  
zomfg  
>grabber base: fixed confised compiler warning about curly braces  
>calculations: avx detect: msvc fix  
>enums: set absolute max led limit to the biggest limit  
>calculations: sse/avx accumulate functions + some cleaning around  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-03  
Contributor(s):  
zomfg  
Patrick Siegler  
Tom Archer  
>adalight/ardulight: stop last will timer on serial write  
>adalight/ardulight: schedule and write last will frame  
>settings window: better name for baudrate warning timer  
>adalight/ardulight led devices: skip writing frame to serial when bufferstill has unconsumed data  
>Merge pull request #305 from Tom-Archer/masterProvide UDP device support  
>ardulight led device: fixed space indents in the header file  
>Allow configuration of WLED timeout  
>Merge pull request #291 from jackun/pulseaudioPulseaudio  
>Merge pull request #310 from zomfg/feature/low-baudrate-skip-writeadalight/ardulight led devices: skip writing frames to serial when buffer is full  
>Merge pull request #314 from zomfg/bugfix/linux-release-build-with-debug-flagsBugfix/linux release build with debug flags  
>Merge pull request #313 from zomfg/feature/x11-grabbedscreen-infoX11 grabber: populate GrabbedScreen .imgDataSize and .bytesPerRow  
>Bump to 5.11.2.22  
>adalight/ardulight device: writeLastWill: optional force parameter  
>src.pro: linux config had debug flags for all builds  
>adalight/ardulight devices: force writeLastWill() on close() if a will is pending  
>Fix changelog date  
>adalight/ardulight devices: lastWillTimer cleanup  
>settings window: clear low baud rate warning after 15 seconds when the frame rate is back in range  
>Merge pull request #301 from zomfg/feature/grab-interval-baudrate-warningShow "low baudrate" warning  
>X11 grabber: populate GrabbedScreen .imgDataSize and .bytesPerRow  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-02  
Contributor(s):  
jackun  
zomfg  
Patrick Siegler  
Tom Archer  
>moodlamp manager: connect to FPS display in settings window status bar  
>moodlamp manager: reset FPS count to 1 on settings changes  
>Add UDP LED device  
>Refactor LedDeviceDrgb into AbstractLedDeviceUdp  
>settigns window: remove FPS warning on mouse hover until next trigger  
>prismatik math: calculate max theoretical framerate for a given led count and baud rate  
>Update README.mdAdd Pulseaudio support dependencies  
>grab manager: user PreciseTimer for more accurate FPS count  
>First pass of DNRGB and WARLS devices  
>settings window: changed LOW BAUDRATE into symbol, added tooltip explaining the baudrate warning  
>Merge pull request #304 from zomfg/feature/more-verbose-serial-error-logmore verbose serial error log  
>moodlamp manager: fixed frame count reset on timer reset  
>Address review comments  
>Fix pulseaudio src.pro and default build-vars  
>moodlamp/sound manager: fixed missing connect() for sendDataOnlyIfColorsChangesChanged  
>moodlamp/soundviz: slow down FPS count to 1 update per second  
>prismatik math: theoretical min baud rate  
>Fix device name  
>led device ardulight: more serial error info  
>settings window: fixed QColorConstants build error on 5.13  
>settings window: highlight FPS in red when FPS over max theoretical framerate  
>sound manager: connect updates to FPS counter in status bar  
>led device adalight: more serial error info  
>settings window: don't show grab interval based max fps for non-grabber modes  
>led device manager: wrong led device WBA update slot  
>Rename UdpDevice to DrgbDevice and add settings/configuration for IP/port  
>moodlamp: use QElpsedTimer for frame time  
>Merge pull request #302 from zomfg/hotfix/wrong-led-device-qtslotled device manager: wrong led device WBA update slot  
>Configure WARLS and DNRGB using UDP configuration page  
>Set DNRGB max to 1023 for time being  
>Update DNRGB max LEDs to 1500  
>led device adalight: removed unsupported serial open() mode  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2020-01  
Contributor(s):  
zomfg  
Patrick Siegler  
>Automatically stop grabbing if device throws errors (#248)  
>Merge pull request #290 from psieg/autostopAutomatically stop grabbing if device throws errors (#248)  
>settings window: replaced qSort() with std::sort(), replaced QListWidget::setItemHidden() with QListWidgetItem::setHidden()  
>Mark admin autostart as not recommended in setup  
>Fix misconnected white balance adjustment signal.Affects WBA changes from Widgets from Settings Window.  
>grab config widget: Qt pre-5.10 version check for compatibility  
>grab widget: changed QColor::dark() to darker()  
>api server and plugin interface: replaced QTime with QElapsedTimer  
>Add support for rotated screen captures (#253)  
>Update to OpenSSL 1.1  
>Merge pull request #288 from zomfg/feature/qt_5_14_fixesQt 5.14  
>Bump to 5.11.2.19  
>replaced QDesktopWidget::screen... methods with QGuiApplication::screen...  
>Remove auto-updater to prevet AV false positive  
>Bump to 5.11.2.21  
>Merge pull request #289 from psieg/rotateAdd support for rotated screen captures (#253)  
>grab manager and zone placement page: default QGuiApplication::screens().value() to primaryScreen  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-12  
Contributor(s):  
jackun  
>Pulseaudio support  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-10  
Contributor(s):  
Justin Kromlinger  
Patrick Siegler  
>Merge pull request #279 from hashworks/patch-1Add AUR badges  
>Add AUR badges  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-03  
Contributor(s):  
Patrick Siegler  
>Add a log message for non-enumerated processes and expand whitelist  
>Update DX grabber skip-list  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-02  
Contributor(s):  
Patrick Siegler  
>Merge pull request #242 from zomfg/feature/moodlamp-typesmood lamps  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2019-01  
Contributor(s):  
zomfg  
Patrick Siegler  
>Merge pull request #231 from zomfg/feature/high-contrast-virtual-led-labelshigh contrast virtual led labels  
>Scale grabbing rect to the inside to avoid exceeding buffer bounds  
>Add default setting for SoundVisualizerVisualizer  
>Merge pull request #224 from zomfg/feature/win-ddupl-downscalingwindows ddupl grabber downscaling  
>settings window: append BASS credits only when BASS_SOUND_SUPPORT is defined  
>soundviz types: macos  
>fix signal wiring for updateWBAdjustments  
>settings window: removed now irrelevant comment  
>Merge pull request #238 from zomfg/feature/soundviz-typessoundviz types  
>math: getBrightness from qRgb  
>grab manager: indent fix  
>Merge pull request #228 from zomfg/feature/macos-cg-grabber-fixmacos cg grabber fix  
>settings window: added a tooltip explaining macOS limitations  
>settings window: check virtual led brightness to change label color  
>grab calculations: use size_t for pitch and index  
>grabber base: count only actually grabbed frames  
>liquid color generator: replaced deprecated qrand with QRandomGenerator  
>Merge pull request #234 from zomfg/feature/macos-soundvizmacos soundviz  
>Add styles plugin to installer  
>Open SettingsWindow if Prismatik is started again  
>Do not retry autoupdate after failures. Show a MessageBox and disable autoupdates.  
>Bump to 5.11.2.18  
>soundviz types: windows (select default crashes)  
>SoundManager: renamed to SoundManagerBase  
>settings window: soundviz: hide lightpack smoothness note for non-Lightpack devices  
>lightpack application: fixed missing soundviz #ifdefs  
>macos av grabber  
>Bump to 5.11.2.19  
>Merge pull request #232 from zomfg/feature/macos-av-grabbermacos av grabber  
>macos soundviz- refactored original sound manager + macos version- renamed BASS_SOUND_SUPPORT to SOUNDVIZ_SUPPORT is source files- switched liquid generator and windows sound manager to precise timer- added sound access requirement to Info.plist  
>mood lamps  
>macos SOUNDVIS_SUPPORT: moved from src.pro to default build-vars  
>Merge pull request #226 from zomfg/feature/macos-blue-light-reductionmacos blue light reduction  
>grabber base: round up bottom-right corner when scaling preparedRect  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-12  
Contributor(s):  
zomfg  
Sergio  
Patrick Siegler  
>grabber base: scale windget rect if needed and override bytesPerRow whenset  
>Prepare to copy openssl runtime too (for differing runtime versions)  
>Merge pull request #217 from zomfg/feature/macos-xcodemacOS Xcode script  
>grab manager: null check on delete of blue light client  
>Add a setting for lights on after display off  
>Merge pull request #225 from zomfg/feature/macos-session-change-detectormacos session change detector  
>readme: fixed macos/linux build steps mixup  
>Merge pull request #221 from zomfg/feature/blue-light-refactoringblue light reduction refactoring  
>grab manager: log a warning when blue light client creation fails  
>windows: re-align event filter with macos version  
>macos: session event filter/listener  
>grabbed screen: added scale and bytesPerRow override values  
>main.cpp modernized OS version check  
>grab manager: leave nightlight color temp logic to itself  
>updated README to add xcode script  
>grab interval (#212)* grabber base: use 1ms precision timer* settings window: display max FPS for a given grab interval  
>qmake config for night light lib inclusion  
>Merge pull request #229 from zomfg/hotfix/readme-macos-linux-mixupreadme: fixed macos/linux build steps mixup  
>use night light wrapper class to avoid extra dependencies  
>grab calculations: replaced 4 accumulate... functions with 1 templated,added some consts and macros for clarity  
>renamed "apply gamma ramp" to "apply blue light reduction"  
>Merge pull request #216 from psieg/pr215_cleangrab calculations: replaced 4 accumulate... functions with 1 templated,  
>grab manager using NIGHTLIGHT_SUPPOR defines  
>macos gamma ramp class  
>blue light reduction refactoring- factory pattern for night light / gamma ramp application- moved windows gamma ramp function to a class- added a wrapper class for windows night light- rearranged night light includes in .pro files (src / grab)  
>Fix vs build  
>macos nightshift support  
>Merge pull request #227 from zomfg/feature/macos-deprecationsmacos deprecations  
>Merge pull request #219 from psieg/pr214_cleannight light support (clean)  
>grabber base: include for std::floor  
>color temperature settings (#208)* Color temperature + gamma (encoding) settings to emulate the effects of f.lux, redshift, Night Light...* global color coef page: color temperature helper slider (3-in-1 RGB control)* prismatik math: cstdint include* prismatik math: replaced cstdint with climits* device wizard: wider window* global color coef page: text explainig color temperature slider  
>grab calculations: removed "register" as it's reserved for a potentially different future useand does nothing in a current statehttp://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0001r1.html  
>Switch back the bluelight settings keyIdeally, migration code would read the old value and write the new value. Until then, stick to the old name in the ini file.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-11  
Contributor(s):  
zomfg  
TJ-91  
Patrick Siegler  
>night light library implementation  
>Update README with new script  
>Merge pull request #201 from zomfg/feature/display-sleep-supportdisplay sleep support (windows)  
>more updates to developer docs  
>edit src.pro and SoundManager.cpp in a way that it compiles without bass support  
>remove config ordered to fix mac build  
>Fix qmake dependencies  
>configure device page: prefill with previous baud rate / serial port / color sequence  
>Merge pull request #202 from zomfg/feature/small-fixessmall fixes to device configuration page  
>fix build system and revert provisional changes  
>Add dependencies to main project to fix mac build  
>Switch project order in solution, make the main binary the first one  
>display sleep support  
>Merge pull request #198 from mickdekkers/patch-1Fix typo in README.md  
>Fix build files for VS2017  
>also revert another overlooked change  
>Merge pull request #205 from psieg/vs2017Vs2017  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-10  
Contributor(s):  
Mick Dekkers  
>Fix typo in README.md  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-04  
Contributor(s):  
Patrick Siegler  
>Do not create a log file if logging is disabled  
>Fix thread access errors  
>Add untested baud rates to wizard  
>Bump to 5.11.2.17  
>Recreate DDupl adapters when an adapter's output has no handleThis happens under unclear circumstances when switching two screes beween extended and duplicated mode.Contrary to the error description, it goes away only once the user switched back.  
>Avoid spamming frames when updating device settings  
>Fix invalid signal  
>Bump to 5.11.2.16  
>Detect failed update, merge trayicon sources  
>Avoid double-processing the same set of colors  
>More realistic FPS indicator  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2018-03  
Contributor(s):  
psieg  
Patrick Siegler  
>Update Process Tweaks  
>VS dependencies, custom temp path for autoupdate files  
>Add API command to persist colors on unlock  
>Persist API colors upon unlocking. Needs corresponding API command  
>Bump to 5.11.2.15  
>Load updates only if all files are defined  
>Send the correct number of colors during wizard (#124)  
>Fix updater: Allow following redirects  
>Merge pull request #163 from psieg/autoupdate2Automatic selfupdater  
>keep trying every 5 seconds  
>Update API doc for new commands  
>Try to restore original grab area positions when screen layout changes (#84, #140, #155)  
>Automatically install updates if enabled  
>Merge pull request #166 from psieg/wizardcountSend the correct number of colors during wizard (#124)  
>format COMMANDS.h  
>Bump to 5.11.2.14, API 2.2  
>Fix dates in changelog  
>Merge pull request #164 from psieg/persistPersist API colors when unlocking  
>Try to reconnect the device after growing time intervals after an IO error  
>Fix updater error messages  
>Merge pull request #168 from psieg/devicelostcrashFix a crash when the Lightpack device connection is lost  
>formatting and PR cleanup  
>Formatting: Use tabs everywhere  
>Merge pull request #170 from psieg/recreateTry to reconnect the device after IO errors  
>Allow more than three digit LED numbers in API (#152)  
>Fix issue number in CHANGELOG  
>Fix a crash when the Lightpack device connection is lost  
>Merge pull request #167 from psieg/resizing Try to restore original grab area positions when screen layout changes  
>Merge pull request #165 from psieg/ledlimitAllow more than three digit LED numbers in API (#152)  
>Show a message when an update has completed  
>Make sure the updater runs on Windows only  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-12  
Contributor(s):  
asgardc3r  
Patrick Siegler  
>Update Settings.cpp  
>Update LedDeviceAdalight.hpp  
>Merge pull request #154 from asgardc3r/mastermore than 255 led strip  
>Update enums.hpp  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-11  
Contributor(s):  
psieg  
>Automatically download update files  
>Fix VS2015 and up build  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-08  
Contributor(s):  
psieg  
>Preparations for automatic selfupdate  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-07  
Contributor(s):  
psieg  
>Always use primary monitor DC for Gamma Ramp  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-06  
Contributor(s):  
Patrick Siegler  
>Merge pull request #121 from psieg/soundapiSound API  
>Bump to 5.11.2.13, API 2.1  
>Clean up API help  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-05  
Contributor(s):  
psieg  
Patrick Siegler  
>Properly os-guard gamma call  
>Add deprecated result  
>Flux support by reading primary GDI DC's gamma ramp  
>Add SoundViz to API mode commands  
>Add soundviz commands to API  
>Fix incorrect warning message  
>Integrate gamma ramp setting  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-04  
Contributor(s):  
David Madison  
Patrick Siegler  
>Merge pull request #101 from psieg/dreamer-dead-psieg-upstream-avoid-memmove-for-image-dataavoid memmove for image data  
>Fixed markdown  
>Merge pull request #107 from dmadison/markdown-fixMarkdown Fix  
>Fixed grammar  
>Update build instructions  
>Fix changelog issue reference  
>Version bump 5.11.2.12  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-03  
Contributor(s):  
psieg  
Patrick Siegler  
>Move to Qt5.7 on mac  
>Make imgData const (write access in grabbers)  
>Remove appmenu trayicon workaround  
>Merge pull request #100 from psieg/trayOnQtRemove appmenu trayicon workaround  
>Merge pull request #95 from psieg/colorcoefColorcoef  
>Use associatedData rather than compiler macros  
>Clear device settings when using device in wizard  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-02  
Contributor(s):  
psieg  
Patrick Siegler  
>Grabbing debug cleanup  
>Remove dead ZoneWidget class  
>Fix warning syntax  
>Add global white balance to wizard  
>Clean up debug levels  
>Enable setMode API and disable dead setDevice, setLedCounts API  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2017-01  
Contributor(s):  
Patrick Siegler  
>Fix SoundViz using incorrect device after profile switch  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-11  
Contributor(s):  
Patrick Siegler  
>Version bump 6.0.0.104  
>Fix lights stay off after resume in certain cases  
>Official version checks for updates from official URL  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-10  
Contributor(s):  
Patrick Siegler  
>Version bump 5.11.2.11  
>Adjust to updated Travis OS X setup  
>Remove uk_UA translation  
>Include bass in 32 bit setup script  
>Rewrite device settings after resume  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-09  
Contributor(s):  
psieg  
Patrick Siegler  
>Merge pull request #74 from dreamer-dead/psieg-upstream-command-line-parserImplement LightpackCommandLineParser class.  
>Another go at trayicon workaround  
>Fix custom offset/rotation not saved  
>Merge pull request #39 from psieg/trayRemove special handling of unity tray icon  
>Fix installer translation  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-08  
Contributor(s):  
dreamer.dead  
Patrick Siegler  
>Merge pull request #71 from dreamer-dead/psieg-upstream-enable-c++11Enable C++11 mode for GCC and Clang.  
>Tweak soft smooth  
>Rebrand to official  
>Get rid of unnecessary memmove() for MacOS.  
>Add BOM to setup scripts to fix translations  
>Fix compilation on platforms other than Mac.  
>Russian translation updates  
>Version bump 6.0.0.103 (testing, rebased)  
>Recommend DDupl to non-expert users  
>Remove duplicate/unused Settings::profileLoaded signal  
>Enable C++11 mode for GCC and Clang.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-07  
Contributor(s):  
Patrick Siegler  
>Fix applying translation changes settings  
>Fix SoundVizSpeed read from incorrect setting  
>Fix SoudVizDevice setting getting lost  
>Fix lights turning on themselves after lock/sleep  
>Add software based smoothing for non-Lightpack devices  
>Apply white balance after luminosity and brightness correction  
>Version bump 5.11.2.10  
>Add offset and rotation to custom layouts  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-06  
Contributor(s):  
psieg  
Patrick Siegler  
>Document openssl dependency  
>Use CustomDistributor to build known layouts- Distributor uses coordinates rather than factors to avoid precision issues- Distributor considers leftover pixels- Custom stand with and thickness- size_t to int  
>Update UI and documentation  
>Merge pull request #41 from psieg/updateUpdate Checker  
>Merge pull request #48 from psieg/sound_deviceAllow selection of sound device  
>Improve wizard launch / device access issues  
>Trayicon workaround 2  
>Add over-brightening to grabbing  
>travis OS X build setup  
>Version bump 5.11.2.9  
>Merge pull request #27 from Archomeda/additional-firmware-commandsAbility to toggle the power LED state  
>Fix checkForUpdate connection warning  
>Merge pull request #51 from psieg/overbrightenAdd over-brightening to grabbing  
>Merge pull request #45 from psieg/audio_visAudio Visualizer  
>Fix linux build  
>TrayIcon workaround  
>Snap GrabWidget to screen when resizing  
>Reduce spread of current version number  
>Version bump 5.11.2.8  
>Merge pull request #43 from psieg/grab_widgetsGrab widgets  
>OS X build fixup  
>Add CustomDistributor to wizard  
>Extract LiquidColorGenerator  
>Sound Visualizer Cleanup  
>Merge pull request #47 from psieg/osxOS X build adaptations  
>Merge pull request #49 from psieg/fullframeAdd CustomDistributor to wizard  
>Remove Speed Test leftovers in code  
>Allow selection of sound device  
>Include openssl in packaging  
>Add credit for BASS  
>Add sound visualizer using bass library  
>Build fixes  
>Fix and use update checker  
>Snap GrabWidgets to each other  
>Fix wizard detects no devices when launched by Prismatik  
>Add liquid sound visualizer  
>Merge GrabWidget and GrabAreaWidget  
>Remove unused updateSmoothSlowdown  
>Merge pull request #50 from psieg/audio_liquidLiquid Audio  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-05  
Contributor(s):  
psieg  
>Version bump 5.11.2.7  
>Remove special handling of unity tray icon  
>Cleanup  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-04  
Contributor(s):  
psieg  
>Linux build fix  
>Remove dead and obsolete grabbers  
>Minor UI and translation fixes  
>Keep API compatible to Lightpack v1.4  
>Properly detect and handle standby  
>Improve DDuplGrabber flow when on secure desktop  
>Unify tabs to spaces  
>Layout improvements for high dpi scenarios  
>Add option to keep lights on after suspend, default after exit to false  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-03  
Contributor(s):  
psieg  
>Update windows version log statements  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2016-02  
Contributor(s):  
Archomeda  
psieg  
Patrick Siegler  
Alex Morbo  
>Version bump 5.11.2.5  
>Change CMD_UNOFFICIAL_SET_USBLED command to be in unofficial cmd range  
>Merge pull request #23 from alexmorbo/masterUpdate README.md  
>Merge pull request #24 from psieg/linux_buildUpdate binary path in linux script, include locale generation in README  
>Add support for toggling the power LED state through the settingsThis also updates the firmware with additional support for unofficialcommands (so we can easily distinguish the official and unofficialfirmware and features in Prismatik and make them both compatible).The new firmware version is (4-7).6+1. Official Prismatik releases stillrecognize the firmware as (4-7).6.  
>Move D3D10Grabber to dedicated thread  
>Version bump 5.11.2.6  
>Version bump 5.11.2.4  
>Merge pull request #16 from psieg/newapiImplement missing API get methods  
>Merge pull request #19 from psieg/fix/dxscan_threadMove D3D10Grabber to dedicated thread  
>Revert "Move D3D10Grabber to dedicated thread"  
>Update binary path in linux script, include locale generation in README  
>Update DDuplGrabber to be able to grab the screensaver desktop  
>Update README and locale script  
>Update readme for building firmware and add a batch build script  
>Update README.md  
>Move DX scanning/injecting to dedicated thread  
>Fix formatting  
>Implement missing API get methods  
>Merge pull request #21 from psieg/dx2DX scanning  
>Fix crash when opening configuration Wizard  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-09  
Contributor(s):  
psieg  
>Fix admin-required-message creating unwanted message loop  
>Fix fake grab not being stopped  
>Version bump 5.11.2.3  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-08  
Contributor(s):  
psieg  
>Merge pull request #12 from psieg/fix/no_grab_updateNo grab update  
>Update README links for fork  
>De-duplicate grabber reinit  
>Fix 0-fps-detection to not false-fire if delay is >500ms  
>Make DDupl the new default grabber  
>Update colors event if no new colors were grabbedto prevent devices from switching off because they get no udpates  
>If no grab updates happen, actually show 0 FPS  
>Live update the sendData flag in the manager  
>Consolidate readmes  
>Clean up GrabManager  
>Version bump 5.11.2.2  
>set fake-grab interval to 900assuming devices have a 1 s switchoff timer  
>Fix live update of DX grabbing  
>Merge pull request #8 from Archomeda/windeployqtImprove Windows builds  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-07  
Contributor(s):  
Archomeda  
psieg  
>Fix crash on profile deletion  
>Change file encodings from KOI8-R to UTF-8 on various files  
>Remove plugins  
>Fix libraryinjector.dll not being copied  
>Update binary files in setup script  
>Merge pull request #6 from psieg/fix/profile_crashFix crash on profile deletion  
>Generate translations in VS post build stage  
>Mark fork as such  
>Place all binary files in bin, and *.lib and *.exp in lib automatically  
>Fix linux build  
>Fix tab  
>Integrate translations better  
>Integrate windeployqt in VS post build stage  
>Fix some more tabs  
>Update Qt to 5.5  
>Revert redist package and include msvcr*.dll and msvcp*.dll directly  
>Merge remote-tracking branch 'woodenshark/master'Conflicts:	README.md  
>Merge pull request #9 from Archomeda/fix-file-encodingFix file encodings on various files  
>Fix tabs  
>Add installer option to schedule admin launch task  
>Change binary output directory to bin instead of src/bin  
>Use --no-compiler-runtime instead of manually removing the redist file  
>Create different installers for 32/64bit  
>Update installer scripts  
>Fix ARP uninstall name  
>Re-add platforms plugin  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-02  
Contributor(s):  
psieg  
>Don't change grabber state if grabbing is stopped  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2015-01  
Contributor(s):  
psieg  
>Also hook D3DDevice9::Reset to avoid stallingIn certain scenarios (alt+tab) the device is lost, this was caught in theold version tooScenarios like voluntary reset (e.g. change of settings) were not covered  
>Multiple important fixes* Add option to skip d3d9 because of performance impact* Fix GAPIProxyGrabber destructor trying to call overriden virtual free()* Fix grabberType changed too often during SettingsWindow setup* Properly cleanup and create D3D10Grabber because stop() does not stop it from injecting  
>More build warnings  
>getNumberOfLeds returns int, fix warnings  
>Fix shared memory corruption  
>Sanitize (unhook) processes when exiting  
>Update file description  
>Make README links repository independent  
>Safe x64 size_t / int / LONG casts  
>Only inject processes with fullscreen windows  
>Make DX grabbing freq configurable  
>Properly cleanup after injection  
>Hooks: If grabbing is stopped, do not actually grab the frame  
>Fix libraryinjector eventlog reporting  
>Fix some compiler warnings in libraryinjector  
>Multiple important fixes* Add option to skip d3d9 because of performance impact* Fix GAPIProxyGrabber destructor trying to call overriden virtual free()* Fix grabberType changed too often during SettingsWindow setup* Properly cleanup and create D3D10Grabber because stop() does not stop it from injectingConflicts:	Software/src/SettingsWindow.ui  
>Add MSBuild instructions  
>ProxyFuncJmp stability: remove NOP, allow execution while editing  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2014-12  
Contributor(s):  
psieg  
>Add thunderbird to DX ignore list too  
>Add offsetfinder helper stub on Win64  
>typos and tabs to spaces  
>If built x64, inject x64, fix DX version detection  
>Add debug message when DX infects a process  
>Clarify fork purpose  
>allow x64 build when building for win32  
>Set msvc build to build in parallel  
>Add devenv.exe to D3D10 exclude list  
>Hooks: post-build: copy dll to src/bin  
>Add Admin-Check to DX hooking  
>Fix path of injected dll  
>Add empty Desktop Duplication GrabberConflicts:	Software/grab/grab.vcxproj	Software/src/Prismatik.vcxproj  
>Simplify DLL injection  
>If compiled x64, inject 64 and 32 bit processes  
>Wait for D3D processes some time before injecting  
>Don't require subdir for hooks32  
>Fix windows build (msvc)  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 


2014-08  
Contributor(s):  
dreamer-dead  
>Implement LightpackCommandLineParser class.	- Use QCommandLineParser class to parse the command line.	- Add alternate way to set debug level (--debug=level).	- Handle --help and --version command switches.	- Add unit-tests for LightpackCommandLineParser class.  
- - - - - - - - - - - - - - - - - - - - - - - - - - - 

