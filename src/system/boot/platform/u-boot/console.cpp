/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "keyboard.h"

#include <SupportDefs.h>
#include <util/kernel_cpp.h>
#include <boot/stage2.h>

#include <string.h>


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);
};



static Console sInput, sOutput;
FILE *stdin, *stdout, *stderr;


static void
scroll_up()
{
}


//	#pragma mark -


Console::Console()
	: ConsoleNode()
{
}


ssize_t
Console::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
Console::WriteAt(void *cookie, off_t /*pos*/, const void *buffer, size_t bufferSize)
{
	return 0;
}


//	#pragma mark -


void
console_clear_screen(void)
{

}


int32
console_width(void)
{
	return 80;
}


int32
console_height(void)
{
	return 25;
}


void
console_set_cursor(int32 x, int32 y)
{

}


void
console_show_cursor(void)
{
}


void
console_hide_cursor(void)
{
}


void
console_set_color(int32 foreground, int32 background)
{

}


int
console_wait_for_key(void)
{
    return 0;
}


status_t
console_init(void)
{
	return B_OK;
}

