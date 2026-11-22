#!/bin/sh

buildscript=build.sh

dirs=
for dfile in `ls */$buildscript`
do
	ddir=`dirname $dfile`
	dirs="${dirs}|${ddir}"
done
dirs=`echo "$dirs" | cut -c 2-`

usage="\n	Usage $0 ($dirs)\n"

[ -z $1 ] && echo -e "$usage" && exit 1

pkgmgr=$1
[ ! -e  "$pkgmgr/$buildscript" ] && echo -e "Error: $pkgmgr/$buildscript not found\n$usage" && exit 1

echo $pkgmgr

(which lsb_release 2> /dev/null) && lsb_release -a
(which qmake6 2> /dev/null) && qmake6 -v
(which qmake 2> /dev/null) && qmake -v
(which g++ 2> /dev/null) && g++ -v
(which clang++ 2> /dev/null) && clang++ -v
(which lscpu 2> /dev/null) && lscpu

set -xe
cd "$pkgmgr"
./"$buildscript"
