# Microsoft Developer Studio Project File - Name="pdf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=pdf - Win32 Debug DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pdf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pdf.mak" CFG="pdf - Win32 Debug DLL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pdf - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pdf - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "pdf - Win32 Release mtDLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdf - Win32 Release DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "pdf - Win32 Debug DLL" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pdf - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /O2 /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /I "../pdpage" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /U "PDF_FEATURE_PDI" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdf.lib"

!ELSEIF  "$(CFG)" == "pdf - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdf.lib"

!ELSEIF  "$(CFG)" == "pdf - Win32 Release mtDLL"

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
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pdf.lib"

!ELSEIF  "$(CFG)" == "pdf - Win32 Release DLL"

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
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /I "../pdpage" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /U "PDF_FEATURE_PDI" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /I "../pdpage" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_EXPORTS" /U "PDF_FEATURE_PDI" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdf.lib"
# ADD LIB32 /nologo /out:"pdf.lib"

!ELSEIF  "$(CFG)" == "pdf - Win32 Debug DLL"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../png" /I "../flate" /I "../tiff" /I "../pdi" /I "../pdcore" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_MT" /D "PDFLIB_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"pdf.lib"
# ADD LIB32 /nologo /out:"pdf.lib"

!ENDIF 

# Begin Target

# Name "pdf - Win32 Release"
# Name "pdf - Win32 Debug"
# Name "pdf - Win32 Release mtDLL"
# Name "pdf - Win32 Release DLL"
# Name "pdf - Win32 Debug DLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\p_afm.c
# End Source File
# Begin Source File

SOURCE=.\p_annots.c
# End Source File
# Begin Source File

SOURCE=.\p_block.c
# End Source File
# Begin Source File

SOURCE=.\p_bmp.c
# End Source File
# Begin Source File

SOURCE=.\p_ccitt.c
# End Source File
# Begin Source File

SOURCE=.\p_cid.c
# End Source File
# Begin Source File

SOURCE=.\p_color.c
# End Source File
# Begin Source File

SOURCE=.\p_draw.c
# End Source File
# Begin Source File

SOURCE=.\p_encoding.c
# End Source File
# Begin Source File

SOURCE=.\p_filter.c
# End Source File
# Begin Source File

SOURCE=.\p_font.c
# End Source File
# Begin Source File

SOURCE=.\p_gif.c
# End Source File
# Begin Source File

SOURCE=.\p_gstate.c
# End Source File
# Begin Source File

SOURCE=.\p_hostfont.c
# End Source File
# Begin Source File

SOURCE=.\p_hyper.c
# End Source File
# Begin Source File

SOURCE=.\p_icc.c
# End Source File
# Begin Source File

SOURCE=.\p_icclib.c
# End Source File
# Begin Source File

SOURCE=.\p_image.c
# End Source File
# Begin Source File

SOURCE=.\p_jpeg.c
# End Source File
# Begin Source File

SOURCE=.\p_kerning.c
# End Source File
# Begin Source File

SOURCE=.\p_params.c
# End Source File
# Begin Source File

SOURCE=.\p_pattern.c
# End Source File
# Begin Source File

SOURCE=.\p_pdi.c
# End Source File
# Begin Source File

SOURCE=.\p_pfm.c
# End Source File
# Begin Source File

SOURCE=.\p_png.c
# End Source File
# Begin Source File

SOURCE=.\p_resource.c
# End Source File
# Begin Source File

SOURCE=.\p_shading.c
# End Source File
# Begin Source File

SOURCE=.\p_subsett.c
# End Source File
# Begin Source File

SOURCE=.\p_template.c
# End Source File
# Begin Source File

SOURCE=.\p_text.c
# End Source File
# Begin Source File

SOURCE=.\p_tiff.c
# End Source File
# Begin Source File

SOURCE=.\p_truetype.c
# End Source File
# Begin Source File

SOURCE=.\p_type1.c
# End Source File
# Begin Source File

SOURCE=.\p_type3.c
# End Source File
# Begin Source File

SOURCE=.\p_xgstate.c
# End Source File
# End Group
# End Target
# End Project
