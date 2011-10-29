/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef DISKUSAGE_H
#define DISKUSAGE_H


#include <Entry.h>
#include <InterfaceDefs.h>


const rgb_color RGB_WIN = { 0xDE, 0xDB, 0xDE, 0xFF };
const rgb_color RGB_PIE_OL = { 0x80, 0x80, 0x80, 0xFF };
const rgb_color RGB_PIE_BG = { 0xFF, 0xFF, 0xFF, 0xFF };
const rgb_color RGB_PIE_MT = { 0xA0, 0xA0, 0xA0, 0xFF };
const rgb_color RGB_PIE_1 = { 0x00, 0x00, 0xb6, 0xFF };
const rgb_color RGB_PIE_2 = { 0x00, 0x00, 0x68, 0xFF };
const rgb_color RGB_PIE_3 = { 0xcf, 0x00, 0x00, 0xFF };
const rgb_color RGB_PIE_4 = { 0xaf, 0x63, 0xb1, 0xFF };

const int kBasePieColorCount = 4;
const rgb_color kBasePieColor[kBasePieColorCount]
	= { RGB_PIE_1, RGB_PIE_2, RGB_PIE_3, RGB_PIE_4 };

const rgb_color kWindowColor = ui_color(B_PANEL_BACKGROUND_COLOR);
const rgb_color kOutlineColor = RGB_PIE_OL;
const rgb_color kPieBGColor = RGB_PIE_BG;
const rgb_color kEmptySpcColor = RGB_PIE_MT;

const char*	const kAppSignature		= "application/x-vnd.Haiku-DiskUsage";
const char*	const kHelpFileName		= "userguide/en/applications/diskusage.html";

const char*	const kEmptyStr			= "";
const char*	const kNameFilePtr		= "file";

const float	kSmallHMargin		= 5.0;
const float	kSmallVMargin		= 2.0;
const float	kButtonMargin		= 20.0;
const float	kMinButtonWidth		= 60.0;

const float	kProgBarWidth		= 150.0;
const float	kProgBarHeight		= 16.0;
const float	kReportInterval		= 2.5;

const float	kDefaultPieSize		= 400.0;
const float	kPieCenterSize		= 80.0;
const float	kPieRingSize		= 20.0;
const float	kPieInnerMargin		= 10.0;
const float	kPieOuterMargin		= 10.0;
const float	kMinSegmentSpan		= 2.0;
const int	kLightenFactor		= 0x12;
const float	kDragThreshold		= 5.0;

extern entry_ref helpFileRef;
extern bool helpFileWasFound;

#define kMenuSelectVol				'gMSV'
#define kBtnRescan					'gBRF'
#define kBtnHelp					'gHLP'
#define kScanRefresh				'gSRF'
#define kScanProgress				'gSPR'
#define kScanDone					'gSDN'
#define kOutdatedMsg				'gOUT'

#define deg2rad(x) (2.0 * M_PI * (x) / 360.0)
#define rad2deg(x) (360.0 * (x) / (2.0 * M_PI))

void size_to_string(off_t byteCount, char* name, int maxLength);


#endif // DISKUSAGE_H

