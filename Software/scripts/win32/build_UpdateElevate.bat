IF NOT "%VS120COMNTOOLS%" == "" (
	call "%VS120COMNTOOLS%\vsvars32.bat"
) ELSE (
	call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
)
MSBuild.exe UpdateElevate\UpdateElevate.sln /p:Configuration=Release