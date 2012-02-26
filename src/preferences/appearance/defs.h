/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#ifndef DEFS_H_
#define DEFS_H_

// If these paths are changed, ensure that they all end in a '/' character
/*
#define SETTINGS_DIR "/boot/home/config/settings/app_server/"
#define COLOR_SET_DIR "/boot/home/config/settings/color_sets/"
#define CURSOR_SET_DIR "/boot/home/config/settings/cursor_sets/"
#define DECORATORS_DIR "/boot/home/config/add-ons/decorators/"
#define COLOR_SETTINGS_NAME "system_colors"
*/

#define APPEARANCE_APP_SIGNATURE "application/x-vnd.Haiku-Appearance"

#define APPLY_SETTINGS 'aply'
#define TRY_SETTINGS 'trys'

#define ATTRIBUTE_CHOSEN 'atch'
#define UPDATE_COLOR 'upcl'
#define DECORATOR_CHOSEN 'dcch'
#define UPDATE_DECORATOR 'updc'
#define UPDATE_COLOR_SET 'upcs'

#define SET_DECORATOR 'sdec'
#define GET_DECORATOR 'gdec'

#define SET_UI_COLORS 'suic'
#define PREFS_CHOSEN 'prch'

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif
