/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsParser.h"

#include <DriverSettingsMessageAdapter.h>


const static settings_template kPortTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_INT32_TYPE, "capacity", NULL},
};

const static settings_template kJobTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_BOOL_TYPE, "disabled", NULL},
	{B_STRING_TYPE, "launch", NULL},
	{B_STRING_TYPE, "requires", NULL},
	{B_BOOL_TYPE, "legacy", NULL},
	{B_MESSAGE_TYPE, "port", kPortTemplate},
	{B_BOOL_TYPE, "no_safemode", NULL},
	{0, NULL, NULL}
};

const static settings_template kConditionTemplate[] = {
	{B_STRING_TYPE, NULL, NULL, true},
	{B_MESSAGE_TYPE, "not", kConditionTemplate},
	{B_MESSAGE_TYPE, "and", kConditionTemplate},
	{B_MESSAGE_TYPE, "or", kConditionTemplate},
	{0, NULL, NULL}
};

const static settings_template kTargetTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_BOOL_TYPE, "reset", NULL},
	{B_MESSAGE_TYPE, "if", kConditionTemplate},
	{B_MESSAGE_TYPE, "job", kJobTemplate},
	{B_MESSAGE_TYPE, "service", kJobTemplate},
	{0, NULL, NULL}
};

const static settings_template kSettingsTemplate[] = {
	{B_MESSAGE_TYPE, "target", kTargetTemplate},
	{B_MESSAGE_TYPE, "job", kJobTemplate},
	{B_MESSAGE_TYPE, "service", kJobTemplate},
	{0, NULL, NULL}
};


SettingsParser::SettingsParser()
{
}


status_t
SettingsParser::ParseFile(const char* path, BMessage& settings)
{
	DriverSettingsMessageAdapter adapter;
	return adapter.ConvertFromDriverSettings(path, kSettingsTemplate, settings);
}
