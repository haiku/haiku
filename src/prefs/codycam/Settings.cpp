#include <Debug.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Settings.h"

Settings *settings = NULL;

// generic setting handler classes


StringValueSetting::StringValueSetting(const char *name, const char *defaultValue,
	const char *valueExpectedErrorString, const char *wrongValueErrorString)
	:	SettingsArgvDispatcher(name),
		defaultValue(defaultValue),
		valueExpectedErrorString(valueExpectedErrorString),
		wrongValueErrorString(wrongValueErrorString),
		value(strdup(defaultValue))
{
}


StringValueSetting::~StringValueSetting()
{
	free(value);
}

void 
StringValueSetting::ValueChanged(const char *newValue)
{
	if (newValue == value)
		// guard against self assingment
		return;
		
	free(value);
	value = strdup(newValue);
}

const char *
StringValueSetting::Value() const
{
	return value;
}

void 
StringValueSetting::SaveSettingValue(Settings *settings)
{
printf("-------StringValueSetting::SaveSettingValue %s %s\n", Name(), value);
	settings->Write("\"%s\"", value);
}

bool 
StringValueSetting::NeedsSaving() const
{
	// needs saving if different than default
	return strcmp(value, defaultValue) != 0;
}

const char *
StringValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return valueExpectedErrorString;
	
	ValueChanged(*argv);	
	return 0;
}

EnumeratedStringValueSetting::EnumeratedStringValueSetting(const char *name,
	const char *defaultValue, const char *const *values, const char *valueExpectedErrorString,
	const char *wrongValueErrorString)
	:	StringValueSetting(name, defaultValue, valueExpectedErrorString, wrongValueErrorString),
		values(values)
{
}

void 
EnumeratedStringValueSetting::ValueChanged(const char *newValue)
{
#if DEBUG
	// must be one of the enumerated values
	bool found = false;
	for (int32 index = 0; ; index++) {
		if (!values[index])
			break;
		if (strcmp(values[index], newValue) != 0) 
			continue;
		found = true;
		break;
	}
	ASSERT(found);
#endif
	StringValueSetting::ValueChanged(newValue);
}

const char *
EnumeratedStringValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return valueExpectedErrorString;

printf("-----EnumeratedStringValueSetting::Handle %s %s\n", *(argv-1), *argv);
	bool found = false;
	for (int32 index = 0; ; index++) {
		if (!values[index])
			break;
		if (strcmp(values[index], *argv) != 0) 
			continue;
		found = true;
		break;
	}	
				
	if (!found)
		return wrongValueErrorString;
	
	ValueChanged(*argv);	
	return 0;
}

ScalarValueSetting::ScalarValueSetting(const char *name, int32 defaultValue,
	const char *valueExpectedErrorString, const char *wrongValueErrorString,
	int32 min, int32 max)
	:	SettingsArgvDispatcher(name),
		defaultValue(defaultValue),
		value(defaultValue),
		max(max),
		min(min),
		valueExpectedErrorString(valueExpectedErrorString),
		wrongValueErrorString(wrongValueErrorString)
{
}

void 
ScalarValueSetting::ValueChanged(int32 newValue)
{
	ASSERT(newValue > min);
	ASSERT(newValue < max);
	value = newValue;
}

int32
ScalarValueSetting::Value() const
{
	return value;
}

void 
ScalarValueSetting::GetValueAsString(char *buffer) const
{
	sprintf(buffer, "%ld", value);
}

const char *
ScalarValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return valueExpectedErrorString;

	int32 newValue = atoi(*argv);
	if (newValue < min || newValue > max)
		return wrongValueErrorString;
	
	value = newValue;	
	return 0;
}

void 
ScalarValueSetting::SaveSettingValue(Settings *settings)
{
	settings->Write("%d", value);
}

bool 
ScalarValueSetting::NeedsSaving() const
{
	return value != defaultValue;
}


BooleanValueSetting::BooleanValueSetting(const char *name, bool defaultValue )
	:	ScalarValueSetting(name, defaultValue, 0, 0)
{
}

bool 
BooleanValueSetting::Value() const
{
	return value;
}

const char *
BooleanValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return "or or off expected";

	if (strcmp(*argv, "on") == 0)
		value = true;
	else if (strcmp(*argv, "off") == 0)
		value = false;
	else
		return "or or off expected";

	return 0;
}

void 
BooleanValueSetting::SaveSettingValue(Settings *settings)
{
	settings->Write(value ? "on" : "off");
}

