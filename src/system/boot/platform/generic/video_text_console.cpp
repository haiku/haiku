/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <boot/stage2.h>
#include <boot/platform/generic/text_console.h>
#include <boot/platform/generic/video.h>

#include <frame_buffer_console.h>


extern console_module_info gFrameBufferConsoleModule;


class VideoTextConsole : public ConsoleNode {
	public:
		VideoTextConsole();

		void Init(addr_t framebuffer);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);

		virtual void	ClearScreen();
		virtual int32	Width();
		virtual int32	Height();
		virtual void	SetCursor(int32 x, int32 y);
		virtual void	SetCursorVisible(bool visible);
		virtual void	SetColors(int32 foreground, int32 background);

	private:
		uint16 fColor;
		bool fShowCursor;
		int32 fCursorX, fCursorY;
		int32 fScreenWidth, fScreenHeight;
};

static VideoTextConsole sVideoTextConsole;
static bool sVideoTextConsoleInitialized;


//	#pragma mark -


VideoTextConsole::VideoTextConsole()
	: ConsoleNode()
{
}


void
VideoTextConsole::Init(addr_t framebuffer)
{
	frame_buffer_update(framebuffer, gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth,
		gKernelArgs.frame_buffer.bytes_per_row);
	gFrameBufferConsoleModule.get_size(&fScreenWidth, &fScreenHeight);

	SetCursorVisible(false);
	SetCursor(0, 0);
	SetColors(15, 0);
	ClearScreen();
}


ssize_t
VideoTextConsole::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t
VideoTextConsole::WriteAt(void *cookie, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;
	for (size_t i = 0; i < bufferSize; i++) {
		const char c = string[i];
		if (c == '\n' || fCursorX >= fScreenWidth) {
			fCursorX = 0;
			if (fCursorY == (fScreenHeight - 1)) {
				// Move the screen up and clear the bottom line.
				gFrameBufferConsoleModule.blit(0, 1, fScreenWidth, 0,
					0, fScreenHeight - 1);
				gFrameBufferConsoleModule.fill_glyph(0, fCursorY, fScreenWidth,
					1, ' ', fColor);
			} else {
				fCursorY++;
			}
		}

		switch (c) {
			case '\n':
				// already handled above
				break;

			default:
				gFrameBufferConsoleModule.put_glyph(fCursorX, fCursorY, c, fColor);
				break;
		}
		fCursorX++;
	}
	return bufferSize;
}


void
VideoTextConsole::ClearScreen()
{
	gFrameBufferConsoleModule.clear(fColor);
}


int32
VideoTextConsole::Width()
{
	return fScreenWidth;
}


int32
VideoTextConsole::Height()
{
	return fScreenHeight;
}


void
VideoTextConsole::SetCursor(int32 x, int32 y)
{
	fCursorX = x;
	fCursorY = y;
	if (fShowCursor)
		SetCursorVisible(true);
}


void
VideoTextConsole::SetCursorVisible(bool visible)
{
	fShowCursor = visible;
	if (fShowCursor)
		gFrameBufferConsoleModule.move_cursor(fCursorX, fCursorY);
	else
		gFrameBufferConsoleModule.move_cursor(-1, -1);
}


void
VideoTextConsole::SetColors(int32 foreground, int32 background)
{
	fColor = (background & 0xf) << 4 | (foreground & 0xf);
}


ConsoleNode*
video_text_console_init(addr_t frameBuffer)
{
	if (!sVideoTextConsoleInitialized) {
		new(&sVideoTextConsole) VideoTextConsole;
		sVideoTextConsoleInitialized = true;
	}

	sVideoTextConsole.Init(frameBuffer);
	return &sVideoTextConsole;
}
