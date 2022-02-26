#!/bin/sh
kde_version=5.15
flatpak_id=de.psieg.Prismatik
destdir=tmp
VERSION=`cat ../../VERSION`

set -xe

sed '
	s#__APP_ID__#'"$flatpak_id"'#
	s#__KDE_VERSION__#'"$kde_version"'#
	' manifest.yml.template > "$flatpak_id.yml"

[ ! -e "$flatpak_id.yml" ] && echo "manifest $flatpak_id.yml not found" && exit 1

rm -rf "$destdir/flatdir" "$destdir/repo"
# flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
echo "######################## install"
ps -aux
flatpak install --user --assumeyes org.kde.Sdk//"$kde_version" org.kde.Platform//"$kde_version"
echo "######################## build"
ps -aux
flatpak-builder --jobs=1 --delete-build-dirs --repo="$destdir/repo" "$destdir/flatdir" "$flatpak_id.yml"
# flatpak build-export "$destdir/repo" "$destdir/flatdir"
echo "######################## bundle"
ps -ux
flatpak build-bundle "$destdir/repo" "prismatik_$VERSION.flatpak" "$flatpak_id"
# flatpak run de.psieg.Prismatik
echo "######################## cleanup"
ps -aux
rm -rf "$destdir"
rm -rf .flatpak-builder
rm -rf "$flatpak_id.yml"
