/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPES_H
#define FILE_TYPES_H


#include <SupportDefs.h>


extern const char* kSignature;

static const uint32 kMsgOpenFilePanel = 'opFp';
static const uint32 kMsgOpenTypesWindow = 'opTw';
static const uint32 kMsgTypesWindowClosed = 'clTw';
static const uint32 kMsgWindowClosed = 'WiCl';

#endif	// FILE_TYPES_H
