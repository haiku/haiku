// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseConstants.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef MOUSE_CONSTANTS_H
#define MOUSE_CONSTANTS_H

// interface messages
const uint32 BUTTON_DEFAULTS			= 'BTde';
const uint32 BUTTON_REVERT				= 'BTre';

const uint32 POPUP_MOUSE_TYPE			= 'PUmt';
const uint32 POPUP_MOUSE_FOCUS			= 'PUmf';
const uint32 POPUP_MOUSE_MAP			= 'PUmm';

const uint32 DOUBLE_CLICK_TEST_AREA		= 'TCte';

const uint32 SLIDER_DOUBLE_CLICK_SPEED	= 'SLdc';
const uint32 SLIDER_MOUSE_SPEED			= 'SLms';
const uint32 SLIDER_MOUSE_ACC			= 'SLma';

const uint32 ERROR_DETECTED				= 'ERor';

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif	/* MOUSE_CONSTANTS_H */
