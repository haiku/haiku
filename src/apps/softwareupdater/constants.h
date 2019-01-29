/*
 * Copyright 2017-2019, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 *		Jacob Secunda
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

#define kAppSignature "application/x-vnd.haiku-softwareupdater"
#define kSettingsFilename "SoftwareUpdater_settings"

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
	STATE_FINAL_MESSAGE,
	STATE_MAX
};

enum update_type {
	USER_SELECTION_NEEDED = 0,
	INVALID_SELECTION,
	CANCEL_UPDATE,
	UPDATE,
	UPDATE_CHECK_ONLY,
	FULLSYNC,
	UPDATE_TYPE_END
};

// Message what values
static const uint32 kMsgTextUpdate = 'iUPD';
static const uint32 kMsgProgressUpdate = 'iPRO';
static const uint32 kMsgCancel = 'iCAN';
static const uint32 kMsgCancelResponse = 'iCRE';
static const uint32 kMsgUpdateConfirmed = 'iCON';
static const uint32 kMsgWarningDismissed = 'iWDI';
static const uint32 kMsgGetUpdateType = 'iGUP';
static const uint32 kMsgNoRepositories = 'iNRE';
static const uint32 kMsgRegister = 'iREG';
static const uint32 kMsgFinalQuit = 'iFIN';
static const uint32 kMsgMoreDetailsToggle = 'iDTO';
static const uint32 kMsgWindowFrameChanged = 'iWFC';
static const uint32 kMsgSetZoomLimits = 'iSZL';
static const uint32 kMsgReboot = 'iREB';
static const uint32 kMsgShowReboot = 'iSRB';

// Message data keys
#define kKeyHeader "key_header"
#define kKeyDetail "key_detail"
#define kKeyPackageName "key_packagename"
#define kKeyPackageCount "key_packagecount"
#define kKeyPercentage "key_percentage"
#define kKeyMessenger "key_messenger"
#define kKeyAlertResult "key_alertresult"

// Settings keys
#define kKeyShowDetails "ShowDetails"
#define kKeyWindowFrame "WindowFrame"

#endif // CONSTANTS_H
