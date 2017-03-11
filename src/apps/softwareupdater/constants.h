/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

enum {
	ACTION_STEP_INIT = 0,
	ACTION_STEP_START,
	ACTION_STEP_DOWNLOAD,
	ACTION_STEP_APPLY,
	ACTION_STEP_COMPLETE,
	ACTION_STEP_MAX
};

enum {
	STATE_HEAD = 0,
	STATE_DISPLAY_STATUS,
	STATE_DISPLAY_PROGRESS,
	STATE_GET_CONFIRMATION,
	STATE_APPLY_UPDATES,
	STATE_MAX
};

// Message what values
static const uint32 kMsgTextUpdate = 'iUPD';
static const uint32 kMsgProgressUpdate = 'iPRO';
static const uint32 kMsgCancel = 'iCAN';
static const uint32 kMsgCancelResponse = 'iCRE';
static const uint32 kMsgUpdateConfirmed = 'iCON';
static const uint32 kMsgClose = 'iCLO';
static const uint32 kMsgShow = 'iSHO';
static const uint32 kMsgShowInfo = 'iSHI';
static const uint32 kMsgRegister = 'iREG';
static const uint32 kMsgFinalQuit = 'iFIN';

// Message data keys
#define kKeyHeader "key_header"
#define kKeyDetail "key_detail"
#define kKeyPackageName "key_packagename"
#define kKeyPackageCount "key_packagecount"
#define kKeyPercentage "key_percentage"
#define kKeyFrame "key_frame"
#define kKeyMessenger "key_messenger"


#endif // CONSTANTS_H
