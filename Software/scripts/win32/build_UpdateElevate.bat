@echo off
setlocal
if defined VSCMD_VER goto :build
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if defined VS_INSTALL_DIR call "%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64
:build
MSBuild.exe UpdateElevate\UpdateElevate.sln /p:Configuration=Release
