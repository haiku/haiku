/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "keyboard.h"
#include "video.h"

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

static uint16 *sScreenBase = (uint16 *)0xb8000;
static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 25;
static uint32 sScreenOffset = 0;
static uint16 sColor = 0x0f00;

static Console sInput, sOutput;
FILE *stdin, *stdout, *stderr;


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
Console::WriteAt(void *cookie, off_t /*pos*/, const void *buffer, size_t bufferSize)
{
	const char *string = (const char *)buffer;

	if (gKernelArgs.frame_buffer.enabled)
		return bufferSize;

	for (uint32 i = 0; i < bufferSize; i++) {
		if (string[0] == '\n')
			sScreenOffset += sScreenWidth - (sScreenOffset % sScreenWidth);
		else
			sScreenBase[sScreenOffset++] = sColor | string[0];

		if (sScreenOffset >= sScreenWidth * sScreenHeight)
			scroll_up();

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
	video_move_text_cursor(x, y);
}


void
console_show_cursor(void)
{
	video_show_text_cursor();
}


void
console_hide_cursor(void)
{
	video_hide_text_cursor();
}


void
console_set_color(int32 foreground, int32 background)
{
	sColor = (background & 0xf) << 12 | (foreground & 0xf) << 8;
}


int
console_wait_for_key(void)
{
	union key key = wait_for_key();

	if (key.code.ascii == 0) {
		switch (key.code.bios) {
			case BIOS_KEY_UP:
				return TEXT_CONSOLE_KEY_UP;
			case BIOS_KEY_DOWN:
				return TEXT_CONSOLE_KEY_DOWN;
			case BIOS_KEY_LEFT:
				return TEXT_CONSOLE_KEY_LEFT;
			case BIOS_KEY_RIGHT:
				return TEXT_CONSOLE_KEY_RIGHT;
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


status_t
console_init(void)
{
	// ToDo: make screen size changeable via stage2_args

	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}

