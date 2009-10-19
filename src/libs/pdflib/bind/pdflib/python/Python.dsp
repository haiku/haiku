# Microsoft Developer Studio Project File - Name="Python" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Python - Win32 python23
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Python.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Python.mak" CFG="Python - Win32 python23"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Python - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Python - Win32 oldpython" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Python - Win32 python21" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Python - Win32 python22" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Python - Win32 python23" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Python - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Python___Win32_Release"
# PROP BASE Intermediate_Dir "Python___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /O2 /I "c:\Programme\Python20\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python15.lib /nologo /dll /debug /machine:I386 /out:"pdflib.dll" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python20.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"pdflib_py.dll" /libpath:"c:\programme\python20\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "Python - Win32 oldpython"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Python___Win32_oldpython"
# PROP BASE Intermediate_Dir "Python___Win32_oldpython"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "oldpython"
# PROP Intermediate_Dir "oldpython"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "c:\Programme\Python 1.5.2\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "c:\Programme\Python 1.5.2\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python15.lib libpng.lib libtiff.lib zlib.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"pdflib_py.dll" /libpath:"..\..\..\..\libs\zlib" /libpath:"c:\programme\python 1.5.2\libs" /libpath:"..\..\..\libs\pdflib" /libpath:"..\..\..\..\libs\libpng" /libpath:"..\..\..\..\libs\libtiff"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python15.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"oldpython/pdflib_py.dll" /libpath:"c:\programme\python 1.5.2\libs" /libpath:"..\..\..\libs\pdflib"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "Python - Win32 python21"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Python___Win32_python21"
# PROP BASE Intermediate_Dir "Python___Win32_python21"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "python21"
# PROP Intermediate_Dir "python21"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "D:\Programme\Python21\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "C:\Programme\Python21\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python15.lib libpng.lib libtiff.lib zlib.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"pdflib_py.dll" /libpath:"..\..\..\..\libs\zlib" /libpath:"c:\programme\python21\libs" /libpath:"..\..\..\libs\pdflib" /libpath:"..\..\..\..\libs\libpng" /libpath:"..\..\..\..\libs\libtiff"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python21.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"python21/pdflib_py.dll" /libpath:"C:\programme\python21\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "Python - Win32 python22"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Python___Win32_python22"
# PROP BASE Intermediate_Dir "Python___Win32_python22"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "python22"
# PROP Intermediate_Dir "python22"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "C:\Programme\Python21\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "C:\Programme\Python22\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python21.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"python21/pdflib_py.dll" /libpath:"C:\programme\python21\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python22.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"python22/pdflib_py.dll" /libpath:"C:\programme\python22\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "Python - Win32 python23"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Python___Win32_python23"
# PROP BASE Intermediate_Dir "Python___Win32_python23"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Python23"
# PROP Intermediate_Dir "Python23"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "C:\Programme\Python22\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "C:\Programme\Python23\include" /I "..\..\..\libs\pdflib" /D "__WIN32__" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python22.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"python22/pdflib_py.dll" /libpath:"C:\programme\python22\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT BASE LINK32 /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib python23.lib pdflib.lib /nologo /base:"0x55340000" /dll /pdb:none /machine:I386 /out:"python23/pdflib_py.dll" /libpath:"C:\programme\python23\libs" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug /nodefaultlib

!ENDIF 

# Begin Target

# Name "Python - Win32 Release"
# Name "Python - Win32 oldpython"
# Name "Python - Win32 python21"
# Name "Python - Win32 python22"
# Name "Python - Win32 python23"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\pdflib_py.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
