/*
	Copyright (c) 2003, Thomas Kurschel


	Part of DDC driver
		
	Internal header
*/

// no dprintf in user space, but if you know the trick ;)
void    _kdprintf_(const char *format, ...);
//bool    set_dprintf_enabled(bool);	/* returns old enable flag */

#define dprintf _kdprintf_

// don't use variables here as this is a static library
// and thus the variables will collide with the main program
#define debug_level_flow 2
#define debug_level_info 4
#define debug_level_error 4
#define DEBUG_MSG_PREFIX "DDC "

#include "debug_ext.h"
