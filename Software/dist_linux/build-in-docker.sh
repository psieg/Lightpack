#!/bin/sh

dirs=
for dfile in `ls */Dockerfile`
do
	ddir=`dirname $dfile`
	dirs="${dirs}|${ddir}"
done
dirs=`echo "$dirs" | cut -c 2-`

usage="\n	Usage $0 ($dirs) os-image-name os-image-tag\n"

[ -z $1 ] && echo -e "$usage" && exit 1

pkgmgr=dpkg
[ ! -z $1 ] && pkgmgr=$1

os=ubuntu
[ ! -z $2 ] && os=$2

version=24.04
[ ! -z $3 ] && version=$3



dockerfile=./$pkgmgr/Dockerfile

[ ! -e  "$dockerfile" ] && echo -e "Error: $dockerfile not found\n$usage" && exit 1

echo $pkgmgr $os $version

set -xe
docker build \
	--build-arg OS=$os \
	--build-arg RELEASE=$version \
	-f "$dockerfile" \
	-t "prismatik/$os:$version" "./$pkgmgr"

indockerbuild=/tmp/build.sh

cat << EOF > "$indockerbuild"
#!/bin/sh
set -xe

cd /Lightpack/Software/dist_linux
./build-natively.sh $pkgmgr
EOF
chmod +x "$indockerbuild"

docker run \
	-v "$(pwd)/../..:/Lightpack" \
	-v "$indockerbuild:/build.sh" \
	-v "/etc/localtime:/etc/localtime:ro" \
	--user="$(id -u):$(id -g)" \
	--rm \
	--name "prismatik_builder_$pkgmgr" "prismatik/$os:$version"
