/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATIONS_H
#define _NOTIFICATIONS_H

#include <Mime.h>
#include <View.h>
#include <String.h>

#define kNotificationServerSignature "application/x-vnd.Haiku-notification_server"

#define B_FOLLOW_DESKBAR B_FOLLOW_NONE

// Messages
const uint32 kNotificationMessage = 'nssm';

// Settings constants
extern const char* kSettingsFile;

// General settings
extern const char* kAutoStartName;
extern const char* kTimeoutName;
extern const char* kWidthName;
extern const char* kIconSizeName;
extern const char* kNotificationPositionName;

// General default settings
const bool kDefaultAutoStart = true;
const int32 kDefaultTimeout = 10;
const int32 kMinimumTimeout = 3;
const int32 kMaximumTimeout = 30;
const float kDefaultWidth = 300.0f;
const float kMinimumWidth = 300.0f;
const float kMaximumWidth = 1000.0f;
const int32 kWidthStep = 50;
const icon_size kDefaultIconSize = B_LARGE_ICON;
const uint32 kDefaultNotificationPosition = B_FOLLOW_DESKBAR;

#endif	// _NOTIFICATIONS_H
