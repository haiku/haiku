# Microsoft Developer Studio Project File - Name="Perl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Perl - Win32 perlAS56
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Perl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Perl.mak" CFG="Perl - Win32 perlAS56"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Perl - Win32 perlAS56" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Perl - Win32 perlAS58" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Perl - Win32 perl5005" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Perl - Win32 perlAS56"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "perlAS56"
# PROP BASE Intermediate_Dir "perlAS56"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "perlAS56"
# PROP Intermediate_Dir "perlAS56"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl\lib\CORE" /I "c:\programme\Perl\lib\CORE\win32" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl\lib\CORE" /I "c:\programme\Perl56\lib\CORE" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib perl56.lib pdflib.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perlAS56\pdflib_pl.dll" /libpath:"c:\programme\Perl\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib perl56.lib pdflib.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perlAS56\pdflib_pl.dll" /libpath:"c:\programme\Perl56\lib\CORE" /libpath:"c:\programme\Perl\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Perl - Win32 perlAS58"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "perlAS58"
# PROP BASE Intermediate_Dir "perlAS58"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "perlAS58"
# PROP Intermediate_Dir "perlAS58"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl\lib\CORE" /I "c:\programme\Perl\lib\CORE\win32" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl58\lib\CORE" /I "c:\programme\Perl58\lib\CORE\win32" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib perl56.lib pdflib.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perlAS58\pdflib_pl.dll" /libpath:"c:\programme\Perl\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib perl58.lib pdflib.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perlAS58\pdflib_pl.dll" /libpath:"c:\programme\Perl58\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Perl - Win32 perl5005"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "perl5005"
# PROP BASE Intermediate_Dir "perl5005"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "perl5005"
# PROP Intermediate_Dir "perl5005"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl5.005\lib\CORE" /I "c:\programme\Perl5.005\lib\CORE\win32" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /D "PERL_OBJECT" /FR /YX /FD /c /Tp
# ADD CPP /nologo /MT /W3 /O2 /I "c:\programme\Perl5.005\lib\CORE" /I "c:\programme\Perl5.005\lib\CORE\win32" /I "..\..\..\libs\pdflib" /D "WIN32" /D "_MT" /D "PDFLIB_STATIC" /D "PERL_OBJECT" /FR /YX /FD /c /Tp
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pdflib.lib perlcore.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perl5005\pdflib_pl.dll" /libpath:"c:\programme\Perl5.005\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib pdflib.lib perlcore.lib /nologo /base:"0x55330000" /dll /pdb:none /machine:I386 /out:"perl5005\pdflib_pl.dll" /libpath:"c:\programme\Perl5.005\lib\CORE" /libpath:"..\..\..\libs\pdflib\Release"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "Perl - Win32 perlAS56"
# Name "Perl - Win32 perlAS58"
# Name "Perl - Win32 perl5005"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\pdflib_pl.c
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
