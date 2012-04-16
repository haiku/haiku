/*
 * Copyright 2010 Wim van der Meer <WPJvanderMeer@gmail.com>
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 *		Christophe Huriaux
 *		Wim van der Meer
 */


#include "Screenshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AppDefs.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Locale.h>
#include <Roster.h>
#include <Screen.h>
#include <TranslatorFormats.h>

#include <WindowInfo.h>

#include "Utility.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screenshot"


Screenshot::Screenshot()
	:
	BApplication("application/x-vnd.haiku-screenshot-cli"),
	fUtility(new Utility()),
	fLaunchGui(true)
{
}


Screenshot::~Screenshot()
{
	delete fUtility;
}


void
Screenshot::ArgvReceived(int32 argc, char** argv)
{
	bigtime_t delay = 0;
	const char* outputFilename = NULL;
	bool includeBorder = false;
	bool includeCursor = false;
	bool grabActiveWindow = false;
	bool saveScreenshotSilent = false;
	bool copyToClipboard = false;
	uint32 imageFileType = B_PNG_FORMAT;
	for (int32 i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			_ShowHelp();
		else if (strcmp(argv[i], "-b") == 0 
			|| strcmp(argv[i], "--border") == 0)
			includeBorder = true;
		else if (strcmp(argv[i], "-m") == 0 
			|| strcmp(argv[i], "--mouse-pointer") == 0)
			includeCursor = true;
		else if (strcmp(argv[i], "-w") == 0 
			|| strcmp(argv[i], "--window") == 0)
			grabActiveWindow = true;
		else if (strcmp(argv[i], "-s") == 0 
			|| strcmp(argv[i], "--silent") == 0)
			saveScreenshotSilent = true;
		else if (strcmp(argv[i], "-f") == 0
			|| strncmp(argv[i], "--format", 6) == 0
			|| strncmp(argv[i], "--format=", 7) == 0)
			imageFileType = _GetImageType(argv[i + 1]);
		else if (strcmp(argv[i], "-d") == 0
			|| strncmp(argv[i], "--delay", 7) == 0
			|| strncmp(argv[i], "--delay=", 8) == 0) {
			int32 seconds = -1;
			if (argc > i + 1)
				seconds = atoi(argv[i + 1]);
			if (seconds >= 0) {
				delay = seconds * 1000000;
				i++;
			} else {
				printf("Screenshot: option requires an argument -- %s\n",
					argv[i]);
				fLaunchGui = false;
				return;
			}
		} else if (strcmp(argv[i], "-c") == 0 
			|| strcmp(argv[i], "--clipboard") == 0)
			copyToClipboard = true;			
		else if (i == argc - 1)
			outputFilename = argv[i];
	}

	_New(delay);

	if (copyToClipboard || saveScreenshotSilent) {
		fLaunchGui = false;
		
		BBitmap* screenshot = fUtility->MakeScreenshot(includeCursor,
			grabActiveWindow, includeBorder);

		if (screenshot == NULL)
			return;

		if (copyToClipboard)
			fUtility->CopyToClipboard(*screenshot);
	
		if (saveScreenshotSilent)
			fUtility->Save(&screenshot, outputFilename, imageFileType);

		delete screenshot;
	}
}


