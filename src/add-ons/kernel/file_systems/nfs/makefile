## ****************************************** ## 
## BeOS Generic Makefile v2.9.0.Zeta-internal ## 

## Fill in this file to specify the project being created, and the referenced 
## makefile-engine will do all of the hard work for you.  This handles both 
## Intel and PowerPC builds of the BeOS. 

## Application Specific Settings --------------------------------------------- 

# specify the name of the binary 
NAME :=nfs

# specify the type of binary 
#       APP:    Application 
#       SHARED: Shared library 
#		ADDON:  An add-on
#       STATIC: Static library archive 
#       DRIVER: Kernel Driver 
#		MODULE: Kernel Module
#		DECOR:	A window decorator project
TYPE :=MODULE

#       add support for new Pe and Eddie features 
#       to fill in generic makefile 

#%{ 
# @src->@ 

#       specify the source files to use 
#       full paths or paths relative to the makefile can be included 
#       all files, regardless of directory, will have their object 
#       files created in the common object directory. 
#       Note that this means this makefile will not work correctly 
#       if two source files with the same name (source.c or source.cpp) 
#       are included from different directories.  Also note that spaces 
#       in folder names do not work well with this makefile. 
SRCS := $(wildcard *.c)

#       specify the resource files to use 
#       full path or a relative path to the resource file can be used. 
RSRCS :=

#       Specify your RDEF files, if any. 
RDEFS :=

# @<-src@ 
#%} 

#       end support for Pe and Eddie 

#       specify additional libraries to link against 
#       there are two acceptable forms of library specifications 
#       -       if your library follows the naming pattern of: 
#               libXXX.so or libXXX.a you can simply specify XXX 
#               library: libbe.so entry: be 
#               
#       -       if your library does not follow the standard library 
#               naming scheme you need to specify the path to the library 
#               and it's name 
#               library: my_lib.a entry: my_lib.a or path/my_lib.a 
LIBS :=

#       specify additional paths to directories following the standard 
#       libXXX.so or libXXX.a naming scheme.  You can specify full paths 
#       or paths relative to the makefile.  The paths included may not 
#       be recursive, so include $(BUILD_SCRIPT_DIR)/all of the paths where libraries can 
#       be found.  Directories where source files are found are 
#       automatically included. 
LIBPATHS :=

#       additional paths to look for system headers 
#       thes use the form: #include $(BUILD_SCRIPT_DIR)/<header> 
#       source file directories are NOT auto-included here 
SYSTEM_INCLUDE_PATHS :=

#       additional paths to look for local headers 
#       thes use the form: #include $(BUILD_SCRIPT_DIR)/"header" 
#       source file directories are automatically included 
LOCAL_INCLUDE_PATHS :=

#       specify the level of optimization that you desire 
#       NONE, SOME, FULL 
OPTIMIZE :=FULL

#       specify any preprocessor symbols to be defined.  The symbols will not 
#       have their values set automatically; you must supply the value (if any) 
#       to use.  For example, setting DEFINES to "DEBUG=1" will cause the 
#       compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG" 
#       would pass "-DDEBUG" on the compiler's command line. 
DEFINES := _BUILDING_fs=1

#       specify special warning levels 
#       if unspecified default warnings will be used 
#       NONE = supress all warnings 
#       ALL = enable all warnings 
WARNINGS :=ALL

#       specify whether image symbols will be created 
#       so that stack crawls in the debugger are meaningful 
#       if TRUE symbols will be created 
SYMBOLS :=

#       specify debug settings 
#       if TRUE will allow application to be run from a source-level 
#       debugger.  Note that this will disable all optimzation. 
DEBUGGER :=

#       specify additional compiler flags for all files 
COMPILER_FLAGS :=

#       specify additional linker flags 
LINKER_FLAGS :=

#       specify additional flex flags 
FLEX_FLAGS =

#       specify additional bison flags 
BISON_FLAGS =

#       specify the version of this particular item 
#       (for example, -app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL") 
#       This may also be specified in a resource. 
APP_VERSION :=

#       (for TYPE == DRIVER only) Specify desired location of driver in the /dev 
#       hierarchy. Used by the driverinstall rule. E.g., DRIVER_PATH = video/usb will 
#       instruct the driverinstall rule to place a symlink to your driver's binary in 
#       ~/add-ons/kernel/drivers/dev/video/usb, so that your driver will appear at 
#       /dev/video/usb when loaded. Default is "misc". 
DRIVER_PATH     :=

#       Specify if you want the object files to be somewhere besides the default location. 
OBJ_DIR         :=

#       Specify a non default placement for the target 
TARGET_DIR      :=

#       If you want to see the complete build line for every file, then set this to 'true', 
#       otherwise it will tell you at the end what the build flags were. 
#CHATTY          :=

#       Special Zeta build system var 
BOOTREL_INSTALL_DIR     := beos/system/add-ons/kernel/file_systems

#       Specify a directory for the 'install' target. 
INSTALL_DIR     := /boot/$(BOOTREL_INSTALL_DIR)

#       Specify the name of this makefile. 
#       If you leave this blank, the makefile will not be considered as part of the 
#       dependenies for the project, and the project will not be rebuilt when the makefile 
#       is changed 
MAKEFILE        :=

#       Specify TRUE if you want the install target to create links in the BeMenu 
MENU_LINKS      :=

#       Related to MENU_LINKS, specify the name of the direcotry in the BeMenu 
#       you wish the link to go in. If the directory does not exist, it will be 
#       created. 
APP_MENU        := 

#       If, for some reason, you don't want to use the dependencies (flex and yacc seem to choke 
#       on them), set this to false 
DODEPS          := 

#	Set this variable if you have an svg text file you wish to use as 
#   your target's icon.
SVG_ICON	:=

#	If you have some fancy custom build steps to do, specify them here
EXTRA_BUILD_STEPS	=


#	If you have some other files that should trigger a re-link, such as libs in the same
#	project that may get rebuilt, specify the full path to them here.
EXTRA_DEPS :=


################################################################################################## 
# 
##                                  Z E T A  S p e c i f i c 
# 
################################################################################################## 

#       Specify the path to the Screenshot file. If this is not specified, then the SCREENSHOT default 
#       will be $(PWD)/$(NAME).png 
#       If this application is not to have a screenshot, use NONE 
SCREENSHOT      :=

#       Specify the category that this zeta component belongs in 
ZETA_CATEGORY   := BaseInstallation

#       The .zip package that this file will be included in. If this is left blank, then 
#       $(NAME).zip will be used. 
#       This feature is meant for having multiple items included in the a single .zip package 
PACKAGE         :=

#       If this target is a component of another target. That is, if this is part of a package 
#       but not the 'main' part. Typically the main app will have the screenshot, and set 
#       the relevant attributes. If this is set to 'true' then this makefile will not attempt to set 
#       attributes or a screenshot or a description. 
IS_COMPONENT    :=

#       If this is a mandatory component, set this to 'false'. The default is true. 
#       Optional components will be zipped up with thier attribs and everything. 
#       Non-Optional (mandatory) components will just be copied to the directory structure and 
#       added to the manifest for copying during install 
OPTIONAL		:=false

#       Specify the names of the Language files here. (ie: AboutWindow.de AboutWindow.en) 
LANG_FILES		:=

#       If your language files are in a different directory, specify the relative directory here. 
#       ie: Language/Dictionaries 
LANG_FILES_DIR  :=


##include $(BUILD_SCRIPT_DIR)/the makefile-engine 
include $(BUILDHOME)/etc/makefile-engine
