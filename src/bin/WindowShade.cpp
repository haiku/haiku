/*
 * Copyright 2007-2011, Haiku Inc. All Rights Reserved.
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

#include <private/interface/DecoratorPrivate.h>


using namespace BPrivate;


static int sColorWhich = -1;
static struct option const kLongOptions[] = {
	{"activetab", required_argument, &sColorWhich, B_WINDOW_TAB_COLOR},
	{"frame", required_argument, &sColorWhich, B_WINDOW_INACTIVE_TAB_COLOR}, //XXX:??
	{"activeborder", required_argument, &sColorWhich, -1}, //XXX:??
	{"inactiveborder", required_argument, &sColorWhich, -1}, //XXX:??
	{"activetitle", required_argument, &sColorWhich, B_WINDOW_TEXT_COLOR},
	{"inactivetitle", required_argument, &sColorWhich, B_WINDOW_INACTIVE_TEXT_COLOR},
	//temporary stuff
	// grep '      B_.*_COLOR' headers/os/interface/InterfaceDefs.h | awk '{ print "I(" substr(tolower($1),3) ", " $1 ")," }'
#define I(l,v) \
	{ #l, required_argument, &sColorWhich, v }, \
	{ #v, required_argument, &sColorWhich, v }

	I(panel_background_color, B_PANEL_BACKGROUND_COLOR),
	I(panel_text_color, B_PANEL_TEXT_COLOR),
	I(document_background_color, B_DOCUMENT_BACKGROUND_COLOR),
	I(document_text_color, B_DOCUMENT_TEXT_COLOR),
	I(control_background_color, B_CONTROL_BACKGROUND_COLOR),
	I(control_text_color, B_CONTROL_TEXT_COLOR),
	I(control_border_color, B_CONTROL_BORDER_COLOR),
	I(control_highlight_color, B_CONTROL_HIGHLIGHT_COLOR),
	I(navigation_base_color, B_NAVIGATION_BASE_COLOR),
	I(navigation_pulse_color, B_NAVIGATION_PULSE_COLOR),
	I(shine_color, B_SHINE_COLOR),
	I(shadow_color, B_SHADOW_COLOR),
	I(menu_background_color, B_MENU_BACKGROUND_COLOR),
	I(menu_selected_background_color, B_MENU_SELECTED_BACKGROUND_COLOR),
	I(menu_item_text_color, B_MENU_ITEM_TEXT_COLOR),
	I(menu_selected_item_text_color, B_MENU_SELECTED_ITEM_TEXT_COLOR),
	I(menu_selected_border_color, B_MENU_SELECTED_BORDER_COLOR),
	I(tooltip_background_color, B_TOOL_TIP_BACKGROUND_COLOR),
	I(tooltip_text_color, B_TOOL_TIP_TEXT_COLOR),
	I(success_color, B_SUCCESS_COLOR),
	I(failure_color, B_FAILURE_COLOR),
	I(keyboard_navigation_color, B_KEYBOARD_NAVIGATION_COLOR),
	I(menu_selection_background_color, B_MENU_SELECTION_BACKGROUND_COLOR),
	I(desktop_color, B_DESKTOP_COLOR),
	I(window_tab_color, B_WINDOW_TAB_COLOR),
	I(window_text_color, B_WINDOW_TEXT_COLOR),
	I(window_inactive_tab_color, B_WINDOW_INACTIVE_TAB_COLOR),
	I(window_inactive_text_color, B_WINDOW_INACTIVE_TEXT_COLOR),
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
			{
				// TODO: refresh (but shouldn't be needed)
				BString name;
				if (get_decorator(name))
					set_decorator(name);
				break;
			}

			case 's':
				// IGNORED, for compatibility with original app
				break;
		}
		sColorWhich = -1;
	}

	return 0;
}
