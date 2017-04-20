/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MESSAGE_DRIVER_SETTINGS_UTILS__H
#define _MESSAGE_DRIVER_SETTINGS_UTILS__H

#include <SupportDefs.h>

struct driver_settings;
class BMessage;
class BFile;

#define MDSU_NAME				"Name"
#define MDSU_VALUES				"Values"
#define MDSU_PARAMETERS			"Parameters"
#define MDSU_VALID				"Valid"


extern bool FindMessageParameter(const char *name, const BMessage& message,
	BMessage *save, int32 *startIndex = NULL);

extern driver_settings *MessageToDriverSettings(const BMessage& message);

extern bool ReadMessageDriverSettings(const char *name, BMessage *message);
extern bool WriteMessageDriverSettings(BFile& file, const BMessage& message);


#endif
