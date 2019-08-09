/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "SettingsHandler.h"

void SetUpSettings(char* filename);
void QuitSettings();

class StringValueSetting : public SettingsArgvDispatcher {
	// simple string setting
public:
								StringValueSetting(const char* name,
									const char* defaultValue,
									const char* valueExpectedErrorString,
									const char* wrongValueErrorString = "");

	virtual						~StringValueSetting();

			void				ValueChanged(const char* newValue);
			const char*			Value() const;
	virtual const char*			Handle(const char* const *argv);

protected:
	virtual	void				SaveSettingValue(Settings*);
	virtual	bool				NeedsSaving() const;

			const char*			fDefaultValue;
			const char*			fValueExpectedErrorString;
			const char*			fWrongValueErrorString;
			char*				fValue;
};


// string setting, values that do not match string enumeration
// are rejected
class EnumeratedStringValueSetting : public StringValueSetting {
public:
	// A pointer to a function returning a string for an index, or NULL
	// if the index is out of bounds.
	typedef const char* (*StringEnumerator)(int32);

								EnumeratedStringValueSetting(const char* name,
									const char* defaultValue,
									StringEnumerator enumerator,
									const char* valueExpectedErrorString,
									const char* wrongValueErrorString);

			void				ValueChanged(const char* newValue);
	virtual const char*			Handle(const char* const *argv);

private:
			bool				_ValidateString(const char* string);
			StringEnumerator	fEnumerator;
};


// simple int32 setting
class ScalarValueSetting : public SettingsArgvDispatcher {
public:
								ScalarValueSetting(const char* name,
									int32 defaultValue,
									const char* valueExpectedErrorString,
									const char* wrongValueErrorString,
									int32 min = INT32_MIN,
									int32 max = INT32_MAX);
	virtual						~ScalarValueSetting();

			void				ValueChanged(int32 newValue);
			int32				Value() const;
			void				GetValueAsString(char*) const;
	virtual const char*			Handle(const char* const *argv);

protected:
	virtual void				SaveSettingValue(Settings*);
	virtual bool				NeedsSaving() const;

			int32				fDefaultValue;
			int32				fValue;
			int32				fMax;
			int32				fMin;
			const char*			fValueExpectedErrorString;
			const char*			fWrongValueErrorString;
};


class BooleanValueSetting : public ScalarValueSetting {
public:
								BooleanValueSetting(const char* name,
									bool defaultValue);
	virtual						~BooleanValueSetting();

			bool				Value() const;
	virtual	const char*			Handle(const char* const *argv);

protected:
	virtual	void				SaveSettingValue(Settings*);
};

#endif	// SETTINGS_H
