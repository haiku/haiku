#include "constants.h"

// ------------------------------------------------------
// PoorMan Window
extern const char* STR_APP_SIG
	= "application/x-vnd.OpenBeOS-PoorMan";
			 char* STR_APP_NAME
	= "PoorMan";
extern const char* STR_ERR_WEB_DIR
	= "You have not yet selected the folder you want PoorMan "
	  "to publish on the Web. PoorMan can create a "
	  "\"public_html\" folder in your home folder or you can select "
	  "one of your own folders to publish.\n\n"
	  "Do you wish to select a folder to publish on the Web?";	
extern const char* STR_ERR_CANT_START
	= "Can't Start the Server";

extern const char* STR_MNU_FILE
	= "File";
extern const char* STR_MNU_FILE_SAVE_AS
	= "Save Console As...";
extern const char* STR_MNU_FILE_SAVE_SELECTION
	= "Save Console Selections As...";
extern const char* STR_MNU_FILE_ABOUT
	= "About PoorMan";
extern const char* STR_MNU_FILE_QUIT
	= "Quit";
extern const char* STR_MNU_EDIT
	= "Edit";
extern const char* STR_MNU_EDIT_COPY
	= "Copy";
extern const char* STR_MNU_EDIT_SELECT_ALL
	= "Select All";
extern const char* STR_MNU_EDIT_PREF
	= "Preferences...";
extern const char* STR_MNU_CTRL
	= "Controls";
extern const char* STR_MNU_CTRL_RUN_SERVER
	= "Run Server";
extern const char* STR_MNU_CTRL_CLEAR_HIT_COUNTER
	= "Clear Hit Counter";
extern const char* STR_MNU_CTRL_CLEAR_CONSOLE
	= "Clear Console Log";
extern const char* STR_MNU_CTRL_CLEAR_LOG_FILE
	= "Clear Log File";

extern const char* STR_FILEPANEL_SAVE_CONSOLE
	= "Save Log Console";
extern const char* STR_FILEPANEL_SAVE_CONSOLE_SELECTION
	= "Save Log Console Selection";
extern const char* STR_TXT_VIEW
	= "Logging View";

// ------------------------------------------------------
// Preferences Window
			 char* STR_WIN_NAME_PREF
	= "PoorMan Settings";

extern const char* STR_SETTINGS_FILE_PATH
	= "/boot/home/config/settings/PoorMan Settings";
extern const char* STR_SETTINGS_NEW_FILE_PATH
	= "/boot/home/config/settings/OBOS PoorMan Settings";
	
extern const char* STR_TAB_SITE
	= "Site";
extern const char* STR_BBX_LOCATION
	= "Web Site Location";
extern const char* STR_TXT_DIRECTORY
	= "Web Directory:";
extern const char* STR_BTN_DIRECTORY
	= "Select Web Directory";
extern const char* STR_TXT_INDEX
	= "Index File Name:";
extern const char* STR_BBX_OPTIONS
	= "Web Site Options";
extern const char* STR_CBX_DIR_LIST
	= "Send Directory List if No Index";
extern const char* STR_CBX_DIR_LIST_LABEL
	= "Send Directory List if No Index";
extern const char* STR_FILEPANEL_SELECT_WEB_DIR
	= "Select Web Directory";
	
extern const char* STR_TAB_LOGGING
	= "Logging";
extern const char* STR_BBX_CONSOLE_LOGGING
	= "Console Logging";
extern const char* STR_CBX_LOG_CONSOLE
	= "Log to Console";
extern const char* STR_BBX_FILE_LOGGING
	= "File Logging";
extern const char* STR_CBX_LOG_FILE
	= "Log to File";
extern const char* STR_TXT_LOG_FILE_NAME
	= "Log File Name:";
extern const char* STR_BTN_CREATE_LOG_FILE
	= "Create Log File";
extern const char* STR_FILEPANEL_CREATE_LOG_FILE
	= "Create Poor Man Log";
	
extern const char* STR_TAB_ADVANCED
	= "Advanced";
extern const char* STR_BBX_CONNECTION
	= "Connections Options";
extern const char* STR_SLD_LABEL
	= "Maximum Simultaneous Connections:";
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
	= "Poor Man's Web Server\n"
	"Copyright © 2004 Haiku\n"
	"All rights reserved.\n\n"
	"Written by: Philip Harrison";
	
// --------------------------------	
extern const rgb_color WHITE				= { 255, 255, 255, 255 };
extern const rgb_color GRAY					= { 184, 184, 184, 255 };
extern const rgb_color BACKGROUND_COLOR		= { 216, 216, 216, 255 };
extern const rgb_color BLACK				= { 0, 0, 0, 255 };
