/*****************************************************************************/
// LiveSetting
// Written by Michael Wilber
//
// LiveSetting.cpp
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

#include "LiveSetting.h"

void
LiveSetting::Init(uint32 id, const char *name, const LiveSettingType dataType)
{
	fId = id;
	fName = name;
	fDataType = dataType;
}

LiveSetting::LiveSetting(uint32 id, const char *name, bool val)
{
	Init(id, name, LIVE_SETTING_BOOL);
	fBoolVal = val;
}

LiveSetting::LiveSetting(uint32 id, const char *name, int32 val)
{
	Init(id, name, LIVE_SETTING_INT32);
	fInt32Val = val;
}

LiveSetting::LiveSetting(uint32 id, const char *name, const char *val)
{
	Init(id, name, LIVE_SETTING_STRING);
	fCharpVal = val;
}

bool
LiveSetting::AddReplaceValue(BMessage *msgTo, BMessage *msgFrom /*= NULL*/) const
{
	bool bResult = false;
	
	switch (fDataType) {
		case LIVE_SETTING_BOOL:
		{
			bool bVal;
			if (GetValue(msgFrom, bVal) == true)
				return AddReplaceValue(msgTo, bVal);
		}
		case LIVE_SETTING_INT32:
		{
			int32 iVal;
			if (GetValue(msgFrom, iVal) == true)
				return AddReplaceValue(msgTo, iVal);
		}
		case LIVE_SETTING_STRING:
		{
			BString str;
			if (GetValue(msgFrom, str) == true)
				return AddReplaceValue(msgTo, str);
		}
		default:
			break;
	}
	
	return bResult;
}	
	
bool
LiveSetting::AddReplaceValue(BMessage *msgTo, bool val) const
{
	if (msgTo == NULL)
		return false;

	bool bResult = false;
	bool dummy;
	if (msgTo->FindBool(fName, &dummy) == B_OK) {
		if (msgTo->ReplaceBool(fName, val) == B_OK)
			bResult = true;
	} else {
		if (msgTo->AddBool(fName, val) == B_OK)
			bResult = true;
	}
	
	return bResult;
}

bool
LiveSetting::AddReplaceValue(BMessage *msgTo, int32 val) const
{
	if (msgTo == NULL)
		return false;

	bool bResult = false;
	int32 dummy;
	if (msgTo->FindInt32(fName, &dummy) == B_OK) {
		if (msgTo->ReplaceInt32(fName, val) == B_OK)
			bResult = true;
	} else {
		if (msgTo->AddInt32(fName, val) == B_OK)
			bResult = true;
	}
	
	return bResult;
}

bool
LiveSetting::AddReplaceValue(BMessage *msgTo, const BString &val) const
{
	if (msgTo == NULL)
		return false;

	bool bResult = false;
	BString dummy;
	if (msgTo->FindString(fName, &dummy) == B_OK) {
		if (msgTo->ReplaceString(fName, val) == B_OK)
			bResult = true;
	} else {
		if (msgTo->AddString(fName, val) == B_OK)
			bResult = true;
	}
	
	return bResult;
}
	
bool
LiveSetting::GetValue(BMessage *msgFrom, bool &val) const
{
	if (fDataType != LIVE_SETTING_BOOL)
		return false;
	
	bool bResult = false;
	if (msgFrom == NULL) {
		val = fBoolVal;
		bResult = true;
		
	} else if (msgFrom->FindBool(fName, &val) == B_OK)
		bResult = true;
	
	return bResult;
}

bool
LiveSetting::GetValue(BMessage *msgFrom, int32 &val) const
{
	if (fDataType != LIVE_SETTING_INT32)
		return false;
	
	bool bResult = false;
	if (msgFrom == NULL) {
		val = fInt32Val;
		bResult = true;
		
	} else if (msgFrom->FindInt32(fName, &val) == B_OK)
		bResult = true;
	
	return bResult;
}

bool
LiveSetting::GetValue(BMessage *msgFrom, BString &val) const
{
	if (fDataType != LIVE_SETTING_STRING)
		return false;
	
	bool bResult = false;
	if (msgFrom == NULL) {
		val = fCharpVal;
		bResult = true;
		
	} else if (msgFrom->FindString(fName, &val) == B_OK)
		bResult = true;
	
	return bResult;
}

