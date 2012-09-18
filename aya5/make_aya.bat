del *.bak
mkdir tmp\

del /F /S /Q tmp\yaya\*

mkdir tmp\yaya\
copy /B /Y .\Release\yaya.dll tmp\yaya\yaya.dll
copy /B /Y .\Release_EXE\yaya.exe tmp\yaya\yaya.exe
copy /B /Y .\readme.txt tmp\yaya\readme.txt
copy /B /Y .\readme-original.txt tmp\yaya\readme-original.txt
upx --best tmp\yaya\yaya.dll
upx --best tmp\yaya\yaya.exe

del /F /S /Q tmp\yaya.zip

chdir tmp\yaya\
zip -r -9 -q ..\yaya.zip *
chdir ..\..

