/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#include <SupportDefs.h>
#include <string.h>
#include "toscalls.h"
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "console.h"


// TOS emulates a VT52

class ConsoleHandle : public Handle {
	public:
		ConsoleHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);
	int16	f
};

class InputConsoleHandle : public ConsoleHandle {
	public:
		InputConsoleHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);

		void PutChar(char c);
		void PutChars(const char *buffer, int count);
		char GetChar();

	private:
		enum { BUFFER_SIZE = 32 };

		char	fBuffer[BUFFER_SIZE];
		int		fStart;
		int		fCount;
};


static InputConsoleHandle sInput;
static ConsoleHandle sOutput;
FILE *stdin, *stdout, *stderr;


ConsoleHandle::ConsoleHandle()
	: Handle()
{
}


ssize_t
ConsoleHandle::ReadAt(void */*cookie*/, off_t /*pos*/, void *buffer,
	size_t bufferSize)
{
	// don't seek in character devices
	// not implemented (and not yet? needed)
	return B_ERROR;
}


ssize_t
ConsoleHandle::WriteAt(void */*cookie*/, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;

	// be nice to our audience and replace single "\n" with "\r\n"

	for (i = 0; i < bufferSize; i++) {
		if (string[i] == '\n')
			Bconout(fHandle, '\r');
		Bconout(fHandle, string[i]);
	}

	return bufferSize;
}


//	#pragma mark -


InputConsoleHandle::InputConsoleHandle()
	: ConsoleHandle()
	, fStart(0)
	, fCount(0)
{
}


ssize_t
InputConsoleHandle::ReadAt(void */*cookie*/, off_t /*pos*/, void *_buffer,
	size_t bufferSize)
{
	char *buffer = (char*)_buffer;

	// copy buffered bytes first
	int bytesTotal = 0;
	while (bufferSize > 0 && fCount > 0) {
		*buffer++ = GetChar();
		bytesTotal++;
		bufferSize--;
	}

	// read from console
	if (bufferSize > 0) {
		ssize_t bytesRead = ConsoleHandle::ReadAt(NULL, 0, buffer, bufferSize);
		if (bytesRead < 0)
			return bytesRead;
		bytesTotal += bytesRead;
	}

	return bytesTotal;
}


void
InputConsoleHandle::PutChar(char c)
{
	if (fCount >= BUFFER_SIZE)
		return;

	int pos = (fStart + fCount) % BUFFER_SIZE;
	fBuffer[pos] = c;
	fCount++;
}


void
InputConsoleHandle::PutChars(const char *buffer, int count)
{
	for (int i = 0; i < count; i++)
		PutChar(buffer[i]);
}

		
char
InputConsoleHandle::GetChar()
{
	if (fCount == 0)
		return 0;

	fCount--;
	char c = fBuffer[fStart];
	fStart = (fStart + 1) % BUFFER_SIZE;
	return c;
}


//	#pragma mark -


status_t
console_init(void)
{
	sInput.SetHandle(DEV_CONSOLE);
	sOutput.SetHandle(DEV_CONSOLE);

	// now that we're initialized, enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


// #pragma mark -


void
console_clear_screen(void)
{
	stdin->WriteAt(NULL, 0LL, "\033E", 2);
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
	buff[3] += (char)x;
	buff[2] += (char)y;
	stdin->WriteAt(NULL, 0LL, buff, 4);
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
	if (color >= 0 && color < 16)
		return color;
	return 0;
}


void
console_set_color(int32 foreground, int32 background)
{
	char buff[] = "\033b \033c ";
	buff[2] = (char)translate_color(foreground);
	buff[5] = (char)translate_color(background);
	stdin->WriteAt(NULL, 0LL, buff, 6);
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

