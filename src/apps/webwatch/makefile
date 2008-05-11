# Makefile for WebWatch

NAME= WebWatch
TYPE= APP
SRCS= WatchApp.cpp WatchView.cpp
RSRCS= WebWatch.rsrc
LIBS= be
LIBPATHS= 
SYSTEM_INCLUDE_PATHS = 
LOCAL_INCLUDE_PATHS = 
OPTIMIZE= FULL
DEFINES= 
WARNINGS = ALL
SYMBOLS = FALSE
DEBUGGER = FALSE
COMPILER_FLAGS =
LINKER_FLAGS =

include /boot/develop/etc/makefile-engine

release:
	@strip $(OBJ_DIR)/$(NAME)
	@xres -o $(OBJ_DIR)/$(NAME) $(RSRCS)
	@mimeset -f $(OBJ_DIR)/$(NAME)
