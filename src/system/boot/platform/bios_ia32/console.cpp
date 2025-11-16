/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "keyboard.h"
#include "video.h"

#include <SupportDefs.h>

#include <boot/stage2.h>
#include <utf8_functions.h>
#include <util/kernel_cpp.h>

#include <string.h>


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual void	ClearScreen();
		virtual int32	Width();
		virtual int32	Height();
		virtual void	SetCursor(int32 x, int32 y);
		virtual void	SetCursorVisible(bool visible);
		virtual void	SetColors(int32 foreground, int32 background);
};

static uint16 *sScreenBase = (uint16 *)0xb8000;
static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 25;
static uint32 sScreenOffset = 0;
static uint16 sColor = 0x0f00;

extern ConsoleNode* gConsoleNode;
static Console sConsole;
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

	size_t length = bufferSize;
	while (length > 0) {
		if (string[0] == '\n') {
			sScreenOffset += sScreenWidth - (sScreenOffset % sScreenWidth);
			length--;
			string++;
		} else if ((string[0] & 0x80) == 0) {
			sScreenBase[sScreenOffset++] = sColor | string[0];
			length--;
			string++;
		} else {
			// UTF8ToCharCode expects a NULL-terminated string, which we can't
			// guarantee. UTF8NextCharLen checks the bounds and allows us to
			// know whether we can safely read the next character.
			uint32 codepoint;
			uint32 charLen = UTF8NextCharLen(string, length);
			if (charLen > 0) {
				codepoint = UTF8ToCharCode(&string);
				length -= charLen;
			} else {
				codepoint = 0xfffd;
				string++;
				length--;
			}

			switch (codepoint) {
				case 0x25B2:	// BLACK UP-POINTING TRIANGLE
					sScreenBase[sScreenOffset++] = sColor | 0x1e;
					break;
				case 0x25BC:	// BLACK DOWN-POINTING TRIANGLE
					sScreenBase[sScreenOffset++] = sColor | 0x1f;
					break;
				case 0x2026:	// HORIZONTAL ELLIPSIS
					WriteAt(cookie, -1, "...", 3);
					break;
				case 0x00A9:	// COPYRIGHT SIGN
					WriteAt(cookie, -1, "(C)", 3);
					break;
				default:
					sScreenBase[sScreenOffset++] = sColor | '?';
			}
		}

		if (sScreenOffset >= sScreenWidth * sScreenHeight)
			scroll_up();
	}
	return bufferSize;
}


void
Console::ClearScreen()
{
	if (gKernelArgs.frame_buffer.enabled)
		return;

	for (uint32 i = 0; i < sScreenWidth * sScreenHeight; i++)
		sScreenBase[i] = sColor;

	// reset cursor position as well
	sScreenOffset = 0;
}


int32
Console::Width()
{
	return sScreenWidth;
}


int32
Console::Height()
{
	return sScreenHeight;
}


void
Console::SetCursor(int32 x, int32 y)
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
Console::SetCursorVisible(bool visible)
{
	if (visible)
		video_show_text_cursor();
	else
		video_hide_text_cursor();
}


void
Console::SetColors(int32 foreground, int32 background)
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

	gConsoleNode = &sConsole;

	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sConsole;
	stdout = stderr = (FILE *)&sConsole;

	return B_OK;
}

