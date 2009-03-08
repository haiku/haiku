/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Screen.h>

#include "ScreenMode.h"


static struct option const kLongOptions[] = {
	{"fall-back", no_argument, 0, 'f'},
	{"short", no_argument, 0, 's'},
	{"list", no_argument, 0, 'l'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *kProgramName = __progname;


static color_space
color_space_for_depth(int32 depth)
{
	switch (depth) {
		case 8:
			return B_CMAP8;
		case 15:
			return B_RGB15;
		case 16:
			return B_RGB16;
		case 24:
			return B_RGB24;
		case 32:
		default:
			return B_RGB32;
	}
}


static void
usage(int status)
{
	fprintf(stderr,
		"Usage: %s [options] <mode>\n"
		"Sets the specified screen mode. When no screen mode has been chosen,\n"
		"the current one is printed. <mode> takes the form: <width> <height>\n"
		"<depth> <refresh-rate>, or <width>x<height>, etc.\n"
		"      --fall-back\tchanges to the standard fallback mode, and displays a\n"
		"\t\t\tnotification requester.\n"
		"  -s  --short\t\twhen no mode is given the current screen mode is\n"
		"\t\t\tprinted in short form.\n"
		"  -l  --list\t\tdisplay a list of the available modes\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char** argv)
{
	bool fallbackMode = false;
	bool setMode = false;
	bool shortOutput = false;
	bool listModes = false;
	int width = -1;
	int height = -1;
	int depth = -1;
	float refresh = -1;

	// TODO: add a possibility to set a virtual screen size in addition to
	// the display resolution!

	int c;
	while ((c = getopt_long(argc, argv, "shlf", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'f':
				fallbackMode = true;
				setMode = true;
				break;
			case 's':
				shortOutput = true;
				break;
			case 'l':
				listModes = true;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind > 0) {
		int depthIndex = -1;

		// arguments to specify the mode are following
		int parsed = sscanf(argv[optind], "%dx%dx%d", &width, &height, &depth);
		if (parsed == 2)
			depthIndex = optind + 1;
		else if (parsed == 1) {
			if (argc - optind > 1) {
				height = strtol(argv[optind + 1], NULL, 0);
				depthIndex = optind + 2;
			} else
				usage(1);
		} else if (parsed != 3)
			usage(1);

		if (depthIndex > 0 && depthIndex < argc)
			depth = strtol(argv[depthIndex], NULL, 0);
		if (depthIndex + 1 < argc)
			refresh = strtod(argv[depthIndex + 1], NULL);

		setMode = true;
	}

	BApplication application("application/x-vnd.Haiku-screenmode");

	ScreenMode screenMode(NULL);
	screen_mode currentMode;
	screenMode.Get(currentMode);

	if ((!setMode) && (!listModes)) {
		const char* format = shortOutput
			? "%ld %ld %ld %g\n" : "Resolution: %ld %ld, %ld bits, %g Hz\n";
		printf(format, currentMode.width, currentMode.height,
			currentMode.BitsPerPixel(), currentMode.refresh);
		return 0;
	}

	screen_mode newMode = currentMode;

        if (listModes) {
		const int modeCount = screenMode.CountModes();
		printf("Available screen modes :\n");

		for (int modeNumber = 0; modeNumber < modeCount; modeNumber++) {
			currentMode = screenMode.ModeAt(modeNumber);
			const char* format = shortOutput
				? "%ld %ld %ld %g\n" : "%ld %ld, %ld bits, %g Hz\n";
			printf(format, currentMode.width, currentMode.height,
				currentMode.BitsPerPixel(), currentMode.refresh);
		}
		return 0;
        } else if (fallbackMode) {
		if (currentMode.width == 800 && currentMode.height == 600) {
			newMode.width = 640;
			newMode.height = 480;
			newMode.space = B_CMAP8;
			newMode.refresh = 60;
		} else {
			newMode.width = 800;
			newMode.height = 600;
			newMode.space = B_RGB16;
			newMode.refresh = 60;
		}
	} else {
		newMode.width = width;
		newMode.height = height;

		if (depth != -1)
			newMode.space = color_space_for_depth(depth);
		else
			newMode.space = B_RGB32;

		if (refresh > 0)
			newMode.refresh = refresh;
		else
			newMode.refresh = 60;
	}

	status_t status = screenMode.Set(newMode);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not set screen mode %ldx%ldx%ldx: %s\n",
			kProgramName, newMode.width, newMode.height, newMode.BitsPerPixel(),
			strerror(status));
		return 1;
	}

	if (fallbackMode) {
		// display notification requester
		BAlert* alert = new BAlert("screenmode",
			"You have used the shortcut <Command><Ctrl><Escape> to reset the "
			"screen mode to a safe fallback.", "Keep", "Revert");
		if (alert->Go() == 1)
			screenMode.Revert();
	}

	return 0;
}
