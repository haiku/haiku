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
const uint32 kMsgDefaults			= 'BTde';
const uint32 kMsgRevert				= 'BTre';

const uint32 kMsgMouseType			= 'PUmt';
const uint32 kMsgMouseFocusMode		= 'PUmf';
const uint32 kMsgMouseMap			= 'PUmm';

const uint32 kMsgDoubleClickSpeed	= 'SLdc';
const uint32 kMsgMouseSpeed			= 'SLms';
const uint32 kMsgAccelerationFactor	= 'SLma';

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif	/* MOUSE_CONSTANTS_H */
