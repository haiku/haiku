/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#include <SupportDefs.h>
#include <string.h>
#include "nextrom.h"
#include <util/kernel_cpp.h>

#include "console.h"
#include "keyboard.h"


static bool sForceBW = false;	// force black & white for Milan

// bootrom has more console info but we don't know the structure
// so we just use getc & putc for now.

class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);

		void PutChar(char c);
		void PutChars(const char *buffer, int count);
		char GetChar();
};


static Console sInput;
static Console sOutput;
FILE *stdin, *stdout, *stderr;


Console::Console()
	: ConsoleNode()
{
}


ssize_t
Console::ReadAt(void */*cookie*/, off_t /*pos*/, void *_buffer,
	size_t bufferSize)
{
	char *buffer = (char*)_buffer;
	int c;
	ssize_t bytesTotal = 0;

	while (bytesTotal < bufferSize && (c = mg->mg_try_getc())) {
		*buffer++ = (char)c;
		bytesTotal++;
	}

	return bytesTotal;
}


ssize_t
Console::WriteAt(void */*cookie*/, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;
	size_t i;

	for (i = 0; i < bufferSize; i++) {
		if (string[i] == '\0')
			break;
		mg->mg_putc(string[i]);
	}

	return bufferSize;
}


void
Console::PutChar(char c)
{
	mg->mg_putc(c);
}


void
Console::PutChars(const char *buffer, int count)
{
	for (int i = 0; i < count; i++)
		mg->mg_putc(buffer[i]);
}


char
Console::GetChar()
{
	return (char)(mg->mg_getc());
}


//	#pragma mark -


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


status_t
console_init(void)
{
	// now that we're initialized, enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	//dump_colors();

	return B_OK;
}


// #pragma mark -


void
console_clear_screen(void)
{
	sInput.WriteAt(NULL, 0LL, "\033E", 2);
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
	char buff[] = "\033Y  ";
	x = MIN(79,MAX(0,x));
	y = MIN(24,MAX(0,y));
	buff[3] += (char)x;
	buff[2] += (char)y;
	sInput.WriteAt(NULL, 0LL, buff, 4);
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
	//return color;
}


void
console_set_color(int32 foreground, int32 background)
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
	sInput.WriteAt(NULL, 0LL, buff, 6);
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

