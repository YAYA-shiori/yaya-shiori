del *.bak
mkdir tmp\

del /F /S /Q tmp\yaya\*

mkdir tmp\yaya\
rem mkdir tmp\yaya\messagetxt\
copy /B /Y .\Release\yaya.dll tmp\yaya\yaya.dll
copy /B /Y .\readme.txt tmp\yaya\readme.txt
copy /B /Y .\readme-original.txt tmp\yaya\readme-original.txt
rem copy /B /Y .\messagetxt\*.* tmp\yaya\messagetxt\
del /F /S /Q tmp\yaya\*.bak
rem del /F /S /Q tmp\yaya\messagetxt\*.bak

del /F /S /Q tmp\yaya.zip

chdir tmp\yaya\
zip -r -9 -q ..\yaya.zip *
chdir ..\..

