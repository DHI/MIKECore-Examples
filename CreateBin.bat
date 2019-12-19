IF NOT DEFINED MikeCoreBin (SET MikeCoreBin=%userroot%\Source\NuGet\MikeCore\distbin)
IF NOT EXIST bin MKDIR bin

copy %MikeCoreBin%\*.* bin
copy Examples\CSharp\DHI.MikeCore.Util\bin\x64\Release\DHI.MikeCore.Util.exe bin
cd Examples\CSharp
call DfsDiffBuild.bat
call MeshMergerBuild.bat
cd ..
cd ..
