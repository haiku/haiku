/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H


#include <GraphicsDefs.h>
#include <SupportDefs.h>


#define APP_SIGNATURE  "application/x-vnd.Haiku-StyledEdit"

const float TEXT_INSET = 3.0;

// Messages for menu commands

// file menu
const uint32 MENU_NEW					= 'MFnw';
const uint32 MENU_OPEN					= 'MFop';
const uint32 MENU_SAVE					= 'MSav';
const uint32 MENU_SAVEAS				= 'MEsa';
const uint32 MENU_REVERT				= 'MFre';
const uint32 MENU_CLOSE					= 'MFcl';
const uint32 MENU_PAGESETUP				= 'MFps';
const uint32 MENU_PRINT					= 'MFpr';
const uint32 MENU_QUIT					= 'MFqu';

// edit menu
const uint32 MENU_CLEAR					= 'MEcl';
const uint32 MENU_FIND					= 'MEfi';
const uint32 MENU_FIND_AGAIN			= 'MEfa';
const uint32 MENU_FIND_SELECTION		= 'MEfs';
const uint32 MENU_REPLACE				= 'MEre';
const uint32 MENU_REPLACE_SAME			= 'MErs';

const uint32 MSG_SEARCH					= 'msea';
const uint32 MSG_REPLACE				= 'msre';
const uint32 MSG_REPLACE_ALL			= 'mrea';

// "Font"-menu
const uint32 FONT_SIZE					= 'FMsi';
const uint32 FONT_FAMILY				= 'FFch';
const uint32 FONT_STYLE					= 'FSch';
const uint32 FONT_COLOR					= 'Fcol';
const uint32 kMsgSetItalic				= 'Fita';
const uint32 kMsgSetBold				= 'Fbld';

// fontcolors
const rgb_color	BLACK 					= {0, 0, 0, 255};
const rgb_color	RED 					= {255, 0, 0, 255};
const rgb_color	GREEN					= {0, 255, 0, 255};
const rgb_color	BLUE					= {0, 0, 255, 255};
const rgb_color	CYAN					= {0, 255, 255, 255};
const rgb_color	MAGENTA					= {255, 0, 255, 255};
const rgb_color	YELLOW 					= {255, 255, 0, 255};

// "Document"-menu
const uint32 ALIGN_LEFT					= 'ALle';
const uint32 ALIGN_CENTER				= 'ALce';
const uint32 ALIGN_RIGHT				= 'ALri';
const uint32 WRAP_LINES					= 'MDwr';
const uint32 SHOW_STATISTICS			= 'MDss';

// enables "edit" menuitems
const uint32 ENABLE_ITEMS				= 'ENit';
const uint32 DISABLE_ITEMS				= 'DIit';
const uint32 CHANGE_WINDOW				= 'CHwi';
const uint32 TEXT_CHANGED				= 'TEch';

// file panel constants
const uint32 OPEN_AS_ENCODING			= 'FPoe';
const uint32 SAVE_AS_ENCODING			= 'FPse';
const uint32 SAVE_THEN_QUIT				= 'FPsq';

// Update Line Info
const uint32 UPDATE_LINE				= 'UPln';

#endif	// CONSTANTS_H

