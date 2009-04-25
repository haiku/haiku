/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "Screenshot.h"
#include "ScreenshotWindow.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Screenshot::Screenshot()
	: BApplication("application/x-vnd.haiku-screenshot"),
	fArgvReceived(false),
	fRefsReceived(false)
{
}


Screenshot::~Screenshot()
{
}


void
Screenshot::ReadyToRun()
{
	if(!fArgvReceived && !fRefsReceived)
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

	bool includeBorder = false;
	bool includeMouse = false;
	bool grabActiveWindow = false;
	bool showConfigureWindow = false;
	bool saveScreenshotSilent = false;

	for (int32 i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			_ShowHelp();
		else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--border") == 0)
			includeBorder = true;
		else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mouse") == 0)
			includeMouse = true;
		else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--window") == 0)
			grabActiveWindow = true;
		else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0)
			saveScreenshotSilent = true;
		else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--options") == 0)
			showConfigureWindow = true;
		else if (strcmp(argv[i], "-d") == 0
			|| strncmp(argv[i], "--delay", 7) == 0
			|| strncmp(argv[i], "--delay=", 8) == 0) {
			int32 seconds = atoi(argv[i + 1]);
			if (seconds > 0) {
				delay = seconds * 1000000;
				i++;
			}
		}
	}
	fArgvReceived = true;
	new ScreenshotWindow(delay, includeBorder, includeMouse, grabActiveWindow,
		showConfigureWindow, saveScreenshotSilent);
}


void
Screenshot::_ShowHelp() const
{
	printf("Screenshot [OPTION]... Creates a bitmap of the current screen\n\n");
	printf("OPTION\n");
	printf("  -o, --options         Show options window first\n");
	printf("  -m, --mouse           Have the mouse pointer in the screenshot\n");
	printf("  -b, --border          Include the window border with the screenshot\n");
	printf("  -w, --window          Use active window instead of the entire screen\n");
	printf("  -d, --delay=seconds   Take screenshot after specified delay [in seconds]\n");
	printf("  -s, --silent          Saves the screenshot without showing the app window\n");
	printf("                        overrides --options, saves to home folder as png\n");
	printf("\n");
	printf("Note: OPTION -b, --border takes only effect when used with -w, --window\n");

	exit(0);
}
