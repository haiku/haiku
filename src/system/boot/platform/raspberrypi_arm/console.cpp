/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "console.h"

#include <SupportDefs.h>

#include "keyboard.h"


class Console : public ConsoleNode {
public:
								Console();

	virtual	ssize_t				ReadAt(void* cookie, off_t pos, void* buffer,
									size_t bufferSize);
	virtual	ssize_t				WriteAt(void* cookie, off_t pos,
									const void* buffer, size_t bufferSize);
};

static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 25;
static uint32 sScreenOffset = 0;
static uint16 sColor = 0x0f00;

static Console sInput, sOutput;

FILE* stdin;
FILE* stdout;
FILE* stderr;


//	#pragma mark -


Console::Console()
	: ConsoleNode()
{
}


ssize_t
Console::ReadAt(void* cookie, off_t pos, void* buffer, size_t bufferSize)
{
#warning IMPLEMENT ReadAt
	return 0;
}


ssize_t
Console::WriteAt(void* cookie, off_t /*pos*/, const void* buffer,
	size_t bufferSize)
{
#warning IMPLEMENT WriteAt
	return 0;
}


//	#pragma mark -


void
console_clear_screen(void)
{
#warning IMPLEMENT console_clear_screen
}


int32
console_width(void)
{
#warning IMPLEMENT console_width
	return 0;
}


int32
console_height(void)
{
#warning IMPLEMENT console_height
	return 0;
}


void
console_set_cursor(int32 x, int32 y)
{
#warning IMPLEMENT console_set_cursor
}


void
console_set_color(int32 foreground, int32 background)
{
#warning IMPLEMENT console_set_color
}


int
console_wait_for_key(void)
{
#warning IMPLEMENT console_wait_for_key
	union key key;
	return key.code.ascii;
}


void
console_show_cursor(void)
{
#warning IMPLEMENT console_show_cursor
}


void
console_hide_cursor(void)
{
#warning IMPLEMENT console_hide_cursor
}


status_t
console_init(void)
{
#warning IMPLEMENT console_init
	return B_OK;
}

