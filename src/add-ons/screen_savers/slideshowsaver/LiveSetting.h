/*****************************************************************************/
// LiveSetting
// Written by Michael Wilber
//
// LiveSetting.h
//
// This class stores the default value for an individual setting and provides
// functions which read and write the setting to BMessages.
//
//
// Copyright (C) Haiku
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef LIVE_SETTING_H
#define LIVE_SETTING_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>
#include <String.h>
#include <vector>
#include "LiveSettingsObserver.h"

class LiveSetting {
public:
	LiveSetting(uint32 id, const char *name, bool val);
	LiveSetting(uint32 id, const char *name, int32 val);
	LiveSetting(uint32 id, const char *name, const char *val);
	~LiveSetting() { }
	
	uint32 GetId() const { return fId; }
	const char *GetName() const { return fName; }
	
	bool AddReplaceValue(BMessage *msgTo, BMessage *msgFrom = NULL) const;
	
	bool AddReplaceValue(BMessage *msgTo, bool val) const;
	bool AddReplaceValue(BMessage *msgTo, int32 val) const;
	bool AddReplaceValue(BMessage *msgTo, const BString &val) const;
	
	bool GetValue(BMessage *msgFrom, bool &val) const;
	bool GetValue(BMessage *msgFrom, int32 &val) const;
	bool GetValue(BMessage *msgFrom, BString &val) const;

private:
	enum LiveSettingType {
		LIVE_SETTING_BOOL = 0,
		LIVE_SETTING_INT32,
		LIVE_SETTING_STRING
	};
	
	void Init(uint32 id, const char *name, const LiveSettingType dataType);
	
	uint32 fId;
	const char *fName;
	LiveSettingType fDataType;
	union {
		bool fBoolVal;
		int32 fInt32Val;
		const char *fCharpVal;
	};
};

#endif // #ifndef LIVE_SETTING_H


