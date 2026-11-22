#!/bin/sh
set -xe

# TODO: better way of dynamically adding build-vars deps?
PULSEAUDIO_SUPPORT=
cd ../..
qmake6 -r
if [ "$(grep -Fo PULSEAUDIO_SUPPORT src/Makefile)" == "PULSEAUDIO_SUPPORT" ]
then
    echo PULSEAUDIO_SUPPORT
    PULSEAUDIO_SUPPORT="'libpulse' 'fftw'"
fi
cd -

sed 's#__PULSEAUDIO_SUPPORT__#'"$PULSEAUDIO_SUPPORT"'#' PKGBUILD.template > PKGBUILD
makepkg -cf
namcap prismatik*.pkg.tar*
rm PKGBUILD
