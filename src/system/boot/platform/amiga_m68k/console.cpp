/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#include <SupportDefs.h>
#include <string.h>
#include "rom_calls.h"
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "console.h"
#include "keyboard.h"


FILE *stdin, *stdout, *stderr;



//	#pragma mark -


status_t
console_init(void)
{
	//TODO

	return B_OK;
}


// #pragma mark -


void
console_clear_screen(void)
{
	//TODO
}


int32
console_width(void)
{
	int columnCount = 80; //XXX: check video mode
	return columnCount;
}


int32
console_height(void)
{
	int lineCount = 25; //XXX: check video mode
	return lineCount;
}


void
console_set_cursor(int32 x, int32 y)
{
	//TODO
}


void
console_set_color(int32 foreground, int32 background)
{
	//TODO
}


int
console_wait_for_key(void)
{
	//TODO
	return 0;
}

