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


del /F /S /Q tmp\yaya_lang_sep\*

mkdir tmp\yaya_lang_sep\
mkdir tmp\yaya_lang_sep\messagetxt\
copy /B /Y .\ReleaseLangSep\yaya.dll tmp\yaya_lang_sep\yaya.dll
copy /B /Y .\readme.txt tmp\yaya_lang_sep\readme.txt
copy /B /Y .\readme-original.txt tmp\yaya_lang_sep\readme-original.txt
copy /B /Y .\messagetxt\*.* tmp\yaya_lang_sep\messagetxt\
del /F /S /Q tmp\yaya_lang_sep\*.bak
del /F /S /Q tmp\yaya_lang_sep\messagetxt\*.bak

del /F /S /Q tmp\yaya_lang_sep.zip

chdir tmp\yaya_lang_sep\
zip -r -9 -q ..\yaya_lang_sep.zip *
chdir ..\..

