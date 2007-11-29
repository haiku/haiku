/*
 * Copyright 2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <getopt.h>
#include <stdio.h>

#include <Application.h>
#include <InterfaceDefs.h>
#include <String.h>


static int sColorWhich = -1;
static struct option const kLongOptions[] = {
	{"activetab", required_argument, &sColorWhich, B_WINDOW_TAB_COLOR},
	{"frame", required_argument, &sColorWhich, B_WINDOW_INACTIVE_TAB_COLOR}, //XXX:??
	{"activeborder", required_argument, &sColorWhich, -1}, //XXX:??
	{"inactiveborder", required_argument, &sColorWhich, -1}, //XXX:??
	{"activetitle", required_argument, &sColorWhich, B_WINDOW_TEXT_COLOR},
	{"inactivetitle", required_argument, &sColorWhich, B_WINDOW_INACTIVE_TEXT_COLOR},
	{"sum", required_argument, 0, 's'},
	{"refresh", no_argument, 0, 'r'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *sProgramName = __progname;


void
usage(void)
{
	printf("%s [-sum md5sum] [colID colspec] [-refresh]\n", sProgramName);
	printf("Tweak the default window decorator colors.\n");
	printf("\t-sum          deprecated option, kept for compatibility\n");
	printf("\t-refresh      refresh the entire display (force update)\n");
	printf("\tcolID can be:\n");
	printf("\t-activetab\n");
	printf("\t-frame\n");
	printf("\t-activeborder\n");
	printf("\t-inactiveborder\n");
	printf("\t-activetitle\n");
	printf("\t-inactivetitle\n");
	printf("\tcolspec is a 6 digit hexadecimal color number. \n"
		"\t\t\t(rrggbb, html format)\n");
}


static int
UpdateUIColor(color_which which, const char *str)
{
	rgb_color color;
	unsigned int r, g, b;
	if (which < 0)
		return -1;
	// parse
	if (!str || !str[0])
		return -1;
	if (str[0] == '#')
		str++;
	if (sscanf(str, "%02x%02x%02x", &r, &g, &b) < 3)
		return -1;
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = 255;
	//printf("setting %d to {%d, %d, %d, %d}\n", which, r, g, b, 255);
	set_ui_color(which, color);
	return B_OK;
}

int
main(int argc, char **argv)
{
	BApplication app("application/x-vnd.Haiku-WindowShade");
	int c;

	// parse command line parameters

	while ((c = getopt_long_only(argc, argv, "h", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				UpdateUIColor((color_which)sColorWhich, optarg);
				break;
			case 'h':
				usage();
				return 0;
			default:
				usage();
				return 1;

			case 'r':
				// TODO: refresh (but shouldn't be needed)
				break;
			case 's':
				// IGNORED, for compatibility with original app
				break;
		}
		sColorWhich = -1;
	}

	return 0;
}
