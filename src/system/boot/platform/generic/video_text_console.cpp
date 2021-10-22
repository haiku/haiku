/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <boot/stage2.h>
#include <boot/platform/generic/text_console.h>
#include <boot/platform/generic/video.h>

#include <frame_buffer_console.h>


extern console_module_info gFrameBufferConsoleModule;


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);
};


static uint16 sColor = 0x0f00;
static bool sShowCursor;
static int32 sCursorX, sCursorY;
static int32 sScreenWidth, sScreenHeight;

static Console sConsole;
FILE *stdin, *stdout, *stderr;


//	#pragma mark -


Console::Console()
	: ConsoleNode()
{
}


ssize_t
Console::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t
Console::WriteAt(void *cookie, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;
	for (size_t i = 0; i < bufferSize; i++) {
		const char c = string[i];
		if (c == '\n' || sCursorX >= sScreenWidth) {
			sCursorX = 0;
			if (sCursorY == (sScreenHeight - 1)) {
				// Move the screen up and clear the bottom line.
				gFrameBufferConsoleModule.blit(0, 1, sScreenWidth, 0,
					0, sScreenHeight - 1);
				gFrameBufferConsoleModule.fill_glyph(0, sCursorY, sScreenWidth,
					1, ' ', sColor);
			} else {
				sCursorY++;
			}
		}

		switch (c) {
			case '\n':
				// already handled above
				break;

			default:
				gFrameBufferConsoleModule.put_glyph(sCursorX, sCursorY, c, sColor);
				break;
		}
		sCursorX++;
	}
	return bufferSize;
}


//	#pragma mark -


void
console_clear_screen(void)
{
	gFrameBufferConsoleModule.clear(sColor);
}


int32
console_width(void)
{
	return sScreenWidth;
}


int32
console_height()
{
	return sScreenHeight;
}


void
console_set_cursor(int32 x, int32 y)
{
	sCursorX = x;
	sCursorY = y;
	if (sShowCursor)
		console_show_cursor();
}


void
console_show_cursor()
{
	sShowCursor = true;
	gFrameBufferConsoleModule.move_cursor(sCursorX, sCursorY);
}


void
console_hide_cursor()
{
	sShowCursor = false;
	gFrameBufferConsoleModule.move_cursor(-1, -1);
}


void
console_set_color(int32 foreground, int32 background)
{
	sColor = (background & 0xf) << 4 | (foreground & 0xf);
}


status_t
video_text_console_init(addr_t frameBuffer)
{
	frame_buffer_update(frameBuffer, gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth,
		gKernelArgs.frame_buffer.bytes_per_row);
	gFrameBufferConsoleModule.get_size(&sScreenWidth, &sScreenHeight);

	console_hide_cursor();
	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sConsole;
	stdout = stderr = (FILE *)&sConsole;

	return B_OK;
}
