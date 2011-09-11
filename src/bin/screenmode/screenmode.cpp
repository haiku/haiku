/*
 * Copyright 2008-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
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
	{"dont-confirm", no_argument, 0, 'q'},
	{"modeline", no_argument, 0, 'm'},
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
print_mode(const screen_mode& mode, bool shortOutput)
{
	const char* format
		= shortOutput ? "%ld %ld %ld %g\n" : "%ld %ld, %ld bits, %g Hz\n";
	printf(format, mode.width, mode.height, mode.BitsPerPixel(), mode.refresh);
}


static void
print_mode(const display_mode& displayMode, const screen_mode& mode)
{
	const display_timing& timing = displayMode.timing;

	printf("%lu  %u %u %u %u  %u %u %u %u ", timing.pixel_clock / 1000,
		timing.h_display, timing.h_sync_start, timing.h_sync_end,
		timing.h_total, timing.v_display, timing.v_sync_start,
		timing.v_sync_end, timing.v_total);

	// TODO: more flags?
	if ((timing.flags & B_POSITIVE_HSYNC) != 0)
		printf(" +HSync");
	if ((timing.flags & B_POSITIVE_VSYNC) != 0)
		printf(" +VSync");
	if ((timing.flags & B_TIMING_INTERLACED) != 0)
		printf(" Interlace");
	printf(" %lu\n", mode.BitsPerPixel());
}


static void
usage(int status)
{
	fprintf(stderr,
		"Usage: %s [options] <mode>\n"
		"Sets the specified screen mode. When no screen mode has been chosen,\n"
		"the current one is printed. <mode> takes the form: <width> <height>\n"
		"<depth> <refresh-rate>, or <width>x<height>, etc.\n"
		"      --fall-back\tchanges to the standard fallback mode, and "
			"displays a\n"
		"\t\t\tnotification requester.\n"
		"  -s  --short\t\twhen no mode is given the current screen mode is\n"
			"\t\t\tprinted in short form.\n"
		"  -l  --list\t\tdisplay a list of the available modes.\n"
		"  -q  --dont-confirm\tdo not confirm the mode after setting it.\n"
		"  -m  --modeline\taccept and print X-style modeline modes:\n"
		"\t\t\t  <pclk> <h-display> <h-sync-start> <h-sync-end> <h-total>\n"
		"\t\t\t  <v-disp> <v-sync-start> <v-sync-end> <v-total> [flags] "
			"[depth]\n"
		"\t\t\t(supported flags are: +/-HSync, +/-VSync, Interlace)\n",
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
	bool modeLine = false;
	bool confirm = true;
	int width = -1;
	int height = -1;
	int depth = -1;
	float refresh = -1;
	display_mode mode;

	// TODO: add a possibility to set a virtual screen size in addition to
	// the display resolution!

	int c;
	while ((c = getopt_long(argc, argv, "shlfqm", kLongOptions, NULL)) != -1) {
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
			case 'm':
				modeLine = true;
				break;
			case 'q':
				confirm = false;
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

		if (!modeLine) {
			int parsed = sscanf(argv[optind], "%dx%dx%d", &width, &height,
				&depth);
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
		} else {
			// parse mode line
			if (argc - optind < 9)
				usage(1);

			mode.timing.pixel_clock = strtol(argv[optind], NULL, 0) * 1000;
			mode.timing.h_display = strtol(argv[optind + 1], NULL, 0);
			mode.timing.h_sync_start = strtol(argv[optind + 2], NULL, 0);
			mode.timing.h_sync_end = strtol(argv[optind + 3], NULL, 0);
			mode.timing.h_total = strtol(argv[optind + 4], NULL, 0);
			mode.timing.h_display = strtol(argv[optind + 5], NULL, 0);
			mode.timing.h_sync_start = strtol(argv[optind + 6], NULL, 0);
			mode.timing.h_sync_end = strtol(argv[optind + 7], NULL, 0);
			mode.timing.h_total = strtol(argv[optind + 8], NULL, 0);
			mode.timing.flags = 0;
			mode.space = B_RGB32;

			int i = optind + 9;
			while (i < argc) {
				if (!strcasecmp(argv[i], "+HSync"))
					mode.timing.flags |= B_POSITIVE_HSYNC;
				else if (!strcasecmp(argv[i], "+VSync"))
					mode.timing.flags |= B_POSITIVE_VSYNC;
				else if (!strcasecmp(argv[i], "Interlace"))
					mode.timing.flags |= B_TIMING_INTERLACED;
				else if (!strcasecmp(argv[i], "-VSync")
					|| !strcasecmp(argv[i], "-HSync")) {
					// okay, but nothing to do
				} else if (isdigit(argv[i][0]) && i + 1 == argc) {
					// bits per pixel
					mode.space
						= color_space_for_depth(strtoul(argv[i], NULL, 0));
				} else {
					fprintf(stderr, "Unknown flag: %s\n", argv[i]);
					exit(1);
				}

				i++;
			}

			mode.virtual_width = mode.timing.h_display;
			mode.virtual_height = mode.timing.v_display;
			mode.h_display_start = 0;
			mode.v_display_start = 0;
		}

		setMode = true;
	}

	BApplication application("application/x-vnd.Haiku-screenmode");

	ScreenMode screenMode(NULL);
	screen_mode currentMode;
	screenMode.Get(currentMode);

	if (listModes) {
		// List all reported modes
		if (!shortOutput)
			printf("Available screen modes:\n");

		for (int index = 0; index < screenMode.CountModes(); index++) {
			if (modeLine) {
				print_mode(screenMode.DisplayModeAt(index),
					screenMode.ModeAt(index));
			} else
				print_mode(screenMode.ModeAt(index), shortOutput);
		}

		return 0;
	}

	if (!setMode) {
		// Just print the current mode
		if (modeLine) {
			display_mode mode;
			screenMode.Get(mode);
			print_mode(mode, currentMode);
		} else {
			if (!shortOutput)
				printf("Resolution: ");
			print_mode(currentMode, shortOutput);
		}
		return 0;
	}

	screen_mode newMode = currentMode;

	if (fallbackMode) {
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
	} else if (modeLine) {
		display_mode currentDisplayMode;
		if (screenMode.Get(currentDisplayMode) == B_OK)
			mode.flags = currentDisplayMode.flags;
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

	status_t status;
	if (modeLine)
		status = screenMode.Set(mode);
	else
		status = screenMode.Set(newMode);

	if (status == B_OK) {
		if (confirm) {
			printf("Is this mode okay (Y/n - will revert after 10 seconds)? ");
			fflush(stdout);

			int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
			fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

			bigtime_t end = system_time() + 10000000LL;
			int c = 'n';
			while (system_time() < end) {
				c = getchar();
				if (c != -1)
					break;
			}

			if (c != '\n' && c != 'y')
				screenMode.Revert();
		}
	} else {
		fprintf(stderr, "%s: Could not set screen mode %ldx%ldx%ld: %s\n",
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
