del *.bak
mkdir tmp\

del /F /S /Q tmp\yaya-src\*
mkdir tmp\yaya-src\
svn export --force . tmp\yaya-src
del /F /S /Q tmp\yaya-src\OriginalAya\*
rmdir tmp\yaya-src\OriginalAya

del /F /S /Q tmp\yaya\*

mkdir tmp\yaya\
copy /B /Y .\Release\yaya.dll tmp\yaya\yaya.dll
copy /B /Y .\readme.txt tmp\yaya\readme.txt
copy /B /Y .\readme-original.txt tmp\yaya\readme-original.txt
upx --best tmp\yaya\yaya.dll

del /F /S /Q tmp\yaya-src.zip
del /F /S /Q tmp\yaya.zip

chdir tmp\yaya-src\
zip -r -9 -q ..\yaya-src.zip *
chdir ..\yaya\
zip -r -9 -q ..\yaya.zip *
chdir ..\..

del /F /S /Q tmp\yaya-src\*

