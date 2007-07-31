/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DDC_INT_H
#define DDC_INT_H


/*!
	Part of DDC driver	
	Internal header
*/


void _sPrintf(const char *format, ...);
	// no dprintf in user space, but if you know the trick ;)

//bool    set_dprintf_enabled(bool);	/* returns old enable flag */

#define dprintf _sPrintf

// don't use variables here as this is a static library
// and thus the variables will collide with the main program
#define debug_level_flow 2
#define debug_level_info 4
#define debug_level_error 4
#define DEBUG_MSG_PREFIX "DDC "

#include "debug_ext.h"

#endif	// DDC_INT_H
