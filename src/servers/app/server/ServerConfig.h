#ifndef _APPSERVER_CONFIG_H
#define _APPSERVER_CONFIG_H

// The ViewDriver is a BView/BWindow combination. Plenty of functionality,
// but dog-slow.
#define VIEWDRIVER 0

// The ScreenDriver utilizes a BWindowScreen to directly access the video
// hardware.
#define SCREENDRIVER 1

// The SecondDriver allows the server to make use of a second video card while
// still running as a regular BeOS application. You could say it's training
// wheels for the HWDriver
#define SECONDDRIVER 2

// The HWDriver is the real thing - loads all the graphics hardware and 
// literally takes over the system as the app_server is supposed to. This 
// one can be used only when the app_server is running as *the* app_server
// and not in testing mode like the other drivers are for.
#define HWDRIVER 3

// Display driver to be used by the server.
#define DISPLAYDRIVER VIEWDRIVER

// Define this if you want the display driver to emulate the input server.
// Comment this out if DISPLAYDRIVER is defined as HWDRIVER.
#define ENABLE_INPUT_SERVER_EMULATION

// This is the application signature of our app_server when running as a
// regular application. When running as the app_server, this is not used.
#define SERVER_SIGNATURE "application/x-vnd.obe-OBAppServer"

// Server port names. The input port is the port which is used to receive
// input messages from the Input Server. The other is the "main" port for
// the server and is utilized mostly by BApplication objects.
#define SERVER_PORT_NAME "OBappserver"
#define SERVER_INPUT_PORT "OBinputport"

// Directory for all app_server-related settings.
#define SERVER_SETTINGS_DIR "/boot/home/config/settings/app_server/"

// Flattened list of usable fonts maintained by the server. The file is a
// flattened BMessage which is used for caching the names when obtained
// by the various font functions.
#define SERVER_FONT_LIST "/boot/home/config/settings/app_server/fontlist"

// folder used to keep saved color sets for the UI - tab color, etc.
#define COLOR_SET_DIR "/boot/home/config/settings/color_sets/"

// name of the file containing the current UI color settings
#define COLOR_SETTINGS_NAME "system_colors"

// Folder for additional window decorators
#define DECORATORS_DIR "/boot/home/config/add-ons/decorators/"

// These definitions provide the server something to use for default
// system fonts.
#define DEFAULT_PLAIN_FONT_FAMILY "Swis721 BT"
#define DEFAULT_PLAIN_FONT_STYLE "Roman"
#define DEFAULT_PLAIN_FONT_SIZE 12
#define DEFAULT_BOLD_FONT_FAMILY "Swis721 BT"
#define DEFAULT_BOLD_FONT_STYLE "Bold"
#define DEFAULT_BOLD_FONT_SIZE 12
#define DEFAULT_FIXED_FONT_FAMILY "Courier10 BT"
#define DEFAULT_FIXED_FONT_STYLE "Roman"
#define DEFAULT_FIXED_FONT_SIZE 12

// This is the port capacity for all monitoring objects - ServerApps
// and ServerWindows
#define DEFAULT_MONITOR_PORT_SIZE 30

#endif
