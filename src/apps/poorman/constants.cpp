#include "constants.h"

#include <InterfaceDefs.h>

// ------------------------------------------------------
// PoorMan Window
const char* STR_APP_SIG
	= "application/x-vnd.Haiku.PoorMan";
char* STR_APP_NAME
	= (char*)"PoorMan";
const char* STR_ERR_WEB_DIR
	= "Please choose the folder to publish on the web.\n\n"
	  "You can have PoorMan create a default \"public_html\" "
	  "in your home folder.\n"
	  "Or you select one of your own folders instead.";
const char* STR_ERR_CANT_START
	= "Cannot start the server";
const char* STR_DIR_CREATED
	= "A default web folder has been created "
	  "at \"/boot/home/public_html.\"\nMake sure there is a HTML "
	  "file named \"index.html\" in that folder.";

const char* STR_MNU_FILE
	= "File";
const char* STR_MNU_FILE_SAVE_AS
	= "Save console as" B_UTF8_ELLIPSIS;
const char* STR_MNU_FILE_SAVE_SELECTION
	= "Save console selections as" B_UTF8_ELLIPSIS;
const char* STR_MNU_FILE_ABOUT
	= "About PoorMan" B_UTF8_ELLIPSIS;
const char* STR_MNU_FILE_QUIT
	= "Quit";
const char* STR_MNU_EDIT
	= "Edit";
const char* STR_MNU_EDIT_COPY
	= "Copy";
const char* STR_MNU_EDIT_SELECT_ALL
	= "Select all";
const char* STR_MNU_EDIT_PREF
	= "Settings" B_UTF8_ELLIPSIS;
const char* STR_MNU_CTRL
	= "Controls";
const char* STR_MNU_CTRL_RUN_SERVER
	= "Run server";
const char* STR_MNU_CTRL_CLEAR_HIT_COUNTER
	= "Clear hit counter";
const char* STR_MNU_CTRL_CLEAR_CONSOLE
	= "Clear console log";
const char* STR_MNU_CTRL_CLEAR_LOG_FILE
	= "Clear log file";

const char* STR_FILEPANEL_SAVE_CONSOLE
	= "Save log console";
const char* STR_FILEPANEL_SAVE_CONSOLE_SELECTION
	= "Save log console selection";
const char* STR_TXT_VIEW
	= "Logging view";

// ------------------------------------------------------
// Preferences Window
char* STR_WIN_NAME_PREF
	= (char*)"PoorMan settings";

const char* STR_SETTINGS_FILE_NAME
	= "PoorMan settings";
const char* STR_SETTINGS_NEW_FILE_NAME
	= "OBOS PoorMan settings";
const char* STR_DEFAULT_WEB_DIRECTORY
	= "/boot/home/public_html";
const char* STR_TAB_SITE
	= "Site";
const char* STR_BBX_LOCATION
	= "Website location";
const char* STR_TXT_DIRECTORY
	= "Web folder:";
const char* STR_BTN_DIRECTORY
	= "Select web folder";
const char* STR_TXT_INDEX
	= "Start page:";
const char* STR_BBX_OPTIONS
	= "Website options";
const char* STR_CBX_DIR_LIST
	= "Send file listing if there's no start page";
const char* STR_CBX_DIR_LIST_LABEL
	= "Send file listing if there's no start page";
const char* STR_FILEPANEL_SELECT_WEB_DIR
	= "Select web folder";

const char* STR_TAB_LOGGING
	= "Logging";
const char* STR_BBX_CONSOLE_LOGGING
	= "Console logging";
const char* STR_CBX_LOG_CONSOLE
	= "Log to console";
const char* STR_BBX_FILE_LOGGING
	= "File logging";
const char* STR_CBX_LOG_FILE
	= "Log to file";
const char* STR_TXT_LOG_FILE_NAME
	= "Log file name:";
const char* STR_BTN_CREATE_LOG_FILE
	= "Create log file";
const char* STR_FILEPANEL_CREATE_LOG_FILE
	= "Create PoorMan log";

const char* STR_TAB_ADVANCED
	= "Advanced";
const char* STR_BBX_CONNECTION
	= "Connections";
const char* STR_SLD_LABEL
	= "Max. simultaneous connections:";
char* STR_SLD_STATUS_LABEL
	= (char*)"connections";

const char CMD_FILE_SAVE_AS
	= 'S';
const char CMD_FILE_QUIT
	= 'Q';
const char CMD_EDIT_COPY
	= 'C';
const char CMD_EDIT_SELECT_ALL
	= 'A';

const char* STR_ABOUT_TITLE
	= "About PoorMan";
const char* STR_ABOUT_BUTTON
	= "OK";
const char* STR_ABOUT_DESC
	= "Poor Man's web server\n"
	"Copyright " B_UTF8_COPYRIGHT " 2004-2009 Haiku\n"
	"All rights reserved.\n\n"
	"Written by: Philip Harrison, Ma Jie";

// --------------------------------
const rgb_color WHITE				= { 255, 255, 255, 255 };
const rgb_color GRAY				= { 184, 184, 184, 255 };
const rgb_color BACKGROUND_COLOR	= { 216, 216, 216, 255 };
const rgb_color BLACK				= { 0, 0, 0, 255 };
const rgb_color RED					= { 255, 0, 0, 255};
const rgb_color GREEN				= { 0, 255, 0, 255};
