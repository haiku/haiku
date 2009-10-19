# Microsoft Developer Studio Project File - Name="Tcl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Tcl - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Tcl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Tcl.mak" CFG="Tcl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Tcl - Win32 Release mtDLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Tcl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Tcl - Win32 Release mtDLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Release_mtDLL"
# PROP BASE Intermediate_Dir "Release_mtDLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Release_mtDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "..\..\..\libs\pdflib" /I "c:\programme\tcl\include" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\libs\pdflib" /I "c:\programme\tcl\include" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib tclstub82.lib /nologo /base:"0x55350000" /dll /pdb:none /machine:I386 /out:"pdflib_tcl.dll" /libpath:"c:\programme\tcl\lib"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib tclstub84.lib pdflib.lib /nologo /base:"0x55350000" /dll /pdb:none /machine:I386 /out:"pdflib_tcl.dll" /libpath:"..\..\..\libs\pdflib\Release_mtDLL" /libpath:"c:\programme\tcl\lib"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Tcl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Tcl___Win32_Release"
# PROP BASE Intermediate_Dir "Tcl___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /I "..\..\..\libs\pdflib" /I "c:\programme\tcl\include" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "..\..\..\libs\pdflib" /I "c:\programme\tcl\include" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MT" /D "PDFLIB_STATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /x
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib tclstub84.lib pdflib.lib /nologo /base:"0x55350000" /dll /pdb:none /machine:I386 /out:"pdflib_tcl.dll" /libpath:"..\..\..\libs\pdflib\Release_mtDLL" /libpath:"c:\programme\tcl\lib"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib tclstub84.lib pdflib.lib /nologo /base:"0x55350000" /dll /pdb:none /machine:I386 /out:"pdflib_tcl.dll" /libpath:"..\..\..\libs\pdflib\Release" /libpath:"c:\programme\tcl\lib"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "Tcl - Win32 Release mtDLL"
# Name "Tcl - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=pdflib_tcl.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
