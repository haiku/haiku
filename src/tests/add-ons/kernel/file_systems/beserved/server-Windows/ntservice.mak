# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=ntservice - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ntservice - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ntservice - Win32 Release" && "$(CFG)" !=\
 "ntservice - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntservice.mak" CFG="ntservice - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntservice - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "ntservice - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "ntservice - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "ntservice - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : ".\WinDebug\ntservice.exe"

CLEAN : 
	-@erase "$(INTDIR)\CCLIENT.OBJ"
	-@erase "$(INTDIR)\cgiapp.obj"
	-@erase "$(INTDIR)\CLISTEN.OBJ"
	-@erase "$(INTDIR)\FTPClient.obj"
	-@erase "$(INTDIR)\myservice.obj"
	-@erase "$(INTDIR)\NTServApp.obj"
	-@erase "$(INTDIR)\NTServApp.res"
	-@erase "$(INTDIR)\NTService.obj"
	-@erase "$(INTDIR)\SERVICE.OBJ"
	-@erase "$(INTDIR)\SimpleSocket.obj"
	-@erase ".\WinDebug\ntservice.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

.\WinRel\NTService.bsc : $(OUTDIR)  $(BSC32_SBRS)
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE"\
 /Fp"$(INTDIR)/ntservice.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NTServApp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntservice.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /profile /machine:I386 /out:"WinDebug/ntservice.exe"
# SUBTRACT LINK32 /map /debug /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib\
 advapi32.lib ole32.lib oleaut32.lib uuid.lib\
 ws2_32.lib /nologo /subsystem:console /profile\
 /machine:I386 /out:"WinDebug/ntservice.exe" 
LINK32_OBJS= \
	"$(INTDIR)\myservice.obj" \
	"$(INTDIR)\NTServApp.obj" \
	"$(INTDIR)\NTServApp.res" \
	"$(INTDIR)\NTService.obj"

".\WinDebug\ntservice.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ntservice - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\ntservice.exe"

CLEAN : 
	-@erase "$(INTDIR)\CCLIENT.OBJ"
	-@erase "$(INTDIR)\cgiapp.obj"
	-@erase "$(INTDIR)\CLISTEN.OBJ"
	-@erase "$(INTDIR)\FTPClient.obj"
	-@erase "$(INTDIR)\myservice.obj"
	-@erase "$(INTDIR)\NTServApp.obj"
	-@erase "$(INTDIR)\NTServApp.res"
	-@erase "$(INTDIR)\NTService.obj"
	-@erase "$(INTDIR)\SERVICE.OBJ"
	-@erase "$(INTDIR)\SimpleSocket.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\ntservice.exe"
	-@erase "$(OUTDIR)\ntservice.ilk"
	-@erase "$(OUTDIR)\ntservice.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

.\WinDebug\NTService.bsc : $(OUTDIR)  $(BSC32_SBRS)
# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MT /W3 /Gm /GX /Zi /O2 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /Gm /GX /Zi /O2 /D "_DEBUG" /D "WIN32" /D "_CONSOLE"\
 /Fp"$(INTDIR)/ntservice.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NTServApp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ntservice.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /map /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib \
 advapi32.lib ole32.lib oleaut32.lib uuid.lib \
 ws2_32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/ntservice.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/ntservice.exe" 
LINK32_OBJS= \
	"$(INTDIR)\myservice.obj" \
	"$(INTDIR)\NTServApp.obj" \
	"$(INTDIR)\NTServApp.res" \
	"$(INTDIR)\NTService.obj"

"$(OUTDIR)\ntservice.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "ntservice - Win32 Release"
# Name "ntservice - Win32 Debug"

!IF  "$(CFG)" == "ntservice - Win32 Release"

!ELSEIF  "$(CFG)" == "ntservice - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\NTService.cpp
DEP_CPP_NTSER=\
	".\NTService.h"\
	".\ntservmsg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\NTService.obj" : $(SOURCE) $(DEP_CPP_NTSER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NTServApp.cpp
DEP_CPP_NTSERV=\
	".\logger.h"\
	".\myservice.h"\
	".\NTServApp.h"\
	".\NTService.h"\
	".\ntservmsg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\NTServApp.obj" : $(SOURCE) $(DEP_CPP_NTSERV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NTServApp.rc
DEP_RSC_NTSERVA=\
	".\MSG00001.bin"\
	".\ntservmsg.rc"\
	

"$(INTDIR)\NTServApp.res" : $(SOURCE) $(DEP_RSC_NTSERVA) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NTServMsg.mc

!IF  "$(CFG)" == "ntservice - Win32 Release"

!ELSEIF  "$(CFG)" == "ntservice - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\myservice.cpp

!IF  "$(CFG)" == "ntservice - Win32 Release"

DEP_CPP_MYSER=\
	".\myservice.h"\
	".\NTServApp.h"\
	".\NTService.h"\
	".\ntservmsg.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\MSWSOCK.H"\
	{$(INCLUDE)}"\WINSOCK2.H"\
	

"$(INTDIR)\myservice.obj" : $(SOURCE) $(DEP_CPP_MYSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ntservice - Win32 Debug"

DEP_CPP_MYSER=\
	".\myservice.h"\
	".\NTServApp.h"\
	".\NTService.h"\
	".\ntservmsg.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\WINSOCK2.H"\
	

"$(INTDIR)\myservice.obj" : $(SOURCE) $(DEP_CPP_MYSER) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
