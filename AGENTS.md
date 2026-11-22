# Build Setup

## Dependencies

| Dependency | Windows | Other Platforms (Linux / macOS) |
|---|---|---|
| **C++17 Toolchain** | Visual Studio 2022+ ("Desktop development with C++") | System C++ compiler (`g++` / `clang++`), `make`, `pkg-config` (Linux), platform SDK |
| **Qt SDK (Qt 6.x)** | MSVC 64-bit kit (with `SerialPort`) | Qt base, tools, and `SerialPort` dev packages / frameworks |
| **TLS / Crypto** *(for update check)* | Built-in Windows Schannel & CNG (no OpenSSL required) | System OpenSSL package / library |
| **Sound Visualizer** *(optional)* | BASS + BASSWASAPI (`<BASS_DIR>`, `<BASSWASAPI_DIR>`) | Platform audio/FFT libs (e.g. PulseAudio + FFTW3 on Linux; built-in on macOS) |
| **Platform Libraries** | Included in Windows SDK | USB / device headers as required by OS (e.g. `libusb`, `libudev` on Linux) |

## Environment

- **Qt location**: Ensure `qmake` is reachable — on Windows, set `QTDIR` to your Qt kit directory (e.g. `<QT_ROOT>\<version>\msvc2022_64`); on other platforms, ensure Qt tools are on `PATH` (or configured via `qtchooser` / SDK env).
- **Compiler environment**: Initialize your toolchain environment if required by the OS (on Windows, run from a Visual Studio Developer Command Prompt / `VsDevCmd.bat -arch=amd64`).

## Build Steps

All commands run from `Software/`:

**1. Configure `build-vars.prf`**

Copy `build-vars.prf.default` to `build-vars.prf` and adjust the section for your platform (`win32`, `unix:!macx`, or `macx`) with your local dependency paths and optional feature `DEFINES`:

```ini
# Example (win32 block):
BASS_DIR = "<path-to-bass>"
BASSWASAPI_DIR = "<path-to-basswasapi>"
TARGET_ARCH = x86_64
```

*(Comment out optional `DEFINES` such as `BASS_SOUND_SUPPORT`, `NIGHTLIGHT_SUPPORT`, or `PULSEAUDIO_SUPPORT` if skipping those dependencies.)*

**2. Generate project / Makefiles & Build**

- **Windows**:
  1. Run `scripts\win32\generate_sln.bat` from a VS Developer Prompt (runs `qmake -recursive -tp vc`, sets static runtime on hook projects, and patches `Win32` targets in `Lightpack.sln`).
  2. Build `Lightpack.sln` (`Release|x64`) with MSBuild / Visual Studio (or run `build_release.bat`).
- **Linux / macOS** *(unverified here — see `README.md` and `Software/scripts/<platform>/`)*:
  1. Run `qmake -r` (or `./scripts/macos/generate_xcode_project.sh` for Xcode).
  2. Build with `make` (or the generated IDE project).

Build output lands in `Software/bin/`.

## Notes

- A `git not found` message during `qmake` is harmless — `GIT_REVISION` simply stays undefined.
- On Windows with Qt 6, `windeployqt` deploys the native `qschannelbackend.dll` TLS plugin by default (no OpenSSL runtime DLLs required), and `UpdateElevate` verifies update signatures via native Windows CNG (`bcrypt.dll` / `crypt32.dll`).
