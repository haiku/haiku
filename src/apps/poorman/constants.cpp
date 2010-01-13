#include "constants.h"

#include <InterfaceDefs.h>

// ------------------------------------------------------
// PoorMan Window
extern const char* STR_APP_SIG
	= "application/x-vnd.Haiku.PoorMan";
char* STR_APP_NAME
	= "PoorMan";
extern const char* STR_ERR_WEB_DIR
	= "Please choose the folder to publish on the web.\n\n"
	  "You can have PoorMan create a default \"public_html\" "
	  "in your home folder.\n"
	  "Or you select one of your own folders instead.";
extern const char* STR_ERR_CANT_START
	= "Cannot start the server";
extern const char* STR_DIR_CREATED
	= "A default web folder has been created "
	  "at \"/boot/home/public_html.\"\nMake sure there is a HTML "
	  "file named \"index.html\" in that folder.";

extern const char* STR_MNU_FILE
	= "File";
extern const char* STR_MNU_FILE_SAVE_AS
	= "Save console as" B_UTF8_ELLIPSIS;
extern const char* STR_MNU_FILE_SAVE_SELECTION
	= "Save console selections as" B_UTF8_ELLIPSIS;
extern const char* STR_MNU_FILE_ABOUT
	= "About PoorMan" B_UTF8_ELLIPSIS;
extern const char* STR_MNU_FILE_QUIT
	= "Quit";
extern const char* STR_MNU_EDIT
	= "Edit";
extern const char* STR_MNU_EDIT_COPY
	= "Copy";
extern const char* STR_MNU_EDIT_SELECT_ALL
	= "Select all";
extern const char* STR_MNU_EDIT_PREF
	= "Settings" B_UTF8_ELLIPSIS;
extern const char* STR_MNU_CTRL
	= "Controls";
extern const char* STR_MNU_CTRL_RUN_SERVER
	= "Run server";
extern const char* STR_MNU_CTRL_CLEAR_HIT_COUNTER
	= "Clear hit counter";
extern const char* STR_MNU_CTRL_CLEAR_CONSOLE
	= "Clear console log";
extern const char* STR_MNU_CTRL_CLEAR_LOG_FILE
	= "Clear log file";

extern const char* STR_FILEPANEL_SAVE_CONSOLE
	= "Save log console";
extern const char* STR_FILEPANEL_SAVE_CONSOLE_SELECTION
	= "Save log console selection";
extern const char* STR_TXT_VIEW
	= "Logging view";

// ------------------------------------------------------
// Preferences Window
			 char* STR_WIN_NAME_PREF
	= "PoorMan settings";

extern const char* STR_SETTINGS_FILE_NAME
	= "PoorMan settings";
extern const char* STR_SETTINGS_NEW_FILE_NAME
	= "OBOS PoorMan settings";
extern const char* STR_DEFAULT_WEB_DIRECTORY
	= "/boot/home/public_html";
extern const char* STR_TAB_SITE
	= "Site";
extern const char* STR_BBX_LOCATION
	= "Website location";
extern const char* STR_TXT_DIRECTORY
	= "Web folder:";
extern const char* STR_BTN_DIRECTORY
	= "Select web folder";
extern const char* STR_TXT_INDEX
	= "Start page:";
extern const char* STR_BBX_OPTIONS
	= "Website options";
extern const char* STR_CBX_DIR_LIST
	= "Send file listing if there's no start page";
extern const char* STR_CBX_DIR_LIST_LABEL
	= "Send file listing if there's no start page";
extern const char* STR_FILEPANEL_SELECT_WEB_DIR
	= "Select web folder";

extern const char* STR_TAB_LOGGING
	= "Logging";
extern const char* STR_BBX_CONSOLE_LOGGING
	= "Console logging";
extern const char* STR_CBX_LOG_CONSOLE
	= "Log to console";
extern const char* STR_BBX_FILE_LOGGING
	= "File logging";
extern const char* STR_CBX_LOG_FILE
	= "Log to file";
extern const char* STR_TXT_LOG_FILE_NAME
	= "Log file name:";
extern const char* STR_BTN_CREATE_LOG_FILE
	= "Create log file";
extern const char* STR_FILEPANEL_CREATE_LOG_FILE
	= "Create PoorMan log";

extern const char* STR_TAB_ADVANCED
	= "Advanced";
extern const char* STR_BBX_CONNECTION
	= "Connections";
extern const char* STR_SLD_LABEL
	= "Max. simultaneous connections:";
	 		 char* STR_SLD_STATUS_LABEL
	= "connections";

extern const char CMD_FILE_SAVE_AS
	= 'S';
extern const char CMD_FILE_QUIT
	= 'Q';
extern const char CMD_EDIT_COPY
	= 'C';
extern const char CMD_EDIT_SELECT_ALL
	= 'A';

extern const char* STR_ABOUT_TITLE
	= "About PoorMan";
extern const char* STR_ABOUT_BUTTON
	= "OK";
extern const char* STR_ABOUT_DESC
	= "Poor Man's web server\n"
	"Copyright " B_UTF8_COPYRIGHT " 2004-2009 Haiku\n"
	"All rights reserved.\n\n"
	"Written by: Philip Harrison, Ma Jie";

// --------------------------------
extern const rgb_color WHITE				= { 255, 255, 255, 255 };
extern const rgb_color GRAY					= { 184, 184, 184, 255 };
extern const rgb_color BACKGROUND_COLOR		= { 216, 216, 216, 255 };
extern const rgb_color BLACK				= { 0, 0, 0, 255 };
extern const rgb_color RED					= { 255, 0, 0, 255};
extern const rgb_color GREEN				= { 0, 255, 0, 255};
