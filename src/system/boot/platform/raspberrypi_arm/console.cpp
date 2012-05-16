/*
 * Copyright 2004-2005, Axel D?rfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "keyboard.h"
#include "serial.h"

#include <SupportDefs.h>
#include <util/kernel_cpp.h>
#include <boot/stage2.h>

#include <string.h>


class Console : public ConsoleNode {
public:
							Console();

	virtual	ssize_t			ReadAt(void* cookie, off_t pos, void* buffer,
								size_t bufferSize);
	virtual	ssize_t			WriteAt(void* cookie, off_t pos,
								const void* buffer, size_t bufferSize);
};


class VTConsole : public ConsoleNode {
public:
							VTConsole();
			void			ClearScreen();
			void			SetCursor(int32 x, int32 y);
			void			SetColor(int32 foreground, int32 background);
};


class SerialConsole : public VTConsole {
public:
							SerialConsole();

	virtual ssize_t			ReadAt(void *cookie, off_t pos, void *buffer,
								size_t bufferSize);
	virtual ssize_t			WriteAt(void *cookie, off_t pos, const void *buffer,
								size_t bufferSize);
};


static Console sInput;
static Console sOutput;
static SerialConsole sSerial;

FILE* stdin;
FILE* stdout;
FILE* stderr;


//	#pragma mark -


Console::Console()
	:
	ConsoleNode()
{
}


ssize_t
Console::ReadAt(void* cookie, off_t pos, void* buffer, size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
Console::WriteAt(void* cookie, off_t /*pos*/, const void* buffer,
	size_t bufferSize)
{
	return 0;
}


//	#pragma mark -


VTConsole::VTConsole()
	:
	ConsoleNode()
{
}


void
VTConsole::ClearScreen()
{
	WriteAt(NULL, 0LL, "\033E", 2);
}


void
VTConsole::SetCursor(int32 x, int32 y)
{
	char buff[] = "\033Y  ";
	x = MIN(79, MAX(0, x));
	y = MIN(24, MAX(0, y));
	buff[3] += (char)x;
	buff[2] += (char)y;
	WriteAt(NULL, 0LL, buff, 4);
}


void
VTConsole::SetColor(int32 foreground, int32 background)
{
	static const char cmap[] = {
		15, 4, 2, 6, 1, 5, 3, 7,
		8, 12, 10, 14, 9, 13, 11, 0 };
	char buff[] = "\033b \033c ";

	if (foreground < 0 && foreground >= 16)
		return;
	if (background < 0 && background >= 16)
		return;

	buff[2] += cmap[foreground];
	buff[5] += cmap[background];
	WriteAt(NULL, 0LL, buff, 6);
}


//     #pragma mark -


SerialConsole::SerialConsole()
	: VTConsole()
{
}


ssize_t
SerialConsole::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
SerialConsole::WriteAt(void *cookie, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	serial_puts((const char *)buffer, bufferSize);
	return bufferSize;
}


//     #pragma mark -


void
console_clear_screen(void)
{
	sSerial.ClearScreen();
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
	sSerial.SetCursor(x, y);
}


void
console_show_cursor(void)
{
}


void
console_hide_cursor(void)
{
}


int
console_wait_for_key(void)
{
	#warning IMPLEMENT console_wait_for_key
	#if 0
	union key key;
	return key.code.ascii;
	#endif
	return 0;
}


void
console_set_color(int32 foreground, int32 background)
{
	sSerial.SetColor(foreground, background);
}


status_t
console_init(void)
{
	stdin = (FILE *)&sSerial;
	stdout = (FILE *)&sSerial;
	stderr = (FILE *)&sSerial;
	return B_OK;
}

