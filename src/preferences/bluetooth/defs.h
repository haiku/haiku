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

#define BLUETOOTH_APP_SIGNATURE "application/x-vnd.haiku-BluetoothPrefs"

#define APPLY_SETTINGS 'aply'
#define REVERT_SETTINGS 'rvrt'
#define DEFAULT_SETTINGS 'dflt'
#define TRY_SETTINGS 'trys'

#define ATTRIBUTE_CHOSEN 'atch'
#define UPDATE_COLOR 'upcl'
#define DECORATOR_CHOSEN 'dcch'
#define UPDATE_DECORATOR 'updc'
#define UPDATE_COLOR_SET 'upcs'

#define SET_VISIBLE 		'sVis'
#define SET_DISCOVERABLE 	'sDis'

#define SET_UI_COLORS 'suic'
#define PREFS_CHOSEN 'prch'

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif
