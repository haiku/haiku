//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _MESSAGE_DRIVER_SETTINGS_UTILS__H
#define _MESSAGE_DRIVER_SETTINGS_UTILS__H

#include <SupportDefs.h>

class BMessage;
class BFile;

#define MDSU_NAME				"Name"
#define MDSU_VALUES				"Values"
#define MDSU_PARAMETERS			"Parameters"
#define MDSU_VALID				"Valid"


extern bool FindMessageParameter(const char *name, BMessage& message,
	BMessage& save, int32 *startIndex = NULL);

extern bool ReadMessageDriverSettings(const char *name, BMessage& message);
extern bool WriteMessageDriverSettings(BFile& file, const BMessage& message);

#endif
