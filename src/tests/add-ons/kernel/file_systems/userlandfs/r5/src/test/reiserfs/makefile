## BeOS Generic Makefile v2.2 ##

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you.  This handles both
## Intel and PowerPC builds of the BeOS.

## Application Specific Settings ---------------------------------------------

# specify the name of the binary
NAME= ../reiserfs

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE= DRIVER

#	add support for new Pe and Eddie features
#	to fill in generic makefile

#%{
# @src->@ 

#	specify the source files to use
#	full paths or paths relative to the makefile can be included
# 	all files, regardless of directory, will have their object
#	files created in the common object directory.
#	Note that this means this makefile will not work correctly
#	if two source files with the same name (source.c or source.cpp)
#	are included from different directories.  Also note that spaces
#	in folder names do not work well with this makefile.
SRCS= \
	Block.cpp \
	BlockCache.cpp \
	cpp.cpp \
	hashes.c \
	Item.cpp \
	Iterators.cpp \
	kernel_interface.cpp \
	Locker.cpp \
	Tree.cpp \
	VNode.cpp \
	Volume.cpp \
	../shared/Debug.cpp \
	../shared/Settings.cpp \
	../shared/String.cpp \
	../shared/SuperBlock.cpp

#	specify the resource files to use
#	full path or a relative path to the resource file can be used.
RSRCS= reiserfs.rsrc

# @<-src@ 
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#		
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
LIBS= 

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
LOCAL_INCLUDE_PATHS = 

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= SOME

#	specify any preprocessor symbols to be defined.  The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use.  For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
#
# USER						- [0/1] userland build
# DEBUG						- [0/1] Enable debugging.
# DBG_PRINT					- [0/1] Print debug output to file (only if DEBUG).
# DBG_PRINT_FILE			- [0/1] Name of debug output file
#							  (only if DBG_PRINT).
# KEEP_WRONG_DIRENT_RECLEN	- [0/1] Reproduce BFS bug in writing the entry
#							  name length into dirent::d_reclen, instead of
#							  record length, as it should be.
#
DEFINES= USER=0 \
		 DEBUG=0 \
		 DEBUG_PRINT=1 \
		 DEBUG_PRINT_FILE=\"/var/log/reiserfs.log\" \
		 KEEP_WRONG_DIRENT_RECLEN=0

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = ALL

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS = TRUE

#	specify debug settings
#	if TRUE will allow application to be run from a source-level
#	debugger.  Note that this will disable all optimzation.
DEBUGGER = FALSE

#	specify additional compiler flags for all files
COMPILER_FLAGS = -include "cpp.h" -include "compatibility.h" \
	-fno-exceptions -fno-rtti \
	-Wmissing-prototypes -Woverloaded-virtual \
	-Wpointer-arith -Wcast-align -Wsign-compare -Wshadow

#	specify additional linker flags
LINKER_FLAGS =

#	specify the version of this particular item
#	(for example, -app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL") 
#	This may also be specified in a resource.
APP_VERSION = 

#	(for TYPE == DRIVER only) Specify desired location of driver in the /dev
#	hierarchy. Used by the driverinstall rule. E.g., DRIVER_PATH = video/usb will
#	instruct the driverinstall rule to place a symlink to your driver's binary in
#	~/add-ons/kernel/drivers/dev/video/usb, so that your driver will appear at
#	/dev/video/usb when loaded. Default is "misc".
DRIVER_PATH = 

## include the makefile-engine
ifeq (x$(USER), xbonefish)
	include /boot/home/develop/makefile-engine
else
	include $(BUILDHOME)/etc/makefile-engine
endif
