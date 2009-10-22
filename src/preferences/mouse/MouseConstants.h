/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Brecht Machiels (brecht@mos6581.org)
 */
#ifndef MOUSE_CONSTANTS_H
#define MOUSE_CONSTANTS_H


// interface messages
const uint32 kMsgDefaults			= 'BTde';
const uint32 kMsgRevert				= 'BTre';

const uint32 kMsgMouseType			= 'PUmt';
const uint32 kMsgMouseFocusMode		= 'PUmf';
const uint32 kMsgFollowsMouseMode	= 'PUff';
const uint32 kMsgAcceptFirstClick	= 'PUaf';
const uint32 kMsgMouseMap			= 'PUmm';

const uint32 kMsgDoubleClickSpeed	= 'SLdc';
const uint32 kMsgMouseSpeed			= 'SLms';
const uint32 kMsgAccelerationFactor	= 'SLma';

// user interface
const uint32 kBorderSpace = 10;
const uint32 kItemSpace = 7;

#endif	/* MOUSE_CONSTANTS_H */
