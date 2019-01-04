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
#include <util/kernel_cpp.h>

#include "efi_platform.h"


class Console : public ConsoleNode {
	public:
		Console();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize);
};


static uint32 sScreenWidth;
static uint32 sScreenHeight;
static uint32 sScreenMode;
static Console sInput, sOutput;
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
	CHAR16 ucsBuffer[bufferSize + 3];
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
				ucsBuffer[j++] = (CHAR16) string[i];
		}
	}

	if (j > 0) {
		ucsBuffer[j] = 0;
		kSystemTable->ConOut->OutputString(kSystemTable->ConOut, ucsBuffer);
	}
	return bufferSize;
}


//	#pragma mark -


void
console_clear_screen(void)
{
	kSystemTable->ConOut->ClearScreen(kSystemTable->ConOut);
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
	kSystemTable->ConOut->SetCursorPosition(kSystemTable->ConOut, x, y);
}


void
console_show_cursor(void)
{
	kSystemTable->ConOut->EnableCursor(kSystemTable->ConOut, true);
}


void
console_hide_cursor(void)
{
	kSystemTable->ConOut->EnableCursor(kSystemTable->ConOut, false);
}


void
console_set_color(int32 foreground, int32 background)
{
	kSystemTable->ConOut->SetAttribute(kSystemTable->ConOut,
		EFI_TEXT_ATTR((foreground & 0xf), (background & 0xf)));
}


int
console_wait_for_key(void)
{
	UINTN index;
	EFI_STATUS status;
	EFI_INPUT_KEY key;
	EFI_EVENT event = kSystemTable->ConIn->WaitForKey;

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
	UINTN width, height;
	UINTN area = 0;
	SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut = kSystemTable->ConOut;

	for (int mode = 0; mode < ConOut->Mode->MaxMode; ++mode) {
		if (ConOut->QueryMode(ConOut, mode, &width, &height) == EFI_SUCCESS) {
			if (width * height > area) {
				sScreenWidth = width;
				sScreenHeight = height;
				sScreenMode = mode;
			}
		}
	}

	ConOut->SetMode(ConOut, sScreenMode);
}


status_t
console_init(void)
{
	update_screen_size();
	console_hide_cursor();
	console_clear_screen();

	// enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


uint32
console_check_boot_keys(void)
{
	EFI_STATUS status;
	EFI_INPUT_KEY key;

	// give the user a chance to press a key
	kBootServices->Stall(500000);

	status = kSystemTable->ConIn->ReadKeyStroke(kSystemTable->ConIn, &key);

	if (status != EFI_SUCCESS)
		return 0;

	if (key.UnicodeChar == 0 && key.ScanCode == SCAN_ESC)
		return BOOT_OPTION_DEBUG_OUTPUT;
	if (key.UnicodeChar == ' ')
		return BOOT_OPTION_MENU;

	return 0;
}


extern "C" void
platform_switch_to_text_mode(void)
{
	kSystemTable->ConOut->Reset(kSystemTable->ConOut, false);
	kSystemTable->ConOut->SetMode(kSystemTable->ConOut, sScreenMode);
}
