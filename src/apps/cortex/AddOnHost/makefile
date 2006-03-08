NAME= CortexAddOnHost
TYPE= APP

#%{
# @src->@ 

SRCS= \
	AddOnHostApp.cpp \
	../support/debug_tools.cpp

RSRCS= AddOnHost_Resource.rsrc

# @<-src@ 
#%}

LIBS= be media
X86_LIBS= stdc++.r4
PPC_LIBS= mslcpp_4_0

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme.  You can specify full paths
#	or paths relative to the makefile.  The paths included may not
#	be recursive, so include all of the paths where libraries can
#	be found.  Directories where source files are found are
#	automatically included.
LIBPATHS= 

#	additional paths to look for system headers
#	thes use the form: #include <header>
#	source file directories are NOT auto-included here
SYSTEM_INCLUDE_PATHS = 

#	additional paths to look for local headers
#	thes use the form: #include "header"
#	source file directories are automatically included
LOCAL_INCLUDE_PATHS = ..

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= FULL

DEFINES= #DEBUG=1 NDEBUG=0
DEBUGGER = #TRUE

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = ALL

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS = 

# if TRUE, all symbols will be removed
STRIP_SYMBOLS = TRUE

#	specify additional compiler flags for all files
COMPILER_FLAGS = -DCORTEX_NAMESPACE=cortex

#	specify additional linker flags
LINKER_FLAGS =


## include local makefile-engine
include ../makefile-engine

