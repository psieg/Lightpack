# -------------------------------------------------
# Lightpack.pro
#
# Created on: 28.04.2010
#
# Lightpack is very simple implementation of the backlight for a laptop
# 
# Copyright (c) 2010, 2011 Mike Shatohin, mikeshatohin [at] gmail.com
#
# http://lightpack.googlecode.com
#
# Lightpack based on:
# LUFA: http://www.fourwalledcubicle.com/LUFA.php
# hidapi: https://github.com/signal11/hidapi
#
# Lightpack is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# Lightpack is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#  
# Project created by QtCreator 2010-04-28T19:08:13
# -------------------------------------------------

TEMPLATE = subdirs

include(build-config.prf)

SUBDIRS = src
SUBDIRS += math grab
src.depends = math grab

win32:SUBDIRS += libraryinjector hooks unhook tests
contains(QMAKE_TARGET.arch, x86_64) {
    SUBDIRS += offsetfinder hooks32 unhook32
    hooks32.file = hooks/hooks32.pro
    unhook32.file = unhook/unhook32.pro
}