void
Screenshot::ReadyToRun()
{
	if (fLaunchGui) {
		// Get a screenshot if we don't have one
		if (fUtility->wholeScreen == NULL)
			_New(0);

		// Send the screenshot data to the GUI
		BMessage message;
		message.what = SS_UTILITY_DATA;

		BMessage* bitmap = new BMessage();
		fUtility->wholeScreen->Archive(bitmap);
		message.AddMessage("wholeScreen", bitmap);

		bitmap = new BMessage();
		fUtility->cursorBitmap->Archive(bitmap);
		message.AddMessage("cursorBitmap", bitmap);

		bitmap = new BMessage();
		fUtility->cursorAreaBitmap->Archive(bitmap);
		message.AddMessage("cursorAreaBitmap", bitmap);

		message.AddPoint("cursorPosition", fUtility->cursorPosition);
		message.AddRect("activeWindowFrame", fUtility->activeWindowFrame);
		message.AddRect("tabFrame", fUtility->tabFrame);
		message.AddFloat("borderSize", fUtility->borderSize);

		be_roster->Launch("application/x-vnd.haiku-screenshot",	&message);
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
}


void
Screenshot::_ShowHelp()
{
	printf("Screenshot [OPTIONS] [FILE]  Creates a bitmap of the current "
		"screen\n\n");
	printf("FILE is the optional output path / filename used in silent mode. "
		"An exisiting\nfile with the same name will be overwritten without "
		"warning. If FILE is not\ngiven the screenshot will be saved to a "
		"file with the default filename in the\nuser's home directory.\n\n");
	printf("OPTIONS\n");
	printf("  -m, --mouse-pointer   Include the mouse pointer\n");
	printf("  -b, --border          Include the window border\n");
	printf("  -w, --window          Capture the active window instead of the "
		"entire screen\n");
	printf("  -d, --delay=seconds   Take screenshot after the specified delay "
		"[in seconds]\n");
	printf("  -s, --silent          Saves the screenshot without showing the "
		"application\n                        window\n");
	printf("  -f, --format=image	Give the image format you like to save "
		"as\n");
	printf("                        [bmp], [gif], [jpg], [png], [ppm], "
		"[tga], [tif]\n");
	printf("  -c, --clipboard       Copies the screenshot to the system "
		"clipboard without\n                        showing the application "
		"window\n");
	printf("\n");
	printf("Note: OPTION -b, --border takes only effect when used with -w, "
		"--window\n");

	fLaunchGui = false;
}


void
Screenshot::_New(bigtime_t delay)
{
	delete fUtility->wholeScreen;
	delete fUtility->cursorBitmap;
	delete fUtility->cursorAreaBitmap;

	if (delay > 0)
		snooze(delay);

	_GetActiveWindowFrame();

	// There is a bug in the drawEngine code that prevents the drawCursor
	// flag from hiding the cursor when GetBitmap is called, so we need to hide
	// the cursor by ourselves. Refer to trac tickets #2988 and #2997
	bool cursorIsHidden = IsCursorHidden();
	if (!cursorIsHidden)
		HideCursor();
	if (BScreen().GetBitmap(&fUtility->wholeScreen, false) != B_OK)
		return;
	if (!cursorIsHidden)
		ShowCursor();

	// Get the current cursor position, bitmap, and hotspot
	BPoint cursorHotSpot;
	get_mouse(&fUtility->cursorPosition, NULL);
	get_mouse_bitmap(&fUtility->cursorBitmap, &cursorHotSpot);
	fUtility->cursorPosition -= cursorHotSpot;	

	// Put the mouse area in a bitmap
	BRect bounds = fUtility->cursorBitmap->Bounds();
	int cursorWidth = bounds.IntegerWidth() + 1;
	int cursorHeight = bounds.IntegerHeight() + 1;
	fUtility->cursorAreaBitmap = new BBitmap(bounds, B_RGBA32);

	fUtility->cursorAreaBitmap->ImportBits(fUtility->wholeScreen->Bits(),
		fUtility->wholeScreen->BitsLength(),
		fUtility->wholeScreen->BytesPerRow(),
		fUtility->wholeScreen->ColorSpace(),
		fUtility->cursorPosition, BPoint(0, 0),
		cursorWidth, cursorHeight);

	// Fill in the background of the mouse bitmap
	uint8* bits = (uint8*)fUtility->cursorBitmap->Bits();
	uint8* areaBits = (uint8*)fUtility->cursorAreaBitmap->Bits();
	for (int32 i = 0; i < cursorHeight; i++) {
		for (int32 j = 0; j < cursorWidth; j++) {
			uint8 alpha = 255 - bits[3];
			bits[0] = ((areaBits[0] * alpha) >> 8) + bits[0];
			bits[1] = ((areaBits[1] * alpha) >> 8) + bits[1];
			bits[2] = ((areaBits[2] * alpha) >> 8) + bits[2];
			bits[3] = 255;
			areaBits += 4;
			bits += 4;
		}
	}
}


status_t
Screenshot::_GetActiveWindowFrame()
{
	fUtility->activeWindowFrame.Set(0, 0, -1, -1);

	// Create a messenger to communicate with the active application
	app_info appInfo;
	status_t status = be_roster->GetActiveAppInfo(&appInfo);
	if (status != B_OK)
		return status;
	BMessenger messenger(appInfo.signature, appInfo.team);
	if (!messenger.IsValid())
		return B_ERROR;

	// Loop through the windows of the active application to find out which
	// window is active
	int32 tokenCount;
	int32* tokens = get_token_list(appInfo.team, &tokenCount);
	bool foundActiveWindow = false;
	BMessage message;
	BMessage reply;
	int32 index = 0;
	int32 token = -1;
	client_window_info* windowInfo = NULL;
	bool modalWindow = false;
	while (true) {
		message.MakeEmpty();
		reply.MakeEmpty();

		message.what = B_GET_PROPERTY;
		message.AddSpecifier("Active");
		message.AddSpecifier("Window", index);
		messenger.SendMessage(&message, &reply, B_INFINITE_TIMEOUT, 50000);
		
		if (reply.what == B_MESSAGE_NOT_UNDERSTOOD)
			break;

		if (!reply.what) {
			// Reply timout, this probably means that we have a modal window
			// so we'll just get the window frame of the top most window
			modalWindow = true;
			free(tokens);
			status_t status = BPrivate::get_window_order(current_workspace(),
				&tokens, &tokenCount);
			if (status != B_OK || !tokens || tokenCount < 1)
				return B_ERROR;
			foundActiveWindow = true;
		} else if (reply.FindBool("result", &foundActiveWindow) != B_OK)
			foundActiveWindow = false;

		if (foundActiveWindow) {
			// Get the client_window_info of the active window
			foundActiveWindow = false;
			for (int i = 0; i < tokenCount; i++) {
				token = tokens[i];
				windowInfo = get_window_info(token);
				if (windowInfo && !windowInfo->is_mini
						&& !windowInfo->show_hide_level > 0) {
					foundActiveWindow = true;
					break;
				}
				free(windowInfo);
			}
			if (foundActiveWindow)
				break;
		}
		index++;
	}
	free(tokens);

	if (!foundActiveWindow)
		return B_ERROR;

	// Get the TabFrame using the scripting interface
	if (!modalWindow) {
		message.MakeEmpty();
		message.what = B_GET_PROPERTY;
		message.AddSpecifier("TabFrame");
		message.AddSpecifier("Window", index);
		reply.MakeEmpty();
		messenger.SendMessage(&message, &reply);

		if (reply.FindRect("result", &fUtility->tabFrame) != B_OK)
			return B_ERROR;
	} else
		fUtility->tabFrame.Set(0, 0, 0, 0);

	// Get the active window frame from the client_window_info
	fUtility->activeWindowFrame.left = windowInfo->window_left;
	fUtility->activeWindowFrame.top = windowInfo->window_top;
	fUtility->activeWindowFrame.right = windowInfo->window_right;
	fUtility->activeWindowFrame.bottom = windowInfo->window_bottom;
	fUtility->borderSize = windowInfo->border_size;

	free(windowInfo);
	
	// Make sure that fActiveWindowFrame doesn't extend beyond the screen frame
	BRect screenFrame(BScreen().Frame());
	if (fUtility->activeWindowFrame.left < screenFrame.left)
		fUtility->activeWindowFrame.left = screenFrame.left;
	if (fUtility->activeWindowFrame.top < screenFrame.top)
		fUtility->activeWindowFrame.top = screenFrame.top;
	if (fUtility->activeWindowFrame.right > screenFrame.right)
		fUtility->activeWindowFrame.right = screenFrame.right;
	if (fUtility->activeWindowFrame.bottom > screenFrame.bottom)
		fUtility->activeWindowFrame.bottom = screenFrame.bottom;

	return B_OK;
}


int32
Screenshot::_GetImageType(const char* name) const
{
	if (strcmp(name, "bmp") == 0)
		return B_BMP_FORMAT;
	else if (strcmp(name, "gif") == 0)
		return B_GIF_FORMAT;
	else if (strcmp(name, "jpg") == 0 || strcmp(name, "jpeg") == 0)
		return B_JPEG_FORMAT;
	else if (strcmp(name, "ppm") == 0)
		return B_PPM_FORMAT;
	else if (strcmp(name, "tga") == 0 || strcmp(name, "targa") == 0)
		return B_TGA_FORMAT;
	else if (strcmp(name, "tif") == 0 || strcmp(name, "tiff") == 0)
		return B_TIFF_FORMAT;
	else {
		return B_PNG_FORMAT;
	}
}


int
main()
{
	Screenshot screenshot;
	return screenshot.Run();
}
