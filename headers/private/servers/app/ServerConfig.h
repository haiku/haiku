#ifndef _APPSERVER_CONFIG_H
#define _APPSERVER_CONFIG_H

// This is defined to place the server in test mode, which modifies certain things like
// system keyboard shortcuts. Note that it is possible, though senseless, to place it in
// regular mode and still use a display driver which depends on the R5 app_server
#ifndef TEST_MODE
	#define TEST_MODE 0
#endif

// Uncomment this if the DisplayDriver should only rely on drawing functions implemented
// in software even though hardware-accelerated functions are available
// NOTE: everything is software right now (since DisplayDriverPainter)
//#define DISABLE_HARDWARE_ACCELERATION

// Define this for a quick hack to test some of the drawing functions
//#define DISPLAYDRIVER_TEST_HACK

// Define this if you want the display driver to emulate the input server.
#if TEST_MODE
#	define ENABLE_INPUT_SERVER_EMULATION
#endif

// This is the application signature of our app_server when running as a
// regular application. When running as the app_server, this is not used.
#define SERVER_SIGNATURE "application/x-vnd.haiku-app-server"

// ToDo: use find_directory() instead of absolute path names!
// Directory for all app_server-related settings. Must include ending slash.
#define SERVER_SETTINGS_DIR "/boot/home/config/settings/app_server/"

// Flattened list of usable fonts maintained by the server. The file is a
// flattened BMessage which is used for caching the names when obtained
// by the various font functions.
#define SERVER_FONT_LIST "/boot/home/config/settings/app_server/fontlist"

// Flattened list of BMessages containing data for each workspace
#define WORKSPACE_DATA_LIST "/boot/home/config/settings/app_server/workspace_settings"

// folder used to keep saved color sets for the UI - tab color, etc.
#define COLOR_SET_DIR "/boot/home/config/settings/color_sets/"

// folder used to keep saved cursor sets for the system
#define CURSOR_SET_DIR "/boot/home/config/settings/cursor_sets/"

// name of the file containing the current UI color settings
#define COLOR_SETTINGS_NAME "system_colors"

// name of the file containing the current system cursor settings
#define CURSOR_SETTINGS_NAME "system_cursors"

// name of the file containing the config data for the desktop
#define WORKSPACE_SETTINGS_NAME "workspace_data"

// Folder for additional window decorators
#define DECORATORS_DIR "/boot/home/config/add-ons/decorators/"

// These definitions provide the server something to use for default
// system fonts.
# define DEFAULT_PLAIN_FONT_FAMILY "Bitstream Vera Sans"
# define FALLBACK_PLAIN_FONT_FAMILY "Swis721 BT"
# define DEFAULT_PLAIN_FONT_STYLE "Roman"
# define DEFAULT_PLAIN_FONT_SIZE 12.0f
# define DEFAULT_BOLD_FONT_FAMILY "Bitstream Vera Sans"
# define FALLBACK_BOLD_FONT_FAMILY "Swis721 BT"
# define DEFAULT_BOLD_FONT_STYLE "Bold"
# define DEFAULT_BOLD_FONT_SIZE 12.0f
# define DEFAULT_FIXED_FONT_FAMILY "Bitstream Vera Sans Mono"
# define FALLBACK_FIXED_FONT_FAMILY "Courier10 BT"
# define DEFAULT_FIXED_FONT_STYLE "Roman"
# define DEFAULT_FIXED_FONT_SIZE 12.0f
// This is the port capacity for all monitoring objects - ServerApps
// and ServerWindows
#define DEFAULT_MONITOR_PORT_SIZE 30

#endif	/* _APPSERVER_CONFIG_H */
