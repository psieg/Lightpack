@echo off
setlocal

if defined VSCMD_VER goto :build

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do set "VS_INSTALL_DIR=%%i"
if not defined VS_INSTALL_DIR (
    echo ERROR: Visual Studio installation with MSBuild not found.
    exit /b 1
)

call "%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64
if errorlevel 1 (
    echo ERROR: VsDevCmd.bat failed
    exit /b 1
)

:build
echo.
echo === Environment ready, starting MSBuild ===
echo.
msbuild "%~dp0Lightpack.sln" /p:Configuration=Release /p:Platform=x64 /m /nologo /v:m
exit /b %errorlevel%
