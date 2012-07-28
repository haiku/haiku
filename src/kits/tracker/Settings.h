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
#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#include <String.h>
#include "SettingsHandler.h"


namespace BPrivate {

extern Settings* settings;

class StringValueSetting : public SettingsArgvDispatcher {
	// simple string setting
public:
	StringValueSetting(const char* name, const char* defaultValue,
		const char* valueExpectedErrorString,
		const char* wrongValueErrorString);

	virtual ~StringValueSetting();

	void ValueChanged(const char* newValue);
	const char* Value() const;
	virtual const char* Handle(const char* const *argv);

protected:
	virtual void SaveSettingValue(Settings*);
	virtual bool NeedsSaving() const;

	const char* fDefaultValue;
	const char* fValueExpectedErrorString;
	const char* fWrongValueErrorString;
	BString fValue;
};

class EnumeratedStringValueSetting : public StringValueSetting {
	// string setting, values that do not match string enumeration
	// are rejected
public:
	EnumeratedStringValueSetting(const char* name, const char* defaultValue,
		const char* const* values, const char* valueExpectedErrorString,
		const char* wrongValueErrorString);

	void ValueChanged(const char* newValue);
	virtual const char* Handle(const char* const *argv);

protected:
	const char* const* fValues;
};

class ScalarValueSetting : public SettingsArgvDispatcher {
	// simple int32 setting
public:
	ScalarValueSetting(const char* name, int32 defaultValue,
		const char* valueExpectedErrorString,
		const char* wrongValueErrorString, int32 min = LONG_MIN,
		int32 max = LONG_MAX);

	void ValueChanged(int32 newValue);
	int32 Value() const;
	void GetValueAsString(char*) const;
	virtual const char* Handle(const char* const *argv);

protected:
	virtual void SaveSettingValue(Settings*);
	virtual bool NeedsSaving() const;

	int32 fDefaultValue;
	int32 fValue;
	int32 fMax;
	int32 fMin;

	const char* fValueExpectedErrorString;
	const char* fWrongValueErrorString;
};

class HexScalarValueSetting : public ScalarValueSetting {
	// hexadecimal int32 setting
public:
	HexScalarValueSetting(const char* name, int32 defaultValue,
		const char* valueExpectedErrorString,
		const char* wrongValueErrorString, int32 min = LONG_MIN,
		int32 max = LONG_MAX);

	void GetValueAsString(char* buffer) const;

protected:
	virtual void SaveSettingValue(Settings* settings);
};

class BooleanValueSetting : public ScalarValueSetting {
	// on-off setting
public:
	BooleanValueSetting(const char* name, bool defaultValue);

	bool Value() const;
	void SetValue(bool value);
	virtual const char* Handle(const char* const *argv);

protected:
	virtual void SaveSettingValue(Settings*);
};

}

using namespace BPrivate;

#endif	// _SETTINGS_H_
