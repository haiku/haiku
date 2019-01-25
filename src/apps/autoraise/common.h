#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#include <AppKit.h>
#include <InterfaceKit.h>
#include <String.h>
#include <StorageKit.h>
#include <WindowInfo.h>

/***************************************************
	common.h
	Constants used by app
***************************************************/

// used to check the image to use to get the resources
#define APP_NAME "AutoRaise"
#define APP_SIG "application/x-vnd.mmu.AutoRaise"
#define SETTINGS_FILE "x-vnd.mmu.AutoRaise_settings"

//names of data segments in settings file
//also used in messages

#define DEFAULT_DELAY 500000LL

// float: delay before raise
#define AR_DELAY "ar:delay"
// bool: last state
#define AR_ACTIVE "ar:active"

#define AR_MODE "ar:mode"
enum {
	Mode_All,
	Mode_DeskbarOver,
	Mode_DeskbarTouch
};

#define AR_BEHAVIOUR "ar:behaviour"

// resources
#define ACTIVE_ICON "AR:ON"
#define INACTIVE_ICON "AR:OFF"

// messages

#define ADD_TO_TRAY 'zATT'
#define REMOVE_FROM_TRAY 'zRFT'
#define OPEN_SETTINGS 'zOPS'
#define MSG_DELAY_POPUP 'arDP'
#define MSG_TOGGLE_ACTIVE 'arTA'
#define MSG_SET_ACTIVE 'arSA'
#define MSG_SET_INACTIVE 'arSI'
#define MSG_SET_DELAY 'arSD'
#define MSG_SET_MODE 'arSM'
#define MSG_SET_BEHAVIOUR 'arSB'

#endif
