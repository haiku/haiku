# Microsoft Developer Studio Project File - Name="pdcore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=pdcore - Win32 Debug DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pdcore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pdcore.mak" CFG="pdcore - Win32 Debug DLL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pdcore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Release mtDLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Release PSP" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Release mtDLL PSP" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Debug PSP" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Release DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Debug PSP DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Debug DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdcore - Win32 Release PSP DLL" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pdcore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Release mtDLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_mtDLL"
# PROP BASE Intermediate_Dir "Release_mtDLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_mtDLL"
# PROP Intermediate_Dir "Release_mtDLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "..\pdflib" /I "..\flate" /I "..\tiff" /I "..\png" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Release PSP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_PSP"
# PROP BASE Intermediate_Dir "Release_PSP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_PSP"
# PROP Intermediate_Dir "Release_PSP"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Release mtDLL PSP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_mtDLL_PSP"
# PROP BASE Intermediate_Dir "Release_mtDLL_PSP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_mtDLL_PSP"
# PROP Intermediate_Dir "Release_mtDLL_PSP"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Debug PSP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_PSP"
# PROP BASE Intermediate_Dir "Debug_PSP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_PSP"
# PROP Intermediate_Dir "Debug_PSP"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdcore.lib"
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_DLL"
# PROP BASE Intermediate_Dir "Release_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_DLL"
# PROP Intermediate_Dir "Release_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdcore.lib"
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Debug PSP DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_PSP_DLL"
# PROP BASE Intermediate_Dir "Debug_PSP_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_PSP_DLL"
# PROP Intermediate_Dir "Debug_PSP_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdcore.lib"
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_DLL"
# PROP BASE Intermediate_Dir "Debug_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_DLL"
# PROP Intermediate_Dir "Debug_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdcore.lib"
# ADD LIB32 /nologo /out:"pdcore.lib"

!ELSEIF  "$(CFG)" == "pdcore - Win32 Release PSP DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_PSP_DLL"
# PROP BASE Intermediate_Dir "Release_PSP_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_PSP_DLL"
# PROP Intermediate_Dir "Release_PSP_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../pdi" /I "../flate" /I "../pdflib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_PSP_BUILD" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdcore.lib"
# ADD LIB32 /nologo /out:"pdcore.lib"

!ENDIF 

# Begin Target

# Name "pdcore - Win32 Release"
# Name "pdcore - Win32 Debug"
# Name "pdcore - Win32 Release mtDLL"
# Name "pdcore - Win32 Release PSP"
# Name "pdcore - Win32 Release mtDLL PSP"
# Name "pdcore - Win32 Debug PSP"
# Name "pdcore - Win32 Release DLL"
# Name "pdcore - Win32 Debug PSP DLL"
# Name "pdcore - Win32 Debug DLL"
# Name "pdcore - Win32 Release PSP DLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\pc_arc4.c
# End Source File
# Begin Source File

SOURCE=.\pc_core.c
# End Source File
# Begin Source File

SOURCE=.\pc_corefont.c
# End Source File
# Begin Source File

SOURCE=.\pc_crypt.c
# End Source File
# Begin Source File

SOURCE=.\pc_ebcdic.c
# End Source File
# Begin Source File

SOURCE=.\pc_encoding.c
# End Source File
# Begin Source File

SOURCE=.\pc_file.c
# End Source File
# Begin Source File

SOURCE=.\pc_font.c
# End Source File
# Begin Source File

SOURCE=.\pc_geom.c
# End Source File
# Begin Source File

SOURCE=.\pc_md5.c
# End Source File
# Begin Source File

SOURCE=.\pc_optparse.c
# End Source File
# Begin Source File

SOURCE=.\pc_output.c
# End Source File
# Begin Source File

SOURCE=.\pc_sbuf.c
# End Source File
# Begin Source File

SOURCE=.\pc_scope.c
# End Source File
# Begin Source File

SOURCE=.\pc_unicode.c
# End Source File
# Begin Source File

SOURCE=.\pc_util.c
# End Source File
# End Group
# End Target
# End Project
