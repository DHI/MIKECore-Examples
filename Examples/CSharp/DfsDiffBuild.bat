set csc=C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe
set sdkBin=..\..\bin
%csc% -r:"%sdkBin%\DHI.Mike.Install.dll" -r:"%sdkBin%\DHI.Generic.MikeZero.DFS.dll" -r:"%sdkBin%\DHI.Generic.MikeZero.EUM.dll" -out:%sdkBin%\DfsDiff.exe DfsDiff.cs 
REM pause