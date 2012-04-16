#include "constants.h"

#include <Catalog.h>
#include <InterfaceDefs.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


// ------------------------------------------------------
// PoorMan Window
const char* STR_APP_SIG
	= "application/x-vnd.Haiku-PoorMan";
const char* STR_APP_NAME
	= B_TRANSLATE_SYSTEM_NAME("PoorMan");
const char* STR_ERR_WEB_DIR
	= B_TRANSLATE(
	  "Please choose the folder to publish on the web.\n\n"
	  "You can have PoorMan create a default \"public_html\" "
	  "in your home folder.\n"
	  "Or you select one of your own folders instead.");
const char* STR_ERR_CANT_START
	= B_TRANSLATE(
	  "Cannot start the server");
const char* STR_DIR_CREATED
	= B_TRANSLATE(
	  "A default web folder has been created "
	  "at \"/boot/home/public_html.\"\nMake sure there is a HTML "
	  "file named \"index.html\" in that folder.");

const char* STR_MNU_FILE
	= B_TRANSLATE("File");
const char* STR_MNU_FILE_SAVE_AS
	= B_TRANSLATE("Save console as" B_UTF8_ELLIPSIS);
const char* STR_MNU_FILE_SAVE_SELECTION
	= B_TRANSLATE("Save console selections as" B_UTF8_ELLIPSIS);
const char* STR_MNU_FILE_QUIT
	= B_TRANSLATE("Quit");
const char* STR_MNU_EDIT
	= B_TRANSLATE("Edit");
const char* STR_MNU_EDIT_COPY
	= B_TRANSLATE("Copy");
const char* STR_MNU_EDIT_SELECT_ALL
	= B_TRANSLATE("Select all");
const char* STR_MNU_EDIT_PREF
	= B_TRANSLATE("Settings" B_UTF8_ELLIPSIS);
const char* STR_MNU_CTRL
	= B_TRANSLATE("Controls");
const char* STR_MNU_CTRL_RUN_SERVER
	= B_TRANSLATE("Run server");
const char* STR_MNU_CTRL_CLEAR_HIT_COUNTER
	= B_TRANSLATE("Clear hit counter");
const char* STR_MNU_CTRL_CLEAR_CONSOLE
	= B_TRANSLATE("Clear console log");
const char* STR_MNU_CTRL_CLEAR_LOG_FILE
	= B_TRANSLATE("Clear log file");

const char* STR_FILEPANEL_SAVE_CONSOLE
	= B_TRANSLATE("Save log console");
const char* STR_FILEPANEL_SAVE_CONSOLE_SELECTION
	= B_TRANSLATE("Save log console selection");
const char* STR_TXT_VIEW
	= B_TRANSLATE("Logging view");

// ------------------------------------------------------
// Preferences Window
char* STR_WIN_NAME_PREF
	= (char*)B_TRANSLATE("PoorMan settings");

const char* STR_SETTINGS_FILE_NAME
	= "PoorMan settings";
const char* STR_DEFAULT_WEB_DIRECTORY
	= "/boot/home/public_html";
const char* STR_TAB_SITE
	= B_TRANSLATE("Site");
const char* STR_BBX_LOCATION
	= B_TRANSLATE("Website location");
const char* STR_TXT_DIRECTORY
	= B_TRANSLATE("Web folder:");
const char* STR_BTN_DIRECTORY
	= B_TRANSLATE("Select web folder");
const char* STR_TXT_INDEX
	= B_TRANSLATE("Start page:");
const char* STR_BBX_OPTIONS
	= B_TRANSLATE("Website options");
const char* STR_CBX_DIR_LIST
	= B_TRANSLATE("Send file listing if there's no start page");
const char* STR_CBX_DIR_LIST_LABEL
	= B_TRANSLATE("Send file listing if there's no start page");
const char* STR_FILEPANEL_SELECT_WEB_DIR
	= B_TRANSLATE("Select web folder");

const char* STR_TAB_LOGGING
	= B_TRANSLATE("Logging");
const char* STR_BBX_CONSOLE_LOGGING
	= B_TRANSLATE("Console logging");
const char* STR_CBX_LOG_CONSOLE
	= B_TRANSLATE("Log to console");
const char* STR_BBX_FILE_LOGGING
	= B_TRANSLATE("File logging");
const char* STR_CBX_LOG_FILE
	= B_TRANSLATE("Log to file");
const char* STR_TXT_LOG_FILE_NAME
	= B_TRANSLATE("Log file name:");
const char* STR_BTN_CREATE_LOG_FILE
	= B_TRANSLATE("Create log file");
const char* STR_FILEPANEL_CREATE_LOG_FILE
	= B_TRANSLATE("Create PoorMan log");

const char* STR_TAB_ADVANCED
	= B_TRANSLATE("Advanced");
const char* STR_BBX_CONNECTION
	= B_TRANSLATE("Connections");
const char* STR_SLD_LABEL
	= B_TRANSLATE("Max. simultaneous connections:");
char* STR_SLD_STATUS_LABEL
	= (char*)B_TRANSLATE("connections");

const char CMD_FILE_SAVE_AS
	= 'S';
const char CMD_FILE_QUIT
	= 'Q';
const char CMD_EDIT_COPY
	= 'C';
const char CMD_EDIT_SELECT_ALL
	= 'A';

// --------------------------------
const rgb_color WHITE				= { 255, 255, 255, 255 };
const rgb_color GRAY				= { 184, 184, 184, 255 };
const rgb_color BACKGROUND_COLOR	= { 216, 216, 216, 255 };
const rgb_color BLACK				= { 0, 0, 0, 255 };
const rgb_color RED					= { 255, 0, 0, 255};
const rgb_color GREEN				= { 0, 255, 0, 255};
