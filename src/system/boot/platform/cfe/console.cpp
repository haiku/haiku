/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Handle.h"
#include "console.h"

#include <SupportDefs.h>
#include <boot/platform/cfe/cfe.h>
#include <boot/stage2.h>
#include <util/kernel_cpp.h>

#include <string.h>


class Console : public Handle {
	public:
		Console();
};

class VTConsole : public Console {
	public:
		VTConsole();
		void	ClearScreen();
		void	SetCursor(int32 x, int32 y);
		void	SetColor(int32 foreground, int32 background);
};

static VTConsole sInput, sOutput;
FILE *stdin, *stdout, *stderr;


//	#pragma mark -


Console::Console()
	: Handle()
{
}


//	#pragma mark -


VTConsole::VTConsole()
	: Console()
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
	x = MIN(79,MAX(0,x));
	y = MIN(24,MAX(0,y));
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


//	#pragma mark -


void
console_clear_screen(void)
{
	sOutput.ClearScreen();
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
	sOutput.SetCursor(x, y);
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
	sOutput.SetColor(foreground, background);
}


int
console_wait_for_key(void)
{
    return 0;
}


int
console_check_for_key(void)
{
	return 0;
}


status_t
console_init(void)
{
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	int handle = cfe_getstdhandle(CFE_STDHANDLE_CONSOLE);
	if (handle < 0)
		return cfe_error(handle);

	sInput.SetHandle(handle);
	sOutput.SetHandle(handle);

	return B_OK;
}

