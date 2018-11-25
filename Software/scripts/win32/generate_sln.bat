REM call this from $repo/Software
%QTDIR%\bin\qmake -recursive -tp vc Lightpack.pro

REM qmake only allows shared linkage of the msvcr* runtime (for reasons acceptable when using Qt in the projects)
REM for our lightweiht, non-qt components which we want to inject into other processes this is bad
REM so after qmake is done, run this little script to rewrite the vcxproj's to use static linking
powershell "ForEach($f in Get-ChildItem hooks\*.vcxproj) { (Get-Content $f).replace(\"MultiThreadedDLL\",\"MultiThreaded\") | Set-Content $f }"
powershell "ForEach($f in Get-ChildItem unhook\*.vcxproj) { (Get-Content $f).replace(\"MultiThreadedDLL\",\"MultiThreaded\") | Set-Content $f }"
powershell "ForEach($f in Get-ChildItem offsetfinder\*.vcxproj) { (Get-Content $f).replace(\"MultiThreadedDLL\",\"MultiThreaded\") | Set-Content $f }"

REM qmake can't create Win32 projects in a x64 SUBDIRS sln. The files are Win32, but the SLN generator assumes all vcxproj's are x64 sln
REM so we have to manually rewrite the SLN to reference the Win32 builds
powershell "$f = \"Lightpack.sln\"; $sln = Get-Content $f; $guid = ($sln | select-string -Pattern 'prismatik-hooks32.*({[0-9A-Z\-]*}).*').Matches[0].Groups[1].Value; $sln = $sln -replace(\"($guid.*)x64\",'$1Win32'); $guid = ($sln | select-string -Pattern 'prismatik-unhook32.*({[0-9A-Z\-]*}).*').Matches[0].Groups[1].Value; $sln = $sln -replace(\"($guid.*)x64\",'$1Win32'); $guid = ($sln | select-string -Pattern 'offsetfinder.*({[0-9A-Z\-]*}).*').Matches[0].Groups[1].Value; $sln = $sln -replace(\"($guid.*)x64\",'$1Win32'); $sln | Set-Content $f"
