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
				sScreenWidth = width;
				sScreenHeight = height;
				sScreenMode = mode;
			}
		}
	}

	ConOut->SetMode(ConOut, sScreenMode);
}


static void
console_control(bool graphics)
{
	TRACE("Checking for EFI Console Control...\n");
	efi_guid consoleControlProtocolGUID = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
	efi_console_control_protocol* consoleControl = NULL;

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&consoleControlProtocolGUID, NULL, (void**)&consoleControl);

	// Some EFI implementations boot up in an EFI graphics mode (Apple)
	// If this protocol doesn't exist, we can assume we're already in text mode.
	if (status != EFI_SUCCESS || consoleControl == NULL) {
		TRACE("EFI Console Control not found. Skipping.\n");
		return;
	}

	TRACE("Located EFI Console Control. Setting EFI %s mode...\n",
		graphics ? "graphics" : "text");

	if (graphics) {
		status = consoleControl->SetMode(consoleControl,
			EfiConsoleControlScreenGraphics);
	} else {
		status = consoleControl->SetMode(consoleControl,
			EfiConsoleControlScreenText);
	}

	TRACE("Setting EFI %s mode was%s successful.\n",
		graphics ? "graphics" : "text", (status == EFI_SUCCESS) ? "" : " not");
}


status_t
console_init(void)
{
	console_control(true);
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
	efi_status status;
	efi_input_key key;

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
	console_control(false);
	kSystemTable->ConOut->Reset(kSystemTable->ConOut, false);
	kSystemTable->ConOut->SetMode(kSystemTable->ConOut, sScreenMode);
}
