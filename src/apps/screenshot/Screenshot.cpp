/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 */


#include "Screenshot.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Catalog.h>
#include <Locale.h>


#include <TranslatorFormats.h>


#include "ScreenshotWindow.h"


Screenshot::Screenshot()
	:
	BApplication("application/x-vnd.Haiku-Screenshot"),
	fArgvReceived(false),
	fRefsReceived(false),
	fImageFileType(B_PNG_FORMAT),
	fTranslator(8)
{
	be_locale->GetAppCatalog(&fCatalog);
}


Screenshot::~Screenshot()
{
}


void
Screenshot::ReadyToRun()
{
	if (!fArgvReceived && !fRefsReceived)
		new ScreenshotWindow();

	fArgvReceived = false;
	fRefsReceived = false;
}


void
Screenshot::RefsReceived(BMessage* message)
{
	int32 delay = 0;
	message->FindInt32("delay", &delay);

	bool includeBorder = false;
	message->FindBool("border", &includeBorder);

	bool includeMouse = false;
	message->FindBool("border", &includeMouse);

	bool grabActiveWindow = false;
	message->FindBool("window", &grabActiveWindow);

	bool saveScreenshotSilent = false;
	message->FindBool("silent", &saveScreenshotSilent);

	bool showConfigureWindow = false;
	message->FindBool("configure", &showConfigureWindow);
	
	new ScreenshotWindow(delay * 1000000, includeBorder, includeMouse,
		grabActiveWindow, showConfigureWindow, saveScreenshotSilent);

	fRefsReceived = true;
}


void
Screenshot::ArgvReceived(int32 argc, char** argv)
{
	bigtime_t delay = 0;

	const char* outputFilename = NULL;
	bool includeBorder = false;
	bool includeMouse = false;
	bool grabActiveWindow = false;
	bool showConfigureWindow = false;
	bool saveScreenshotSilent = false;

	for (int32 i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			_ShowHelp();
		else if (strcmp(argv[i], "-b") == 0 
			|| strcmp(argv[i], "--border") == 0)
			includeBorder = true;
		else if (strcmp(argv[i], "-m") == 0 
			|| strcmp(argv[i], "--mouse-pointer") == 0)
			includeMouse = true;
		else if (strcmp(argv[i], "-w") == 0 
			|| strcmp(argv[i], "--window") == 0)
			grabActiveWindow = true;
		else if (strcmp(argv[i], "-s") == 0 
			|| strcmp(argv[i], "--silent") == 0)
			saveScreenshotSilent = true;
		else if (strcmp(argv[i], "-o") == 0 
			|| strcmp(argv[i], "--options") == 0)
			showConfigureWindow = true;
		else if (strcmp(argv[i], "-f") == 0
			|| strncmp(argv[i], "--format", 6) == 0
			|| strncmp(argv[i], "--format=", 7) == 0) {
			_SetImageTypeSilence(argv[i + 1]);
		} else if (strcmp(argv[i], "-d") == 0
			|| strncmp(argv[i], "--delay", 7) == 0
			|| strncmp(argv[i], "--delay=", 8) == 0) {
			int32 seconds = -1;
			if (argc > i + 1)
				seconds = atoi(argv[i + 1]);
			if (seconds >= 0) {
				delay = seconds * 1000000;
				i++;
			} else {
				printf("Screenshot: option requires an argument -- %s\n"
					, argv[i]);
				exit(0);
			}
		} else if (i == argc - 1)
			outputFilename = argv[i];
	}
	
	fArgvReceived = true;
	
	new ScreenshotWindow(delay, includeBorder, includeMouse, grabActiveWindow, 
		showConfigureWindow, saveScreenshotSilent, fImageFileType, 
		fTranslator, outputFilename);
}


void
Screenshot::_ShowHelp() const
{
	printf("Screenshot [OPTIONS] [FILE]  Creates a bitmap of the current screen\n\n");
	printf("FILE is the optional output path / filename used in silent mode. If FILE is not given, a default filename will be generated in the prefered directory.\n\n");
	printf("OPTIONS\n");
	printf("  -o, --options         Show options window first\n");
	printf("  -m, --mouse-pointer   Include the mouse pointer\n");
	printf("  -b, --border          Include the window border\n");
	printf("  -w, --window          Capture the active window instead of the entire screen\n");
	printf("  -d, --delay=seconds   Take screenshot after specified delay [in seconds]\n");
	printf("  -s, --silent          Saves the screenshot without showing the app window\n");
	printf("                        overrides --options\n");
	printf("  -f, --format=image	Write the image format you like to save as\n");
	printf("                        [bmp], [gif], [jpg], [png], [ppm], [targa], [tiff]\n");
	printf("\n");
	printf("Note: OPTION -b, --border takes only effect when used with -w, --window\n");
	exit(0);
}


void
Screenshot::_SetImageTypeSilence(const char* name)
{
	if (strcmp(name, "bmp") == 0) {
		fImageFileType = B_BMP_FORMAT;
		fTranslator = 1;
	} else if (strcmp(name, "gif") == 0) {
		fImageFileType = B_GIF_FORMAT;
		fTranslator = 3;
	} else if (strcmp(name, "jpg") == 0) {
		fImageFileType = B_JPEG_FORMAT;
		fTranslator = 6;
	} else if (strcmp(name, "ppm") == 0) {
		fImageFileType = B_PPM_FORMAT;
		fTranslator = 9;
	} else if (strcmp(name, "targa") == 0) {
		fImageFileType = B_TGA_FORMAT;
		fTranslator = 14;
	} else if (strcmp(name, "tif") == 0) {
		fImageFileType = B_TIFF_FORMAT;
		fTranslator = 15;
	} else { //png
		fImageFileType = B_PNG_FORMAT;
		fTranslator = 8;
	}
}
