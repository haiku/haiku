/*
 * Copyright 2002-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Pascal R. G. Abresch
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
const uint32 MENU_RELOAD				= 'MFrl';
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
const uint32 MSG_HIDE_WINDOW			= 'mhdw';
const uint32 MSG_FIND_WINDOW_QUIT		= 'mfwq';
const uint32 MSG_REPLACE_WINDOW_QUIT	= 'mrwq';

// "Font"-menu
const uint32 FONT_SIZE					= 'FMsi';
const uint32 FONT_FAMILY				= 'FFch';
const uint32 FONT_STYLE					= 'FSch';
const uint32 FONT_COLOR					= 'Fcol';
const uint32 kMsgSetItalic				= 'Fita';
const uint32 kMsgSetBold				= 'Fbld';
const uint32 kMsgSetUnderline			= 'Fuln';
const uint32 kMsgSetFontDown			= 'Fsdw';
const uint32 kMsgSetFontUp				= 'Fsup';

const rgb_color palette[] = {
	{ 220, 0, 0, 255 },		// red
	{ 220, 73, 0, 255 },	// orange
	{ 220, 220, 0, 255 },	// yellow
	{ 59, 177, 0, 255 },	// green
	{ 60, 22, 0, 255 },		// brown
	{ 36, 71, 106, 255 },	// blue
	{ 0, 123, 186, 255 },	// cyan
	{ 0, 106, 115, 255 },	// teal
	{ 53, 53, 53, 255 },	// charcoal
	{ 137, 0, 72, 255 },	// magenta
	{ 0, 0, 0, 255 },		//black
	{ 255, 255, 255, 255 }	// white
};

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

// Update StatusView
const uint32 UPDATE_STATUS				= 'UPSt';
const uint32 UPDATE_STATUS_REF			= 'UPSr';
const uint32 UNLOCK_FILE				= 'UNLk';
const uint32 UPDATE_LINE_SELECTION		= 'UPls';

// fontSize constant
const int32 fontSizes[] = {9, 10, 11, 12, 14, 18, 24, 36, 48, 72};

#endif	// CONSTANTS_H

