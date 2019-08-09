/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <Debug.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Settings"


Settings* settings = NULL;


StringValueSetting::StringValueSetting(const char* name,
	const char* defaultValue, const char* valueExpectedErrorString,
	const char* wrongValueErrorString)
	:
	SettingsArgvDispatcher(name),
	fDefaultValue(defaultValue),
	fValueExpectedErrorString(valueExpectedErrorString),
	fWrongValueErrorString(wrongValueErrorString),
	fValue(strdup(defaultValue))
{
}


StringValueSetting::~StringValueSetting()
{
	free(fValue);
}


void
StringValueSetting::ValueChanged(const char* newValue)
{
	if (newValue == fValue)
		// guard against self assingment
		return;

	free(fValue);
	fValue = strdup(newValue);
}


const char*
StringValueSetting::Value() const
{
	return fValue;
}


void
StringValueSetting::SaveSettingValue(Settings* settings)
{
	printf("-----StringValueSetting::SaveSettingValue %s %s\n", Name(), fValue);
	settings->Write("\"%s\"", fValue);
}


bool
StringValueSetting::NeedsSaving() const
{
	// needs saving if different than default
	return strcmp(fValue, fDefaultValue) != 0;
}


const char*
StringValueSetting::Handle(const char* const *argv)
{
	if (*++argv == NULL)
		return fValueExpectedErrorString;

	ValueChanged(*argv);
	return 0;
}


//	#pragma mark -


EnumeratedStringValueSetting::EnumeratedStringValueSetting(const char* name,
	const char* defaultValue, StringEnumerator enumerator,
	const char* valueExpectedErrorString,
	const char* wrongValueErrorString)
	:
	StringValueSetting(name, defaultValue, valueExpectedErrorString,
		wrongValueErrorString),
	fEnumerator(enumerator)
{
}


void
EnumeratedStringValueSetting::ValueChanged(const char* newValue)
{
#if DEBUG
	// must be one of the enumerated values
	ASSERT(_ValidateString(newValue));
#endif
	StringValueSetting::ValueChanged(newValue);
}


const char*
EnumeratedStringValueSetting::Handle(const char* const *argv)
{
	if (*++argv == NULL)
		return fValueExpectedErrorString;

	printf("---EnumeratedStringValueSetting::Handle %s %s\n", *(argv-1), *argv);
	if (!_ValidateString(*argv))
		return fWrongValueErrorString;

	ValueChanged(*argv);
	return 0;
}


bool
EnumeratedStringValueSetting::_ValidateString(const char* string)
{
	for (int32 i = 0;; i++) {
		const char* enumString = fEnumerator(i);
		if (enumString == NULL)
			return false;
		if (strcmp(enumString, string) == 0)
			return true;
	}
	return false;
}


//	#pragma mark -


ScalarValueSetting::ScalarValueSetting(const char* name, int32 defaultValue,
	const char* valueExpectedErrorString, const char* wrongValueErrorString,
	int32 min, int32 max)
	:
	SettingsArgvDispatcher(name),
	fDefaultValue(defaultValue),
	fValue(defaultValue),
	fMax(max),
	fMin(min),
	fValueExpectedErrorString(valueExpectedErrorString),
	fWrongValueErrorString(wrongValueErrorString)
{
}


ScalarValueSetting::~ScalarValueSetting()
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
ScalarValueSetting::Handle(const char* const *argv)
{
	if (*++argv == NULL)
		return fValueExpectedErrorString;

	int32 newValue = atoi(*argv);
	if (newValue < fMin || newValue > fMax)
		return fWrongValueErrorString;

	fValue = newValue;
	return 0;
}


void
ScalarValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write("%" B_PRId32, fValue);
}


bool
ScalarValueSetting::NeedsSaving() const
{
	return fValue != fDefaultValue;
}


//	#pragma mark -


BooleanValueSetting::BooleanValueSetting(const char* name, bool defaultValue)
	:
	ScalarValueSetting(name, defaultValue, 0, 0)
{
}


BooleanValueSetting::~BooleanValueSetting()
{
}


bool
BooleanValueSetting::Value() const
{
	return fValue;
}


const char*
BooleanValueSetting::Handle(const char* const *argv)
{
	if (*++argv == NULL) {
		return B_TRANSLATE_COMMENT("on or off expected","Do not translate "
			"'on' and 'off'");
	}

	if (strcmp(*argv, "on") == 0)
		fValue = true;
	else if (strcmp(*argv, "off") == 0)
		fValue = false;
	else {
		return B_TRANSLATE_COMMENT("on or off expected", "Do not translate "
		"'on' and 'off'");
	}

	return 0;
}


void
BooleanValueSetting::SaveSettingValue(Settings* settings)
{
	settings->Write(fValue ? "on" : "off");
}
