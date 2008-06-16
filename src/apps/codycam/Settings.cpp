#include "Settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>


Settings* settings = NULL;


StringValueSetting::StringValueSetting(const char* name, const char* defaultValue,
	const char* valueExpectedErrorString, const char* wrongValueErrorString)
	: SettingsArgvDispatcher(name),
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
	printf("-------StringValueSetting::SaveSettingValue %s %s\n", Name(), fValue);
	settings->Write("\"%s\"", fValue);
}


bool 
StringValueSetting::NeedsSaving() const
{
	// needs saving if different than default
	return strcmp(fValue, fDefaultValue) != 0;
}


const char*
StringValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return fValueExpectedErrorString;
	
	ValueChanged(*argv);	
	return 0;
}


//	#pragma mark -


EnumeratedStringValueSetting::EnumeratedStringValueSetting(const char* name,
	const char* defaultValue, const char *const *values,
	const char* valueExpectedErrorString,
	const char* wrongValueErrorString)
	: StringValueSetting(name, defaultValue, valueExpectedErrorString, wrongValueErrorString),
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
EnumeratedStringValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
		return fValueExpectedErrorString;

	printf("-----EnumeratedStringValueSetting::Handle %s %s\n", *(argv-1), *argv);
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
	: SettingsArgvDispatcher(name),
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
	sprintf(buffer, "%ld", fValue);
}


const char*
ScalarValueSetting::Handle(const char *const *argv)
{
	if (!*++argv) 
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
	settings->Write("%d", fValue);
}


bool 
ScalarValueSetting::NeedsSaving() const
{
	return fValue != fDefaultValue;
}


//	#pragma mark -


BooleanValueSetting::BooleanValueSetting(const char* name, bool defaultValue)
	: ScalarValueSetting(name, defaultValue, 0, 0)
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
BooleanValueSetting::Handle(const char *const *argv)
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
