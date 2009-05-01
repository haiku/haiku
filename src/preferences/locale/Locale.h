/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_H
#define LOCALE_H


#include <SupportDefs.h>


extern const char *kSignature;

static const uint32 kMsgOpenFilePanel = 'opFp';
static const uint32 kMsgOpenOpenWindow = 'opOw';
static const uint32 kMsgOpenWindowClosed = 'clOw';
static const uint32 kMsgWindowClosed = 'WiCl';
static const uint32 kMsgSettingsChanged = 'SeCh';

static const uint32 kMsgOpenFindWindow = 'OpFw';
static const uint32 kMsgFindWindowClosed = 'clFw';
static const uint32 kMsgFindTarget = 'FTgt';
static const uint32 kMsgFind = 'find';

#endif	/* LOCALE_H */
