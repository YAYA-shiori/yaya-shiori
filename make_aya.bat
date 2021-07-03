del *.bak
mkdir tmp\

del /F /S /Q tmp\aya\*

mkdir tmp\aya\
copy /B /Y .\Release\aya.dll tmp\aya\aya.dll
rem copy /B /Y .\Release_EXE\aya.exe tmp\aya\aya.exe
copy /B /Y .\readme.txt tmp\aya\readme.txt
copy /B /Y .\readme-original.txt tmp\aya\readme-original.txt
rem upx --best tmp\aya\aya.dll
rem upx --best tmp\aya\aya.exe

del /F /S /Q tmp\aya.zip

chdir tmp\aya\
zip -r -9 -q ..\aya.zip *
chdir ..\..

