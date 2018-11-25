#!/bin/bash
# call this from $repo/Software
echo "================================================="
echo "If you're using VS2017, run generate_sln.bat from"
echo "the developer command prompt instead."
echo "================================================="
echo ""


$QTDIR/bin/qmake -recursive -tp vc Lightpack.pro

# qmake only allows shared linkage of the msvcr* runtime (for reasons acceptable when using Qt in the projects)
# for our lightweiht, non-qt components which we want to inject into other processes this is bad
# so after qmake is done, run this little script to rewrite the vcxproj's to use static linking

function fix_static {
	for f in $1
	do
		if [ -f $f -a -r $f ]; then
			echo "rewriting to static linkage: $f"
			sed "s/MultiThreadedDLL/MultiThreaded/g" "$f" | sed "s/MultiThreadedDebugDLL/MultiThreadedDebug/g" > "$f.tmp" && mv "$f.tmp" "$f"
		fi
	done
}

fix_static "hooks/*.vcxproj"
fix_static "unhook/*.vcxproj"
fix_static "offsetfinder/*.vcxproj"

# qmake can't create Win32 projects in a x64 SUBDIRS sln. The files are Win32, but the SLN generator assumes all vcxproj's are x64 sln
# so we have to manually rewrite the SLN to reference the Win32 builds
PATHS=("unhook/prismatik-unhook32.vcxproj" "hooks/prismatik-hooks32.vcxproj" "offsetfinder/offsetfinder.vcxproj")
	SLN="$(cat Lightpack.sln)"
	for f in ${PATHS[*]}
	do
		if [ -e $f ]; then
			GUID=$(sed -n 's/.*\({[0-9A-Z\-]*}\).*/\1/p' "$f")
			echo "rewriting to Win32: $GUID ($f)"
			SLN=$(echo "$SLN" | sed "s/\($GUID.*\)|x64/\1|Win32/g")
		fi
	done
	echo "$SLN" > Lightpack.sln