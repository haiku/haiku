//--------------------------------------------------------------------
//	
//	constants.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _constants_h
#define _constants_h

#include <GraphicsDefs.h>

//--------------------------------------------------------------------
//	Message IDs

const int32 MSG_WIN_ADD_MENU			= 'oWAM';
const int32 MSG_WIN_DELETE_MENU			= 'oWDM';
const int32 MSG_VIEW_ADD_MENU			= 'oVAM';
const int32 MSG_VIEW_DELETE_MENU		= 'oVDM';
const int32 MSG_VIEW_ADD_ITEM			= 'oVAI';
const int32 MSG_WIN_HIDE_USER_MENUS		= 'oWHU';
const int32 MSG_WIN_LARGE_TEST_ICONS	= 'oWFM';
const int32 MSG_MENU_OUTLINE_SEL		= 'oSMU';
const int32 MSG_TEST_ITEM				= 'oWTM';
const int32 MSG_USER_ITEM				= 'oWUM';
const int32 MSG_LABEL_EDIT				= 'oLED';


//--------------------------------------------------------------------
//	Strings

extern const char* STR_APP_SIG;

// Simple localization scheme
#define LOCALE_USA

extern const char* STR_APP_NAME;
extern const char* STR_IERROR;
extern const char* STR_SEPARATOR;
extern const char* STR_NO_FULL_MENU_BAR;
extern const char* STR_NO_HIDDEN_MENU_BAR;
extern const char* STR_NO_MENU_VIEW;
extern const char* STR_NO_STATUS_VIEW;
extern const char* STR_NO_LABEL_CTRL;
extern const char* STR_NO_HIDE_USER_CHECK;
extern const char* STR_NO_LARGE_ICON_CHECK;
extern const char* STR_NO_ADDMENU_BUTTON;
extern const char* STR_NO_ADDITEM_BUTTON;
extern const char* STR_NO_DELETE_BUTTON;
extern const char* STR_NO_MENU_OUTLINE;
extern const char* STR_NO_MENU_SCROLL_VIEW;
extern const char* STR_MNU_FILE;
extern const char* STR_MNU_FILE_ABOUT;
extern const char* STR_MNU_FILE_CLOSE;
extern const char* STR_MNU_TEST;
extern const char* STR_MNU_TEST_ITEM;
extern const char* STR_MNU_EMPTY_ITEM;
extern const char* STR_LABEL_CTRL;
extern const char* STR_HIDE_USER_MENUS;
extern const char* STR_LARGE_TEST_ICONS;
extern const char* STR_ADD_MENU;
extern const char* STR_ADD_ITEM;
extern const char* STR_ADD_SEP;
extern const char* STR_DELETE_MENU;
extern const char* STR_STATUS_DEFAULT;
extern const char* STR_STATUS_TEST;
extern const char* STR_STATUS_USER;
extern const char* STR_STATUS_ADD_MENU;
extern const char* STR_STATUS_ADD_ITEM;
extern const char* STR_STATUS_ADD_SEPARATOR;
extern const char* STR_STATUS_DELETE_SEPARATOR;
extern const char* STR_STATUS_DELETE_MENU;
extern const char* STR_STATUS_DELETE_ITEM;
extern const char* STR_ABOUT_TITLE;
extern const char* STR_ABOUT_BUTTON;
extern const char* STR_ABOUT_DESC;


//--------------------------------------------------------------------
//	Shortcuts

extern const char CMD_FILE_CLOSE;
extern const char CMD_TEST_ICON_SIZE;


//--------------------------------------------------------------------
//	Colors

extern const rgb_color BKG_GREY;

#endif /* _constants_h */