/*
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
 * Copyright 2013 Fredrik Holmqvist, fredrik.holmqvist@gmail.com. All rights
 * reserved.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"

#include <string.h>

#include <SupportDefs.h>

#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/platform/generic/video.h>
#include <efi/protocol/console-control.h>
#include <util/kernel_cpp.h>

#include "efi_platform.h"


// This likely won't work without moving things around.
// Too early (pre-console init)
//#define TRACE_CONSOLE
#ifdef TRACE_CONSOLE
#   define TRACE(x...) dprintf(x)
#else
#   define TRACE(x...)
#endif


class EFITextConsole : public ConsoleNode {
	public:
		EFITextConsole();

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

	public:
		uint32 fScreenWidth, fScreenHeight;
};


extern ConsoleNode* gConsoleNode;
static uint32 sScreenMode;
static EFITextConsole sConsole;
FILE *stdin, *stdout, *stderr;


//	#pragma mark -


EFITextConsole::EFITextConsole()
	: ConsoleNode()
{
}


ssize_t
EFITextConsole::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t
EFITextConsole::WriteAt(void *cookie, off_t /*pos*/, const void *buffer,
	size_t bufferSize)
{
	const char *string = (const char *)buffer;
	char16_t ucsBuffer[bufferSize + 3];
	uint32 j = 0;

	for (uint32 i = 0; i < bufferSize; i++) {
		switch (string[i]) {
			case '\n': {
				ucsBuffer[j++] = '\r';
				ucsBuffer[j++] = '\n';
			} //fallthrough
			case 0 : {
				//Not sure if we should keep going or abort for 0.
				//Keep going was easy anyway.
				ucsBuffer[j] = 0;
				kSystemTable->ConOut->OutputString(kSystemTable->ConOut,
					ucsBuffer);
				j = 0;
				continue;
			}
			default:
				ucsBuffer[j++] = (char16_t)string[i];
		}
	}

	if (j > 0) {
		ucsBuffer[j] = 0;
		kSystemTable->ConOut->OutputString(kSystemTable->ConOut, ucsBuffer);
	}
	return bufferSize;
}


void
EFITextConsole::ClearScreen()
{
	kSystemTable->ConOut->ClearScreen(kSystemTable->ConOut);
}


int32
EFITextConsole::Width()
{
	return fScreenWidth;
}


int32
EFITextConsole::Height()
{
	return fScreenHeight;
}


void
EFITextConsole::SetCursor(int32 x, int32 y)
{
	kSystemTable->ConOut->SetCursorPosition(kSystemTable->ConOut, x, y);
}


void
EFITextConsole::SetCursorVisible(bool visible)
{
	kSystemTable->ConOut->EnableCursor(kSystemTable->ConOut, visible);
}


void
EFITextConsole::SetColors(int32 foreground, int32 background)
{
	kSystemTable->ConOut->SetAttribute(kSystemTable->ConOut,
		EFI_TEXT_ATTR((foreground & 0xf), (background & 0xf)));
}


int
console_wait_for_key(void)
{
	size_t index;
	efi_status status;
	efi_input_key key;
	efi_event event = kSystemTable->ConIn->WaitForKey;

	do {
		kBootServices->WaitForEvent(1, &event, &index);
		status = kSystemTable->ConIn->ReadKeyStroke(kSystemTable->ConIn, &key);
	} while (status == EFI_NOT_READY);

	if (key.UnicodeChar > 0)
		return (int) key.UnicodeChar;

	switch (key.ScanCode) {
		case SCAN_ESC:
			return TEXT_CONSOLE_KEY_ESCAPE;
		case SCAN_UP:
			return TEXT_CONSOLE_KEY_UP;
		case SCAN_DOWN:
			return TEXT_CONSOLE_KEY_DOWN;
		case SCAN_LEFT:
			return TEXT_CONSOLE_KEY_LEFT;
		case SCAN_RIGHT:
			return TEXT_CONSOLE_KEY_RIGHT;
		case SCAN_PAGE_UP:
			return TEXT_CONSOLE_KEY_PAGE_UP;
		case SCAN_PAGE_DOWN:
			return TEXT_CONSOLE_KEY_PAGE_DOWN;
		case SCAN_HOME:
			return TEXT_CONSOLE_KEY_HOME;
		case SCAN_END:
			return TEXT_CONSOLE_KEY_END;
	}
	return 0;
}


static void update_screen_size(void)
{
	size_t width, height;
	size_t area = 0;
	efi_simple_text_output_protocol *ConOut = kSystemTable->ConOut;

	for (int mode = 0; mode < ConOut->Mode->MaxMode; ++mode) {
		if (ConOut->QueryMode(ConOut, mode, &width, &height) == EFI_SUCCESS) {
			if (width * height > area) {
				sConsole.fScreenWidth = width;
				sConsole.fScreenHeight = height;
				sScreenMode = mode;
			}
		}
	}

	ConOut->SetMode(ConOut, sScreenMode);
}


status_t
console_init(void)
{
#if 1
	gConsoleNode = &sConsole;

	update_screen_size();
	console_hide_cursor();
	console_clear_screen();
#else
	// FIXME: This does not work because we cannot initialize video before VFS, as it
	// needs to read the driver settings before setting a mode; and also because the
	// heap does not yet exist.
	platform_init_video();
	platform_switch_to_logo();
	gConsoleNode = video_text_console_init(gKernelArgs.frame_buffer.physical_buffer.start);
#endif

	// enable stdio functionality
	stdin = (FILE *)gConsoleNode;
	stdout = stderr = (FILE *)gConsoleNode;

	return B_OK;
}


uint32
console_check_boot_keys(void)
{
	efi_input_key key;

	for (int i = 0; i < 3; i++) {
		// give the user a chance to press a key
		kBootServices->Stall(100000);

		efi_status status = kSystemTable->ConIn->ReadKeyStroke(
			kSystemTable->ConIn, &key);

		if (status != EFI_SUCCESS)
			continue;

		if (key.UnicodeChar == 0 && key.ScanCode == SCAN_ESC)
			return BOOT_OPTION_DEBUG_OUTPUT;
		if (key.UnicodeChar == ' ')
			return BOOT_OPTION_MENU;
	}
	return 0;
}


extern "C" void
platform_switch_to_text_mode(void)
{
	kSystemTable->ConOut->Reset(kSystemTable->ConOut, false);
	kSystemTable->ConOut->SetMode(kSystemTable->ConOut, sScreenMode);
}
