#ifndef _MESSAGE_DRIVER_SETTINGS_UTILS__H
#define _MESSAGE_DRIVER_SETTINGS_UTILS__H

#include <SupportDefs.h>

class BMessage;
class BFile;


extern bool FindMessageParameter(const char *name, BMessage& message,
	BMessage& save, int32 *startIndex = NULL);

extern bool ReadMessageDriverSettings(const char *name, BMessage& message);
extern bool WriteMessageDriverSettings(BFile& file, const BMessage& message);

#endif
