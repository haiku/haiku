/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef NETWORK_TIME_H
#define NETWORK_TIME_H


#include <SupportDefs.h>


static const uint32 kMsgUpdateTime = 'uptm';
static const uint32 kMsgStopTimeUpdate = 'stpt';
static const uint32 kMsgStatusUpdate = 'stup';
static const uint32 kMsgUpdateSettings = 'upse';
static const uint32 kMsgSettingsUpdated = 'seUp';
static const uint32 kMsgResetServerList = 'rtsl';

static const uint32 kMsgEditServerList = 'edsl';
static const uint32 kMsgEditServerListWindowClosed = 'eswc';

#endif	/* NETWORK_TIME_H */
