/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include <Debug.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "TrackerSettings.h"


Settings* settings = NULL;

// generic setting handler classes

StringValueSetting::StringValueSetting(const char* name,
	const char* defaultValue, const char* valueExpectedErrorString,
	const char* wrongValueErrorString)
	:	SettingsArgvDispatcher(name),
		fDefaultValue(defaultValue),
		fValueExpectedErrorString(valueExpectedErrorString),
		fWrongValueErrorString(wrongValueErrorString),
		fValue(defaultValue)
{
}


StringValueSetting::~StringValueSetting()
{
}


void
StringValueSetting::ValueChanged(const char* newValue)
{
	fValue = newValue;
}


const char*
StringValueSetting::Value() const
{
	return fValue.String();
}


void
StringValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write("\"%s\"", fValue.String());
}


bool
StringValueSetting::NeedsSaving() const
{
	// needs saving if different than default
	return fValue != fDefaultValue;
}


const char*
StringValueSetting::Handle(const char* const* argv)
{
	if (!*++argv)
		return fValueExpectedErrorString;
	
	ValueChanged(*argv);
	return 0;
}


//	#pragma mark -


EnumeratedStringValueSetting::EnumeratedStringValueSetting(const char* name,
	const char* defaultValue, const char* const* values,
	const char* valueExpectedErrorString, const char* wrongValueErrorString)
	:	StringValueSetting(name, defaultValue, valueExpectedErrorString,
			wrongValueErrorString),
		fValues(values)
{
}


void
EnumeratedStringValueSetting::ValueChanged(const char* newValue)
{
#if DEBUG
	// must be one of the enumerated values
	bool found = false;
	for (int32 index = 0; ; index++) {
		if (!fValues[index])
			break;

		if (strcmp(fValues[index], newValue) != 0)
			continue;

		found = true;
		break;
	}
	ASSERT(found);
#endif
	StringValueSetting::ValueChanged(newValue);
}


const char*
EnumeratedStringValueSetting::Handle(const char* const* argv)
{
	if (!*++argv)
		return fValueExpectedErrorString;

	bool found = false;
	for (int32 index = 0; ; index++) {
		if (!fValues[index])
			break;

		if (strcmp(fValues[index], *argv) != 0)
			continue;

		found = true;
		break;
	}

	if (!found)
		return fWrongValueErrorString;

	ValueChanged(*argv);
	return 0;
}


//	#pragma mark -


ScalarValueSetting::ScalarValueSetting(const char* name, int32 defaultValue,
	const char* valueExpectedErrorString, const char* wrongValueErrorString,
	int32 min, int32 max)
	:	SettingsArgvDispatcher(name),
		fDefaultValue(defaultValue),
		fValue(defaultValue),
		fMax(max),
		fMin(min),
		fValueExpectedErrorString(valueExpectedErrorString),
		fWrongValueErrorString(wrongValueErrorString)
{
}


void
ScalarValueSetting::ValueChanged(int32 newValue)
{
	ASSERT(newValue > fMin);
	ASSERT(newValue < fMax);
	fValue = newValue;
}


int32
ScalarValueSetting::Value() const
{
	return fValue;
}


void
ScalarValueSetting::GetValueAsString(char* buffer) const
{
	sprintf(buffer, "%" B_PRId32, fValue);
}


const char*
ScalarValueSetting::Handle(const char* const* argv)
{
	if (!*++argv)
		return fValueExpectedErrorString;

	int32 newValue;
	if ((*argv)[0] == '0' && (*argv)[1] == 'x')
		sscanf(*argv, "%" B_PRIx32, &newValue);
	else
		newValue = atoi(*argv);

	if (newValue < fMin || newValue > fMax)
		return fWrongValueErrorString;

	fValue = newValue;
	return NULL;
}


void
ScalarValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write("%ld", fValue);
}


bool
ScalarValueSetting::NeedsSaving() const
{
	return fValue != fDefaultValue;
}


//	#pragma mark -


HexScalarValueSetting::HexScalarValueSetting(const char* name,
	int32 defaultValue, const char* valueExpectedErrorString,
	const char* wrongValueErrorString, int32 min, int32 max)
		:	ScalarValueSetting(name, defaultValue, valueExpectedErrorString,
					wrongValueErrorString, min, max)
{
}


void
HexScalarValueSetting::GetValueAsString(char* buffer) const
{
	sprintf(buffer, "0x%08" B_PRIx32, fValue);
}


void
HexScalarValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write("0x%08" B_PRIx32, fValue);
}


//	#pragma mark -


BooleanValueSetting::BooleanValueSetting(const char* name, bool defaultValue)
	:	ScalarValueSetting(name, defaultValue, 0, 0)
{
}


bool
BooleanValueSetting::Value() const
{
	return fValue != 0;
}


void
BooleanValueSetting::SetValue(bool value)
{
	fValue = value;
}


const char*
BooleanValueSetting::Handle(const char* const* argv)
{
	if (!*++argv)
		return "on or off expected";

	if (strcmp(*argv, "on") == 0)
		fValue = true;
	else if (strcmp(*argv, "off") == 0)
		fValue = false;
	else
		return "on or off expected";

	return 0;
}


void
BooleanValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write(fValue ? "on" : "off");
}
