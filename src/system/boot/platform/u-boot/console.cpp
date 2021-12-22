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


class VTConsole : public ConsoleNode {
public:
							VTConsole();

	virtual void			ClearScreen();
	virtual int32			Width();
	virtual int32			Height();
	virtual void			SetCursor(int32 x, int32 y);
	virtual void			SetCursorVisible(bool visible);
	virtual void			SetColors(int32 foreground, int32 background);
};


class SerialConsole : public VTConsole {
public:
							SerialConsole();

	virtual ssize_t			ReadAt(void *cookie, off_t pos, void *buffer,
								size_t bufferSize);
	virtual ssize_t			WriteAt(void *cookie, off_t pos, const void *buffer,
								size_t bufferSize);
};


extern ConsoleNode* gConsoleNode;
static SerialConsole sSerial;

FILE* stdin;
FILE* stdout;
FILE* stderr;


//	#pragma mark -


VTConsole::VTConsole()
	:
	ConsoleNode()
{
}


void
VTConsole::ClearScreen()
{
	WriteAt(NULL, 0LL, "\033[2J", 4);
}


int32
VTConsole::Width()
{
	// TODO?
	return 80;
}


int32
VTConsole::Height()
{
	// TODO?
	return 25;
}


void
VTConsole::SetCursor(int32 x, int32 y)
{
	char buffer[9];
	x = MIN(80, MAX(1, x));
	y = MIN(25, MAX(1, y));
	int len = snprintf(buffer, sizeof(buffer),
		"\033[%" B_PRId32 ";%" B_PRId32 "H", y, x);
	WriteAt(NULL, 0LL, buffer, len);
}


void
VTConsole::SetCursorVisible(bool visible)
{
	// TODO?
}


void
VTConsole::SetColors(int32 foreground, int32 background)
{
	static const char cmap[] = {
		0, 4, 2, 6, 1, 5, 3, 7 };
	char buffer[12];

	if (foreground < 0 || foreground >= 8)
		return;
	if (background < 0 || background >= 8)
		return;

	// We assume normal display attributes here
	int len = snprintf(buffer, sizeof(buffer),
		"\033[%" B_PRId32 ";%" B_PRId32 "m",
		cmap[foreground] + 30, cmap[background] + 40);

	WriteAt(NULL, 0LL, buffer, len);
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


int
console_wait_for_key(void)
{
	union key key;
	key.ax = serial_getc(true);
	return key.code.ascii;
}


status_t
console_init(void)
{
	gConsoleNode = &sSerial;
	stdin = (FILE *)&sSerial;
	stdout = (FILE *)&sSerial;
	stderr = (FILE *)&sSerial;
	return B_OK;
}
