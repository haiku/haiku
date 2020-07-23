/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APPSERVER_CONFIG_H
#define _APPSERVER_CONFIG_H

// This is defined to place the server in test mode, which modifies certain things like
// system keyboard shortcuts. Note that it is possible, though senseless, to place it in
// regular mode and still use a display driver which depends on the R5 app_server
#ifndef TEST_MODE
#	define TEST_MODE 0
#endif

// Define this if you want the display driver to emulate the input server.
#if TEST_MODE
#	define ENABLE_INPUT_SERVER_EMULATION
//#	define USE_DIRECT_WINDOW_TEST_MODE
#endif

// This is the application signature of our app_server when running as a
// regular application. When running as the app_server, this is not used.
#define SERVER_SIGNATURE "application/x-vnd.haiku-app-server"

// Folder for additional window decorators
#define DECORATORS_DIR "/boot/home/config/add-ons/decorators/"

// These definitions provide the server something to use for default
// system fonts.
#define DEFAULT_PLAIN_FONT_FAMILY "Noto Sans Display"
#define FALLBACK_PLAIN_FONT_FAMILY "Swis721 BT"
#define DEFAULT_PLAIN_FONT_STYLE "Book"
#define DEFAULT_PLAIN_FONT_SIZE 12.0f
#define DEFAULT_BOLD_FONT_FAMILY "Noto Sans Display"
#define FALLBACK_BOLD_FONT_FAMILY "Swis721 BT"
#define DEFAULT_BOLD_FONT_STYLE "Bold"
#define DEFAULT_BOLD_FONT_SIZE 12.0f
#define DEFAULT_FIXED_FONT_FAMILY "Noto Sans Mono"
#define FALLBACK_FIXED_FONT_FAMILY "Courier10 BT"
#define DEFAULT_FIXED_FONT_STYLE "Regular"
#define DEFAULT_FIXED_FONT_SIZE 12.0f

// This is the port capacity for all monitoring objects - ServerApps
// and ServerWindows
#define DEFAULT_MONITOR_PORT_SIZE 50

#endif	/* _APPSERVER_CONFIG_H */
