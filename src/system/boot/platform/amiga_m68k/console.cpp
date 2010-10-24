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

class KeyboardDevice : public ExecDevice {
	public:
		KeyboardDevice();
		virtual ~KeyboardDevice();

		status_t	Open();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		int	WaitForKey();
		status_t	ReadEvent(struct InputEvent *event);

	protected:
};


static const uint16 kPalette[] = {
	0x000,
	0x00a,
	0x0a0,
	0x0aa,
	0xa00,
	0xa0a,
	0xa50,
	0xaaa,
	0x555,
	0x55f,
	0x5f5,
	0x5ff,
	0xf55,
	0xf5f,
	0xff5,
	0xfff
};

static Screen *sScreen;
static int16 sFontWidth, sFontHeight;
static int sScreenTopOffset = 16;
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
			if (fY >= console_height())
				fY = 0;
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


// #pragma mark -


KeyboardDevice::KeyboardDevice()
	: ExecDevice()
{
}


KeyboardDevice::~KeyboardDevice()
{
}


status_t
KeyboardDevice::Open()
{
	return ExecDevice::Open("keyboard.device");
}


status_t
KeyboardDevice::ReadEvent(struct InputEvent *event)
{
	fIOStdReq->io_Command = KBD_READEVENT;
	fIOStdReq->io_Flags = IOF_QUICK;
	fIOStdReq->io_Length = sizeof(struct InputEvent);
	fIOStdReq->io_Data = event;
	status_t err = Do();
	if (err < B_OK)
		return err;
	/*
	dprintf("key: class %d sclass %d code %d 0x%02x qual 0x%04x\n", 
		event->ie_Class, event->ie_SubClass,
		event->ie_Code, event->ie_Code, event->ie_Qualifier);
	*/
	return B_OK;
}


ssize_t
KeyboardDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	struct InputEvent event;
	ssize_t actual;
	status_t err;
	
	do {
		err = ReadEvent(&event);
		if (err < B_OK)
			return err;
	} while (event.ie_Code > IECODE_UP_PREFIX);

	actual = MapRawKey(&event, (char *)buffer, bufferSize, NULL);
	//dprintf("%s actual %d\n", __FUNCTION__, actual);
	if (actual > 0) {
		return actual;
	}
	return B_ERROR;
}


int
KeyboardDevice::WaitForKey()
{
	struct InputEvent event;
	char ascii;
	ssize_t actual;
	status_t err;
	
	do {
		err = ReadEvent(&event);
		if (err < B_OK)
			return err;
	} while (event.ie_Code < IECODE_UP_PREFIX);

	event.ie_Code &= ~IECODE_UP_PREFIX;

	switch (event.ie_Code) {
		case IECODE_KEY_UP:
			return TEXT_CONSOLE_KEY_UP;
		case IECODE_KEY_DOWN:
			return TEXT_CONSOLE_KEY_DOWN;
		case IECODE_KEY_LEFT:
			return TEXT_CONSOLE_KEY_LEFT;
		case IECODE_KEY_RIGHT:
			return TEXT_CONSOLE_KEY_RIGHT;
		case IECODE_KEY_PAGE_UP:
			return TEXT_CONSOLE_KEY_PAGE_UP;
		case IECODE_KEY_PAGE_DOWN:
			return TEXT_CONSOLE_KEY_PAGE_DOWN;
		default:
			break;
	}
	
	actual = MapRawKey(&event, &ascii, 1, NULL);
	//dprintf("%s actual %d\n", __FUNCTION__, actual);
	if (actual > 0)
		return ascii;
	return TEXT_CONSOLE_NO_KEY;
}


ssize_t
KeyboardDevice::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


static KeyboardDevice sInput;


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
		4,
		BLACK, WHITE,
		0x8000,
		0x11,
		NULL,
		"Haiku Loader",
		NULL,
		NULL
	};
	
	sScreen = OpenScreen(&newScreen);
	if (sScreen == NULL)
		panic("OpenScreen()\n");
	
	LoadRGB4(&sScreen->ViewPort, kPalette, 16);
	
	SetDrMd(&sScreen->RastPort, JAM2);
	
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
	
	sScreenTopOffset = sScreen->BarHeight * 2; // ???

	
	//ClearScreen(&sScreen->RastPort);

	dbgerr = stdout = stderr = (FILE *)&sOutput;

	console_set_cursor(0, 0);
	
	/*
	dprintf("LeftEdge %d\n", sScreen->LeftEdge);
	dprintf("TopEdge %d\n", sScreen->TopEdge);
	dprintf("Width %d\n", sScreen->Width);
	dprintf("Height %d\n", sScreen->Height);
	dprintf("MouseX %d\n", sScreen->MouseX);
	dprintf("MouseY %d\n", sScreen->MouseY);
	dprintf("Flags 0x%08x\n", sScreen->Flags);
	dprintf("BarHeight %d\n", sScreen->BarHeight);
	dprintf("BarVBorder %d\n", sScreen->BarVBorder);
	dprintf("BarHBorder %d\n", sScreen->BarHBorder);
	dprintf("MenuVBorder %d\n", sScreen->MenuVBorder);
	dprintf("MenuHBorder %d\n", sScreen->MenuHBorder);
	dprintf("WBorTop %d\n", sScreen->WBorTop);
	dprintf("WBorLeft %d\n", sScreen->WBorLeft);
	dprintf("WBorRight %d\n", sScreen->WBorRight);
	dprintf("WBorBottom %d\n", sScreen->WBorBottom);
	*/

	KEYMAP_BASE_NAME = (Library *)OldOpenLibrary(KEYMAPNAME);
	if (KEYMAP_BASE_NAME == NULL)
		panic("Cannot open %s", KEYMAPNAME);

	sInput.AllocRequest(sizeof(struct IOStdReq));
	sInput.Open();
	stdin = (FILE *)&sInput;

	

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
	int columnCount = sScreen->Width / sFontWidth;
	return columnCount;
}


int32
console_height(void)
{
	int lineCount = (sScreen->Height - sScreenTopOffset) / sFontHeight;
	return lineCount;
}


void
console_set_cursor(int32 x, int32 y)
{
	Move(&sScreen->RastPort, sFontWidth * x,
		sFontHeight * y + sScreenTopOffset);
		// why do I have to add this to keep the title ?
	
}


void
console_set_color(int32 foreground, int32 background)
{
	SetAPen(&sScreen->RastPort, foreground);
	SetBPen(&sScreen->RastPort, background);
}


int
console_wait_for_key(void)
{
	int key = sInput.WaitForKey();
	//dprintf("k: %08x '%c'\n", key, key);
	return key;
}

