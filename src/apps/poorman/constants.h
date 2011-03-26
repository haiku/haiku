#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <GraphicsDefs.h>

// ------------------------------------------------------
// BMessages
const int32 MSG_MENU_FILE_SAVE_AS			= 'oMSA';
const int32 MSG_MENU_FILE_SAVE_SELECTION	= 'oMSS';
const int32 MSG_MENU_EDIT_COPY				= 'oMCP';
const int32 MSG_MENU_EDIT_SELECT_ALL		= 'oMSE';
const int32 MSG_MENU_EDIT_PREF				= 'oMPR';
const int32 MSG_MENU_CTRL_RUN				= 'oMRN';
const int32 MSG_MENU_CTRL_CLEAR_HIT			= 'oMCH';
const int32 MSG_MENU_CTRL_CLEAR_CONSOLE		= 'oMCC';
const int32 MSG_MENU_CTRL_CLEAR_LOG			= 'oMCL';
const int32 MSG_FILE_PANEL_SAVE_CONSOLE		= 'oSAV';
const int32 MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION	= 'SAVS';

// BMessages Preferences Window
const int32 MSG_PREF_BTN_DONE				= 'oPBD';
const int32 MSG_PREF_BTN_CANCEL				= 'oPBC';
			// Site Tab
const int32 MSG_PREF_SITE_BTN_SELECT		= 'PSBS';
const int32 MSG_PREF_SITE_CBX_INDEX			= 'PSCI';
const int32 MSG_FILE_PANEL_SELECT_WEB_DIR	= 'oSWD';
			// Logging Tab
const int32 MSG_PREF_LOG_CBX_CONSOLE		= 'PLCC';
const int32 MSG_PREF_LOG_CBX_FILE			= 'PLCF';
const int32 MSG_PREF_LOG_BTN_CREATE_FILE	= 'PLBF';
const int32 MSG_FILE_PANEL_CREATE_LOG_FILE	= 'oCLF';
			// Advanced Tab
const int32 MSG_PREF_ADV_SLD_MAX_CONNECTION = 'PASM';

// BMessage Preferences File
const uint32 MSG_PREF_FILE = 'pref';


const uint32 kStartServer = 'strt';

//BMessage for Logger
const int32 MSG_LOG = 'logg';
// ------------------------------------------------------
// PoorMan Window
extern const char* STR_APP_SIG;
extern const char* STR_APP_NAME;
extern const char* STR_ERR_WEB_DIR; 	// Alert Box
extern const char* STR_ERR_CANT_START;
extern const char* STR_DIR_CREATED;

extern const char* STR_MNU_FILE;
extern const char* STR_MNU_FILE_SAVE_AS;
extern const char* STR_MNU_FILE_SAVE_SELECTION;
extern const char* STR_MNU_FILE_QUIT;
extern const char* STR_MNU_EDIT;
extern const char* STR_MNU_EDIT_COPY;
extern const char* STR_MNU_EDIT_SELECT_ALL;
extern const char* STR_MNU_EDIT_PREF;
extern const char* STR_MNU_CTRL;
extern const char* STR_MNU_CTRL_RUN_SERVER;
extern const char* STR_MNU_CTRL_CLEAR_HIT_COUNTER;
extern const char* STR_MNU_CTRL_CLEAR_CONSOLE;
extern const char* STR_MNU_CTRL_CLEAR_LOG_FILE;

extern const char* STR_FILEPANEL_SAVE_CONSOLE;
extern const char* STR_FILEPANEL_SAVE_CONSOLE_SELECTION;
extern const char* STR_TXT_VIEW;

// ------------------------------------------------------
// Preferences Window
extern		 char* STR_WIN_NAME_PREF;
extern const char* STR_SETTINGS_FILE_NAME;
extern const char* STR_DEFAULT_WEB_DIRECTORY;

extern const char* STR_TAB_SITE;
extern const char* STR_BBX_LOCATION;
extern const char* STR_TXT_DIRECTORY;
extern const char* STR_BTN_DIRECTORY;
extern const char* STR_TXT_INDEX;
extern const char* STR_BBX_OPTIONS;
extern const char* STR_CBX_DIR_LIST;
extern const char* STR_CBX_DIR_LIST_LABEL;
extern const char* STR_FILEPANEL_SELECT_WEB_DIR;

extern const char* STR_TAB_LOGGING;
extern const char* STR_BBX_CONSOLE_LOGGING;
extern const char* STR_CBX_LOG_CONSOLE;
extern const char* STR_BBX_FILE_LOGGING;
extern const char* STR_CBX_LOG_FILE;
extern const char* STR_TXT_LOG_FILE_NAME;
extern const char* STR_BTN_CREATE_LOG_FILE;
extern const char* STR_FILEPANEL_CREATE_LOG_FILE;

extern const char* STR_TAB_ADVANCED;
extern const char* STR_BBX_CONNECTION;
extern const char* STR_SLD_LABEL;
extern 		 char* STR_SLD_STATUS_LABEL;

extern const char CMD_FILE_SAVE_AS;
extern const char CMD_FILE_QUIT;
extern const char CMD_EDIT_COPY;
extern const char CMD_EDIT_SELECT_ALL;


/* Colors */
extern const rgb_color WHITE;
extern const rgb_color GRAY;
extern const rgb_color BACKGROUND_COLOR;
extern const rgb_color BLACK;
extern const rgb_color RED;
extern const rgb_color GREEN;
#endif
