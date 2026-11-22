#!/bin/sh -xe

cd ../..
qmake6 -r
make -j$(nproc)
cd -

VERSION=`cat ../../RELEASE_VERSION`

arch=amd64
[ ! -z $1 ] && arch=$1

debdir=deb
rm -rf "$debdir"

# copy generic template /etc /usr ...
template_path=../package_template
for file in `find "$template_path" -type f`
do
	install -Dm644 "$file" `echo "$file" | sed 's#'"$template_path"'/#'"$debdir"'/#'`
done
# copy the build
install -Dm755 -s ../../bin/Prismatik "$debdir/usr/bin/prismatik"

timestamp=`date -u`
size=`du -s deb | cut -f 1`

# copy deb specific template files
for file in `find DEBIAN_template -type f`
do
	dest="$debdir/DEBIAN/"`basename "$file"`

	install -Dm644 "$file" "$dest"

	# replace templated values
	sed -i'' '
		s#${version}#'"$VERSION"'#g
		s#${timestamp}#'"$timestamp"'#g
		s#${arch}#'"$arch"'#g
		s#${size}#'"$size"'#g
	' "$dest"
done
# restore exec permissions (stripped by the loop above)
chmod +x "$debdir/DEBIAN/postinst"

# update MD5 sums
hashdeep -r "$debdir/usr" > "$debdir/DEBIAN/md5sums"
chmod 0644 "$debdir/DEBIAN/md5sums"

# build package in format "name_version_arch.deb"
fakeroot dpkg-deb --build "$debdir/" .

# delete working dir on success
# rm -rf "$debdir"
