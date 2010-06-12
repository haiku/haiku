//--------------------------------------------------------------------
//	
//	constants.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "constants.h"

extern const char* STR_APP_SIG
	= "application/x-vnd.BeDTS.MenuWorld";
	
#if defined(LOCALE_USA)

//--------------------------------------------------------------------
//	American English Strings


extern const char* STR_APP_NAME
	= "MenuWorld";

extern const char* STR_IERROR
	= "Internal Error:\n";

extern const char* STR_SEPARATOR
	= "--------------------";
	
extern const char* STR_NO_FULL_MENU_BAR
	= "Couldn't find the window's menu bar.";

extern const char* STR_NO_HIDDEN_MENU_BAR
	= "Couldn't find the window's hidden menu bar.";

extern const char* STR_NO_MENU_VIEW
	= "Couldn't find the window's menu view";

extern const char* STR_NO_STATUS_VIEW
	= "Couldn't find the window's status view";
	
extern const char* STR_NO_LABEL_CTRL
	= "Couldn't find the label edit field.";

extern const char* STR_NO_HIDE_USER_CHECK
	= "Couldn't find the hide user menus check box.";

extern const char* STR_NO_LARGE_ICON_CHECK
	= "Couldn't find the large test icons check box.";
		
extern const char* STR_NO_ADDMENU_BUTTON
	= "Couldn't find the add menu button.";
	
extern const char* STR_NO_ADDITEM_BUTTON
	= "Couldn't find the add menu item button.";
	
extern const char* STR_NO_DELETE_BUTTON
	= "Couldn't find the delete button.";

extern const char* STR_NO_MENU_OUTLINE
	= "Couldn't find the menu outline list.";

extern const char* STR_NO_MENU_SCROLL_VIEW
	= "Couldn't find the menu outline list scroll view.";
	
extern const char* STR_MNU_FILE
	= "File";
	
extern const char* STR_MNU_FILE_ABOUT
	= "About...";

extern const char* STR_MNU_FILE_CLOSE
	= "Close";
	
extern const char* STR_MNU_TEST
	= "Test";

extern const char* STR_MNU_TEST_ITEM
	= "Test Item";

extern const char* STR_MNU_EMPTY_ITEM
	= "(Empty menu)";

extern const char* STR_LABEL_CTRL
	= "Label:";
	
extern const char* STR_HIDE_USER_MENUS
	= "Hide User Menus";

extern const char* STR_LARGE_TEST_ICONS
	= "Large Test Icons";
	
extern const char* STR_ADD_MENU
	= "Add Menu";

extern const char* STR_ADD_ITEM
	= "Add Item";
	
extern const char* STR_ADD_SEP
	= "Add Separator";

extern const char* STR_DELETE_MENU
	= "Delete";
	
extern const char* STR_STATUS_DEFAULT
	= "";
	
extern const char* STR_STATUS_TEST
	= "Test item selected: ";
	
extern const char* STR_STATUS_USER
	= "Menu item selected: ";
	
extern const char* STR_STATUS_ADD_MENU
	= "Added menu: ";

extern const char* STR_STATUS_ADD_ITEM
	= "Added menu item: ";

extern const char* STR_STATUS_ADD_SEPARATOR
	= "Added separator menu item";
	
extern const char* STR_STATUS_DELETE_MENU
	= "Deleted menu: ";
	
extern const char* STR_STATUS_DELETE_ITEM
	= "Deleted menu item: ";
	
extern const char* STR_STATUS_DELETE_SEPARATOR
	= "Deleted separator menu item";

extern const char* STR_ABOUT_TITLE
	= "About MenuWorld";
	
extern const char* STR_ABOUT_BUTTON
	= "Struth!";
	
extern const char* STR_ABOUT_DESC
	= "MenuWorld: a menu editing example\n\n"
	"How to use:\n"
	"    Type some text into the label field, and "
	"press \"Add Menu\" to add a menu.\n"
	"    Select an item in the outline list, and "
	"press \"Delete\" to remove it.\n"
	"    Select an item in the outline list, type "
	"some text into the label field, and press "
	"\"Add Item\" to add an item under the selected "
	"item. If nothing besides spaces or dashes is "
	"typed in, a separator item will be created.\n"
	"    \"Hide User Menus\" hides the menus you've "
	"generated.\n"
	"    \"Large Test Icons\" changes the size of "
	"the numbers in the Test menu.";
	
#endif /* LOCALE_USA */



//--------------------------------------------------------------------
//	Shortcuts

extern const char CMD_FILE_CLOSE
	= 'W';
	
extern const char CMD_TEST_ICON_SIZE
	= 'I';



//--------------------------------------------------------------------
//	Colors

extern const rgb_color BKG_GREY		= { 216, 216, 216, 0 };
