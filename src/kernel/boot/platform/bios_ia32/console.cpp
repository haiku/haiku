/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>
#include <string.h>
#include <util/kernel_cpp.h>

#include "console.h"


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);
};

static uint16 *sScreenBase = (uint16 *)0xb8000;
static uint32 sScreenWidth = 80;
static uint32 sScreenHeight = 24;
static uint32 sScreenOffset = 0;

static Console sInput, sOutput;
FILE *stdin, *stdout, *stderr;


static void
clear_screen()
{
	for (uint32 i = 0; i < sScreenWidth * sScreenHeight; i++)
		sScreenBase[i] = 0xf20;
}


static void
scroll_up()
{
	memcpy(sScreenBase, sScreenBase + sScreenWidth,
		sScreenWidth * sScreenHeight * 2 - sScreenWidth * 2);
	sScreenOffset = (sScreenHeight - 1) * sScreenWidth;

	for (uint32 i = 0; i < sScreenWidth; i++)
		sScreenBase[sScreenOffset + i] = 0x0720;
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

	for (uint32 i = 0; i < bufferSize; i++) {
		if (string[0] == '\n')
			sScreenOffset += sScreenWidth - (sScreenOffset % sScreenWidth);
		else
			sScreenBase[sScreenOffset++] = 0xf00 | string[0];

		if (sScreenOffset >= sScreenWidth * sScreenHeight)
			scroll_up();

		string++;
	}
	return bufferSize;
}


//	#pragma mark -


status_t
console_init(void)
{
	// ToDo: make screen size changeable via stage2_args

	clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


