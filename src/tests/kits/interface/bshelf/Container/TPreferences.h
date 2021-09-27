//
// TPreferences
// Using BMessages to save user settings.
//
// Eric Shepherd
//
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	Modified by H. Reh "SetData" added - Dec. 2001
	                   "SetRef"  added - Dec. 2003
*/

#ifndef __TPREFS_H__
#define __TPREFS_H__

#include <Path.h>
#include <Message.h>
#include <Directory.h>

class _EXPORT TPreferences : public BMessage 
{
	public:
									TPreferences(char *filename);
									~TPreferences();
	
	
			status_t		InitCheck(void);
	
			status_t		SetBool(const char *name, bool b);
			status_t		SetInt8(const char *name, int8 i);
			status_t		SetInt16(const char *name, int16 i);
			status_t		SetInt32(const char *name, int32 i);
			status_t		SetInt64(const char *name, int64 i);
			status_t		SetFloat(const char *name, float f);
			status_t		SetDouble(const char *name, double d);
			status_t		SetString(const char *name, const char *string);
			status_t		SetPoint(const char *name, BPoint p);
			status_t		SetRect(const char *name, BRect r);
			status_t		SetMessage(const char *name, const BMessage *message);
			status_t		SetFlat(const char *name, const BFlattenable *obj);
			status_t		SetData(const char *name, type_code type, const void *data, ssize_t numBytes);
			status_t		SetRef(const char *name, entry_ref *ref);
	
	private:	
			BPath				path;
			BPath				parent;
			status_t		status;
};

inline status_t TPreferences::InitCheck(void) 
{
	return status;
}

#endif
