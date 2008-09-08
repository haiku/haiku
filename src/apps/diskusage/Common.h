/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef COMMON_H
#define COMMON_H

#include <Entry.h>
#include <GraphicsDefs.h>
#include <Resources.h>
#include <Roster.h>


#ifdef ASSIGN_RESOURCES
#   define EXTERN
#   define EQ(x) =x
#else
#   define EXTERN extern
#   define EQ(x)
#endif

#define PUBLIC_CONST extern const


// Resources
EXTERN char* kStrRescan;
EXTERN char* kStrScanningX;
EXTERN char* kStrUnavail;
EXTERN char* kVolMenuLabel;
EXTERN char* kVolMenuDefault;
EXTERN char* kVolPrompt;
EXTERN char* kOneFile;
EXTERN char* kManyFiles;
EXTERN char* kMenuGetInfo;
EXTERN char* kMenuOpen;
EXTERN char* kMenuOpenWith;
EXTERN char* kMenuNoApps;
EXTERN char* kInfoSize;
EXTERN char* kInfoInFiles;
EXTERN char* kInfoCreated;
EXTERN char* kInfoModified;
EXTERN char* kInfoTimeFmt;
EXTERN char* kInfoKind;
EXTERN char* kInfoPath;

EXTERN rgb_color kWindowColor;
EXTERN rgb_color kOutlineColor;
EXTERN rgb_color kPieBGColor;
EXTERN rgb_color kEmptySpcColor;
EXTERN int kBasePieColorCount;
EXTERN rgb_color *kBasePieColor;

// Non-resources :)
PUBLIC_CONST char*	kAppSignature		EQ("application/x-vnd.haiku-diskusage");
PUBLIC_CONST char*	kHelpFileName		EQ("apps/diskusage/DiskUsage.html");
PUBLIC_CONST char*	kPieRectAttrName	EQ("mainrect");

PUBLIC_CONST char*	kHelpBtnLabel		EQ("?");
PUBLIC_CONST char*	kEmptyStr			EQ("");
PUBLIC_CONST char*	kNameVolPtr			EQ("vol");
PUBLIC_CONST char*	kNameFilePtr		EQ("file");

PUBLIC_CONST float	kSmallHMargin		EQ(5.0);
PUBLIC_CONST float	kSmallVMargin		EQ(2.0);
PUBLIC_CONST float	kButtonMargin		EQ(20.0);
PUBLIC_CONST float	kMinButtonWidth		EQ(60.0);

PUBLIC_CONST float	kProgBarWidth		EQ(150.0);
PUBLIC_CONST float	kProgBarHeight		EQ(16.0);
PUBLIC_CONST float	kReportInterval		EQ(2.5);

PUBLIC_CONST float	kDefaultPieSize		EQ(400.0);
PUBLIC_CONST float	kPieCenterSize		EQ(80.0);
PUBLIC_CONST float	kPieRingSize		EQ(20.0);
PUBLIC_CONST float	kPieInnerMargin		EQ(10.0);
PUBLIC_CONST float	kPieOuterMargin		EQ(10.0);
PUBLIC_CONST float	kMinSegmentSpan		EQ(2.0);
PUBLIC_CONST int	kLightenFactor		EQ(0x12);
PUBLIC_CONST float	kDragThreshold		EQ(5.0);

EXTERN app_info kAppInfo;
EXTERN entry_ref kHelpFileRef;
EXTERN bool kFoundHelpFile;

#define kMenuSelectVol				'gMSV'
#define kBtnRescan					'gBRF'
#define kBtnHelp					'gHLP'
#define kScanRefresh				'gSRF'
#define kScanProgress				'gSPR'
#define kScanDone					'gSDN'

#define deg2rad(x) (2.0 * M_PI * (x) / 360.0)
#define rad2deg(x) (360.0 * (x) / (2.0 * M_PI))

BResources* read_resources(const char* appSignature);
void size_to_string(off_t byteCount, char* name);

#endif // COMMON_H

