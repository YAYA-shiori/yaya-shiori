# Microsoft Developer Studio Project File - Name="aya5" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aya5 - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "aya5.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "aya5.mak" CFG="aya5 - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "aya5 - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "aya5 - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aya5 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AYA5_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Oy- /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AYA5_EXPORTS" /D WINVER=0x400 /D _WIN32_IE=0 /D _WIN32_WINNT=0x400 /FD /GA /QI0f /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x13700000" /dll /map /machine:I386 /out:"Release/yaya.dll" /libpath:"D:\boost\libs\regex\build\vc6-stlport"

!ELSEIF  "$(CFG)" == "aya5 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AYA5_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AYA5_EXPORTS" /D WINVER=0x400 /D _WIN32_IE=0 /D _WIN32_WINNT=0x400 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /profile /map /debug /machine:I386 /out:"Debug/yaya.dll"

!ENDIF 

# Begin Target

# Name "aya5 - Win32 Release"
# Name "aya5 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "TinyXPath_SRC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tinyxpath\action_store.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\lex_util.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\node_set.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinystr.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinyxml.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinyxmlerror.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinyxmlparser.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tokenlist.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xml_util.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_expression.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_processor.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_stack.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_static.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_stream.cpp
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_syntax.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\aya5.cpp
# End Source File
# Begin Source File

SOURCE=.\aya5.rc
# End Source File
# Begin Source File

SOURCE=.\ayavm.cpp
# End Source File
# Begin Source File

SOURCE=.\basis.cpp
# End Source File
# Begin Source File

SOURCE=.\ccct.cpp
# End Source File
# Begin Source File

SOURCE=.\comment.cpp
# End Source File
# Begin Source File

SOURCE=.\duplevinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\file.cpp
# End Source File
# Begin Source File

SOURCE=.\file1.cpp
# End Source File
# Begin Source File

SOURCE=.\function.cpp
# End Source File
# Begin Source File

SOURCE=.\globalvariable.cpp
# End Source File
# Begin Source File

SOURCE=.\lib.cpp
# End Source File
# Begin Source File

SOURCE=.\lib1.cpp
# End Source File
# Begin Source File

SOURCE=.\localvariable.cpp
# End Source File
# Begin Source File

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\logexcode.cpp
# End Source File
# Begin Source File

SOURCE=.\manifest.cpp
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\mt19937ar.cpp
# End Source File
# Begin Source File

SOURCE=.\parser0.cpp
# End Source File
# Begin Source File

SOURCE=.\parser1.cpp
# End Source File
# Begin Source File

SOURCE=.\selecter.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\sysfunc.cpp
# End Source File
# Begin Source File

SOURCE=.\value.cpp
# End Source File
# Begin Source File

SOURCE=.\valuesub.cpp
# End Source File
# Begin Source File

SOURCE=.\wordmatch.cpp
# End Source File
# Begin Source File

SOURCE=.\wsex.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "TinyXPath_H"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tinyxpath\action_store.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\byte_stream.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\lex_token.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\lex_util.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\node_set.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinystr.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinyxml.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tinyxpath_conf.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\tokenlist.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xml_util.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_expression.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_processor.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_stack.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_static.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_stream.h
# End Source File
# Begin Source File

SOURCE=.\tinyxpath\xpath_syntax.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\aya5.h
# End Source File
# Begin Source File

SOURCE=.\ayavm.h
# End Source File
# Begin Source File

SOURCE=.\basis.h
# End Source File
# Begin Source File

SOURCE=.\ccct.h
# End Source File
# Begin Source File

SOURCE=.\cell.h
# End Source File
# Begin Source File

SOURCE=.\comment.h
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\function.h
# End Source File
# Begin Source File

SOURCE=.\globaldef.h
# End Source File
# Begin Source File

SOURCE=.\lib.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\logexcode.h
# End Source File
# Begin Source File

SOURCE=.\manifest.h
# End Source File
# Begin Source File

SOURCE=.\messages.h
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\mt19937ar.h
# End Source File
# Begin Source File

SOURCE=.\parser0.h
# End Source File
# Begin Source File

SOURCE=.\parser1.h
# End Source File
# Begin Source File

SOURCE=.\selecter.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\sysfunc.h
# End Source File
# Begin Source File

SOURCE=.\value.h
# End Source File
# Begin Source File

SOURCE=.\variable.h
# End Source File
# Begin Source File

SOURCE=.\wordmatch.h
# End Source File
# Begin Source File

SOURCE=.\wsex.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
