/*
 * Copyright 2007-2023, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#include <SupportDefs.h>
#include <string.h>
#include "toscalls.h"
#include <util/kernel_cpp.h>

#include "console.h"
#include "keyboard.h"


static bool sForceBW = false;	// force black & white for Milan


// TOS emulates a VT52

class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);

		void ClearScreen();
		int32 Width();
		int32 Height();
		void SetCursor(int32 x, int32 y);
		void SetCursorVisible(bool visible);
		void SetColors(int32 foreground, int32 background);
	private:
		int16 fHandle;
};


extern ConsoleNode* gConsoleNode;
static Console sConsole;
FILE *stdin, *stdout, *stderr;


Console::Console()
	: ConsoleNode(), fHandle(DEV_CONSOLE)
{
}


ssize_t
Console::ReadAt(void */*cookie*/, off_t /*pos*/, void *buffer,
	size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
Console::WriteAt(void */*cookie*/, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;
	size_t i;

	// be nice to our audience and replace single "\n" with "\r\n"

	for (i = 0; i < bufferSize; i++) {
		if (string[i] == '\0')
			break;
		if (string[i] == '\n')
			Bconout(fHandle, '\r');
		Bconout(fHandle, string[i]);
	}

	return bufferSize;
}


void
Console::ClearScreen()
{
	Write("\033E", 2);
}


int32
Console::Width()
{
	return 80;
}


int32
Console::Height()
{
	return 25;
}


void
Console::SetCursor(int32 x, int32 y)
{
	char buff[] = "\033Y  ";
	x = MIN(79,MAX(0,x));
	y = MIN(24,MAX(0,y));
	buff[3] += (char)x;
	buff[2] += (char)y;
	Write(buff, 4);
}


void
Console::SetCursorVisible(bool visible)
{
	// TODO
}

static int
translate_color(int32 color)
{
	/*
			r	g	b
		0:	0	0	0		// black
		1:	0	0	aa		// dark blue
		2:	0	aa	0		// dark green
		3:	0	aa	aa		// cyan
		4:	aa	0	0		// dark red
		5:	aa	0	aa		// purple
		6:	aa	55	0		// brown
		7:	aa	aa	aa		// light gray
		8:	55	55	55		// dark gray
		9:	55	55	ff		// light blue
		a:	55	ff	55		// light green
		b:	55	ff	ff		// light cyan
		c:	ff	55	55		// light red
		d:	ff	55	ff		// magenta
		e:	ff	ff	55		// yellow
		f:	ff	ff	ff		// white
	*/
	// cf. http://www.fortunecity.com/skyscraper/apple/308/html/chap4.htm
	static const char cmap[] = {
		15, 4, 2, 6, 1, 5, 3, 7,
		8, 12, 10, 14, 9, 13, 11, 0 };

	if (color < 0 || color >= 16)
		return 0;
	return cmap[color];
}


void
Console::SetColors(int32 foreground, int32 background)
{
	char buff[] = "\033b \033c ";
	if (sForceBW) {
		if (background == 0)
			foreground = 15;
		else {
			background = 15;
			foreground = 0;
		}

	}
	buff[2] += (char)translate_color(foreground);
	buff[5] += (char)translate_color(background);
	Write(buff, 6);
}


//	#pragma mark -


constexpr bool kDumpColors = false;
constexpr bool kDumpMilanModes = false;

static void
dump_colors()
{
	int bg, fg;
	dprintf("colors:\n");
	for (bg = 0; bg < 16; bg++) {
		for (fg = 0; fg < 16; fg++) {
			console_set_color(fg, bg);
			dprintf("#");
		}
		console_set_color(0, 15);
		dprintf("\n");
	}
}


static int32
dump_milan_modes(SCREENINFO *info, uint32 flags)
{
	dprintf("mode: %d '%s':\n flags %08" B_PRIx32 " @%08" B_PRIx32 " %dx%d (%dx%d)\n%d planes %d colors fmt %08" B_PRIx32 "\n",
		info->devID, info->name, info->scrFlags, info->frameadr,
		info->scrWidth, info->scrHeight,
		info->virtWidth, info->virtHeight,
		info->scrPlanes, info->scrColors, info->scrFormat);
	return ENUMMODE_CONT;
}

status_t
console_init(void)
{
	gConsoleNode = &sConsole;

	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sConsole;
	stdout = stderr = (FILE *)&sConsole;

	if (tos_find_cookie('_MIL')) {
		dprintf("Milan detected... forcing black & white\n");
		if (kDumpMilanModes) {
			dprintf("Getrez() = %d\n", Getrez());
			Setscreen(-1, &dump_milan_modes, MI_MAGIC, CMD_ENUMMODES);
			Setscreen((void*)-1, (void*)-1, 0, 0);
		}
		sForceBW = true;
	}

	if (kDumpColors)
		dump_colors();

	return B_OK;
}


// #pragma mark -


int
console_wait_for_key(void)
{
#if 0
	// XXX: do this way and remove keyboard.cpp ?
	// wait for a key
	char buffer[3];
	ssize_t bytesRead;
	do {
		bytesRead = sInput.ReadAt(NULL, 0, buffer, 3);
		if (bytesRead < 0)
			return 0;
	} while (bytesRead == 0);
#endif
	union key key = wait_for_key();

	if (key.code.ascii == 0) {
		switch (key.code.bios) {
			case BIOS_KEY_UP:
				return TEXT_CONSOLE_KEY_UP;
			case BIOS_KEY_DOWN:
				return TEXT_CONSOLE_KEY_DOWN;
			case BIOS_KEY_PAGE_UP:
				return TEXT_CONSOLE_KEY_PAGE_UP;
			case BIOS_KEY_PAGE_DOWN:
				return TEXT_CONSOLE_KEY_PAGE_DOWN;
			case BIOS_KEY_HOME:
				return TEXT_CONSOLE_KEY_HOME;
			case BIOS_KEY_END:
				return TEXT_CONSOLE_KEY_END;
			default:
				return 0;
		}
	} else
		return key.code.ascii;
}

