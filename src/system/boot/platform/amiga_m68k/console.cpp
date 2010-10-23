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

class ConsoleHandle : public CharHandle {
	public:
		ConsoleHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);
	private:
		static int16	fX;
		static int16	fY;
		uint16	fPen;
};


static Screen *sScreen;
static int16 sFontWidth, sFontHeight;
int16 ConsoleHandle::fX = 0;
int16 ConsoleHandle::fY = 0;

FILE *stdin, *stdout, *stderr, *dbgerr;


//	#pragma mark -

ConsoleHandle::ConsoleHandle()
	: CharHandle()
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
	size_t i, len;

	// be nice to our audience and replace single "\n" with "\r\n"

	for (i = 0, len = 0; i < bufferSize; i++, len++) {
		if (string[i] == '\0')
			break;
		if (string[i] == '\n') {
			//Text(&sScreen->RastPort, &string[i - len], len);
			fX = 0;
			fY++;
			len = 0;
			console_set_cursor(fX, fY);
			continue;
		}
		Text(&sScreen->RastPort, &string[i], 1);
	}

	// not exactly, but we don't care...
	return bufferSize;
}

static ConsoleHandle sOutput;
static ConsoleHandle sErrorOutput;
static ConsoleHandle sDebugOutput;


//	#pragma mark -


status_t
console_init(void)
{
	
	GRAPHICS_BASE_NAME = (GfxBase *)OldOpenLibrary(GRAPHICSNAME);
	if (GRAPHICS_BASE_NAME == NULL)
		panic("Cannot open %s", GRAPHICSNAME);
	
	static NewScreen newScreen = {
		0, 0,
		640, -1,
		2,
		0, 1,
		0x8000,
		0x1,
		NULL,
		"Haiku Loader",
		NULL,
		NULL
	};
	
	sScreen = OpenScreen(&newScreen);
	if (sScreen == NULL)
		panic("OpenScreen()\n");
	
	static const uint16 palette[] = {0xbb9, 0x0, 0xfff, 0x0f0};
	LoadRGB4(&sScreen->ViewPort, palette, 4);
	
	SetDrMd(&sScreen->RastPort, 0);
	// seems not necessary, there is a default font already set.
	/*
	TextAttr attrs = { "Topaz", 8, 0, 0};
	TextFont *font = OpenFont(&attrs);
	*/
	TextFont *font = OpenFont(sScreen->Font);
	if (font == NULL)
		panic("OpenFont()\n");
	sFontHeight = sScreen->Font->ta_YSize;
	sFontWidth = font->tf_XSize;
	
	ClearScreen(&sScreen->RastPort);

	dbgerr = stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


// #pragma mark -


void
console_clear_screen(void)
{
	ClearScreen(&sScreen->RastPort);
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
	Move(&sScreen->RastPort, sFontWidth * x, sFontHeight * y);
	
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

