/*
 * Copyright 2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#ifndef _CANNACOMMON_H
#define _CANNACOMMON_H

#include <SupportDefs.h>
#include <Point.h>
//BMessage constant
#define CANNA_METHOD_ACTIVATED			'Cact'

//Kouho Window messages
#define KOUHO_WINDOW_SHOW				'Kshw'
#define KOUHO_WINDOW_HIDE				'Khid'
#define KOUHO_WINDOW_SHOWAT				'Ksat'
#define KOUHO_WINDOW_SETTEXT			'Kstx'
#define KOUHO_WINDOW_SHOW_ALONE			'Ksal'
#define NUM_SELECTED_FROM_KOUHO_WIN		'Knum'

//Palette Window messages
#define PALETTE_WINDOW_SHOW				'Pshw'
#define PALETTE_WINDOW_HIDE				'Phid'
#define PALETTE_WINDOW_BUTTON_UPDATE	'Pupd'
#define MODE_CHANGED_FROM_PALETTE		'Pmch'

//result codes from CannaInterface object
const uint32	MIKAKUTEI_NO_CHANGE		= 0x00;
const uint32	KAKUTEI_EXISTS			= 0x01;
const uint32	MIKAKUTEI_EXISTS		= 0x02;
const uint32	NEW_INPUT_STARTED		= 0x04;
const uint32	THROUGH_INPUT			= 0x08;
const uint32	GUIDELINE_APPEARED		= 0x10;
const uint32	GUIDELINE_DISAPPEARED	= 0x20;
const uint32	GUIDELINE_CHANGED		= 0x40;
const uint32	MODE_CHANGED			= 0x80;
const uint32	NOTHING_TO_KAKUTEI		= 0x100;
const uint32	MIKAKUTEI_BECOME_EMPTY	= 0x200;

//Menu messages
#define RELOAD_INIT_FILE				'Mrel'
#define ARROW_KEYS_FLIPPED				'Marf'

//buffer related constants
#define CONVERT_BUFFER_SIZE 510
#define KOUHO_WINDOW_MAXCHAR 150
#define NUMBER_DISPLAY_MAXCHAR 20

#define CANNAIM_SETTINGS_FILE	"/boot/home/config/settings/cannaim_settings"

typedef struct Preferences
{
	BPoint			palette_loc;
	bool			convert_arrowkey;
	BPoint			standalone_loc;
} Preferences;


#endif
