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
#   define EQ(x)
#else
#   define EXTERN extern
#   define EQ(x)
#endif

#define EXTERN_CONST extern const


// Resources
EXTERN char* kStrScan;
EXTERN char* kStrRescan;
EXTERN char* kOutdatedStr;
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
EXTERN_CONST char*	kAppSignature;
EXTERN_CONST char*	kHelpFileName;
EXTERN_CONST char*	kPieRectAttrName;

EXTERN_CONST char*	kHelpBtnLabel;
EXTERN_CONST char*	kEmptyStr;
EXTERN_CONST char*	kNameVolPtr;
EXTERN_CONST char*	kNameFilePtr;

EXTERN_CONST float	kSmallHMargin;
EXTERN_CONST float	kSmallVMargin;
EXTERN_CONST float	kButtonMargin;
EXTERN_CONST float	kMinButtonWidth;

EXTERN_CONST float	kProgBarWidth;
EXTERN_CONST float	kProgBarHeight;
EXTERN_CONST float	kReportInterval;

EXTERN_CONST float	kDefaultPieSize;
EXTERN_CONST float	kPieCenterSize;
EXTERN_CONST float	kPieRingSize;
EXTERN_CONST float	kPieInnerMargin;
EXTERN_CONST float	kPieOuterMargin;
EXTERN_CONST float	kMinSegmentSpan;
EXTERN_CONST int	kLightenFactor;
EXTERN_CONST float	kDragThreshold;

EXTERN app_info kAppInfo;
EXTERN entry_ref kHelpFileRef;
EXTERN bool kFoundHelpFile;

#define kMenuSelectVol				'gMSV'
#define kBtnRescan					'gBRF'
#define kBtnHelp					'gHLP'
#define kScanRefresh				'gSRF'
#define kScanProgress				'gSPR'
#define kScanDone					'gSDN'
#define kOutdatedMsg				'gOUT'

#define deg2rad(x) (2.0 * M_PI * (x) / 360.0)
#define rad2deg(x) (360.0 * (x) / (2.0 * M_PI))

BResources* read_resources(const char* appSignature);
void size_to_string(off_t byteCount, char* name);


#endif // COMMON_H
