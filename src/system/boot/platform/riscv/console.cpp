/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "video.h"
#include "graphics.h"
#include <Htif.h>
#include "virtio.h"

#include <SupportDefs.h>
#include <util/kernel_cpp.h>
#include <boot/stage2.h>

#include <string.h>


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);
};

static uint16* sScreenBase;
static uint32 sScreenOfsX = 0;
static uint32 sScreenOfsY = 0;
static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 25;
static uint32 sScreenOffset = 0;
static uint16 sColor = 0x0f00;

static Console sInput, sOutput;
FILE *stdin, *stdout, *stderr;


static const uint32 kPalette[] = {
	0x000000,
	0x0000aa,
	0x00aa00,
	0x00aaaa,
	0xaa0000,
	0xaa00aa,
	0xaa5500,
	0xaaaaaa,
	0x555555,
	0x5555ff,
	0x55ff55,
	0x55ffff,
	0xff5555,
	0xff55ff,
	0xffff55,
	0xffffff
};


static void
RefreshFramebuf(int32 x0, int32 y0, int32 w, int32 h)
{
	for (int32 y = y0; y < y0 + h; y++)
		for (int32 x = x0; x < x0 + w; x++) {
			uint16 cell = sScreenBase[x + y * sScreenWidth];
			int32 charX = sScreenOfsX + x*gFixedFont.charWidth;
			int32 charY = sScreenOfsY + y*gFixedFont.charHeight;
			uint32 bgColor = kPalette[cell / 0x1000 % 0x10];
			uint32 fontColor = kPalette[cell / 0x100 % 0x10];
			Clear(gFramebuf.Clip(charX, charY, gFixedFont.charWidth,
				gFixedFont.charHeight), bgColor);
			BlitMaskRgb(gFramebuf, gFixedFont.ThisGlyph(cell % 0x100),
				charX, charY, fontColor);
		}
}


static void
scroll_up()
{
	memcpy(sScreenBase, sScreenBase + sScreenWidth,
		sScreenWidth * sScreenHeight * 2 - sScreenWidth * 2);
	sScreenOffset = (sScreenHeight - 1) * sScreenWidth;

	for (uint32 i = 0; i < sScreenWidth; i++)
		sScreenBase[sScreenOffset + i] = sColor | ' ';
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
Console::WriteAt(void *cookie, off_t /*pos*/, const void *buffer,
		size_t bufferSize)
{
	const char *string = (const char *)buffer;

	if (gKernelArgs.frame_buffer.enabled)
		return bufferSize;

	for (uint32 i = 0; i < bufferSize; i++) {
		if (string[0] == '\n') {
			sScreenOffset += sScreenWidth - (sScreenOffset % sScreenWidth);
		} else {
			sScreenBase[sScreenOffset++] = sColor | string[0];
			RefreshFramebuf((sScreenOffset - 1) % sScreenWidth,
				(sScreenOffset - 1) / sScreenWidth, 1, 1);
		}

		if (sScreenOffset >= sScreenWidth * sScreenHeight) {
			scroll_up();
			RefreshFramebuf(0, 0, sScreenWidth, sScreenHeight);
		}

		string++;
	}

	return bufferSize;
}


//	#pragma mark -


void
console_clear_screen(void)
{
	if (gKernelArgs.frame_buffer.enabled)
		return;

	for (uint32 i = 0; i < sScreenWidth * sScreenHeight; i++)
		sScreenBase[i] = sColor;

	// reset cursor position as well
	sScreenOffset = 0;

	RefreshFramebuf(0, 0, sScreenWidth, sScreenHeight);
}


int32
console_width(void)
{
	return sScreenWidth;
}


int32
console_height(void)
{
	return sScreenHeight;
}


void
console_set_cursor(int32 x, int32 y)
{
	if (y >= (int32)sScreenHeight)
		y = sScreenHeight - 1;
	else if (y < 0)
		y = 0;
	if (x >= (int32)sScreenWidth)
		x = sScreenWidth - 1;
	else if (x < 0)
		x = 0;

	sScreenOffset = x + y * sScreenWidth;
}


void
console_show_cursor(void)
{
	// TODO: implement
}


void
console_hide_cursor(void)
{
	// TODO: implement
}


void
console_set_color(int32 foreground, int32 background)
{
	sColor = (background & 0xf) << 12 | (foreground & 0xf) << 8;
}


int
console_wait_for_key(void)
{
	int key = virtio_input_wait_for_key();

	switch (key) {
	case 71: return TEXT_CONSOLE_KEY_RETURN;
	case 30: return TEXT_CONSOLE_KEY_BACKSPACE;
	case  1: return TEXT_CONSOLE_KEY_ESCAPE;
	case 94: return TEXT_CONSOLE_KEY_SPACE;

	case 87: return TEXT_CONSOLE_KEY_UP;
	case 98: return TEXT_CONSOLE_KEY_DOWN;
	case 97: return TEXT_CONSOLE_KEY_LEFT;
	case 99: return TEXT_CONSOLE_KEY_RIGHT;
	case 33: return TEXT_CONSOLE_KEY_PAGE_UP;
	case 54: return TEXT_CONSOLE_KEY_PAGE_DOWN;
	case 32: return TEXT_CONSOLE_KEY_HOME;
	case 53: return TEXT_CONSOLE_KEY_END;

	default:
		return TEXT_CONSOLE_NO_KEY;
	}
}


status_t
console_init(void)
{
	sScreenWidth = 80;
	sScreenHeight = 25;
	sScreenOfsX = gFramebuf.width/2 - sScreenWidth*gFixedFont.charWidth/2;
	sScreenOfsY = gFramebuf.height/2 - sScreenHeight*gFixedFont.charHeight/2;

	sScreenBase = new(std::nothrow) uint16[sScreenWidth * sScreenHeight];

	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}
