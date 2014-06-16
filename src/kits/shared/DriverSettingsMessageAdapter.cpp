/*
 * Copyright 2006-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz <mmlr@mlotz.ch>
 */


#include "DriverSettingsMessageAdapter.h"

#include <stdio.h>
#include <stdlib.h>

#include <File.h>
#include <String.h>


DriverSettingsMessageAdapter::DriverSettingsMessageAdapter()
{
}


DriverSettingsMessageAdapter::~DriverSettingsMessageAdapter()
{
}


status_t
DriverSettingsMessageAdapter::ConvertFromDriverSettings(
	const driver_settings& settings, const settings_template* settingsTemplate,
	BMessage& message)
{
	message.MakeEmpty();

	for (int32 i = 0; i < settings.parameter_count; i++) {
		status_t status = _ConvertFromDriverParameter(settings.parameters[i],
			settingsTemplate, message);
		if (status == B_BAD_VALUE) {
			// ignore unknown entries
			continue;
		}
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
DriverSettingsMessageAdapter::ConvertFromDriverSettings(const char* path,
	const settings_template* settingsTemplate, BMessage& message)
{
	void* handle = load_driver_settings(path);
	if (handle == NULL)
		return B_ENTRY_NOT_FOUND;

	const driver_settings* settings = get_driver_settings(handle);
	status_t status;
	if (settings != NULL) {
		status = ConvertFromDriverSettings(*settings, settingsTemplate,
			message);
	} else
		status = B_BAD_DATA;

	unload_driver_settings(handle);
	return status;
}


status_t
DriverSettingsMessageAdapter::ConvertToDriverSettings(
	const settings_template* settingsTemplate, BString& settings,
	const BMessage& message)
{
	int32 index = 0;
	char *name = NULL;
	type_code type;
	int32 count = 0;

	while (message.GetInfo(B_ANY_TYPE, index++, &name, &type, &count) == B_OK) {
		status_t result = _AppendSettings(settingsTemplate, settings, message,
			name, type, count);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
DriverSettingsMessageAdapter::ConvertToDriverSettings(const char* path,
	const settings_template* settingsTemplate, const BMessage& message)
{
	BString settings;
	status_t status = ConvertToDriverSettings(settingsTemplate, settings,
		message);
	if (status != B_OK)
		return status;

	settings.RemoveFirst("\n");
	BFile settingsFile(path, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);

	ssize_t written = settingsFile.Write(settings.String(), settings.Length());
	if (written < 0)
		return written;

	return written == settings.Length() ? B_OK : B_ERROR;
}


// #pragma mark -


const settings_template*
DriverSettingsMessageAdapter::_FindSettingsTemplate(
	const settings_template* settingsTemplate, const char* name)
{
	while (settingsTemplate->name != NULL) {
		if (!strcmp(name, settingsTemplate->name))
			return settingsTemplate;

		settingsTemplate++;
	}

	return NULL;
}


const settings_template*
DriverSettingsMessageAdapter::_FindParentValueTemplate(
	const settings_template* settingsTemplate)
{
	settingsTemplate = settingsTemplate->sub_template;
	if (settingsTemplate == NULL)
		return NULL;

	while (settingsTemplate->name != NULL) {
		if (settingsTemplate->parent_value)
			return settingsTemplate;

		settingsTemplate++;
	}

	return NULL;
}


status_t
DriverSettingsMessageAdapter::_AddParameter(const driver_parameter& parameter,
	const char* name, uint32 type, BMessage& message)
{
	for (int32 i = 0; i < parameter.value_count; i++) {
		switch (type) {
			case B_STRING_TYPE:
				message.AddString(name, parameter.values[i]);
				break;
			case B_INT32_TYPE:
				message.AddInt32(name, atoi(parameter.values[i]));
				break;
			case B_BOOL_TYPE:
				if (!strcasecmp(parameter.values[i], "true")
					|| !strcasecmp(parameter.values[i], "on")
					|| !strcasecmp(parameter.values[i], "enabled")
					|| !strcasecmp(parameter.values[i], "1"))
					message.AddBool(name, true);
				else
					message.AddBool(name, false);
				break;
		}
	}
	if (type == B_BOOL_TYPE && parameter.value_count == 0) {
		// boolean parameters are always true
		message.AddBool(name, true);
	}

	return B_OK;
}


status_t
DriverSettingsMessageAdapter::_ConvertFromDriverParameter(
	const driver_parameter& parameter,
	const settings_template* settingsTemplate, BMessage& message)
{
	settingsTemplate = _FindSettingsTemplate(settingsTemplate, parameter.name);
	if (settingsTemplate == NULL) {
		fprintf(stderr, "unknown parameter %s\n", parameter.name);
		return B_BAD_VALUE;
	}

	_AddParameter(parameter, parameter.name, settingsTemplate->type, message);

	if (settingsTemplate->type == B_MESSAGE_TYPE
		&& parameter.parameter_count > 0) {
		status_t status = B_OK;
		BMessage subMessage;
		for (int32 j = 0; j < parameter.parameter_count; j++) {
			status = _ConvertFromDriverParameter(parameter.parameters[j],
				settingsTemplate->sub_template, subMessage);
			if (status != B_OK)
				break;

			const settings_template* parentValueTemplate
				= _FindParentValueTemplate(settingsTemplate);
			if (parentValueTemplate != NULL) {
				_AddParameter(parameter, parentValueTemplate->name,
					parentValueTemplate->type, subMessage);
			}
		}
		if (status == B_OK)
			message.AddMessage(parameter.name, &subMessage);
	}

	return B_OK;
}


status_t
DriverSettingsMessageAdapter::_AppendSettings(
	const settings_template* settingsTemplate, BString& settings,
	const BMessage& message, const char* name, type_code type, int32 count,
	const char* settingName)
{
	const settings_template* valueTemplate
		= _FindSettingsTemplate(settingsTemplate, name);
	if (valueTemplate == NULL) {
		fprintf(stderr, "unknown field %s\n", name);
		return B_BAD_VALUE;
	}

	if (valueTemplate->type != type) {
		fprintf(stderr, "field type mismatch %s\n", name);
		return B_BAD_VALUE;
	}

	if (settingName == NULL)
		settingName = name;

	if (type != B_MESSAGE_TYPE) {
		settings.Append("\n");
		settings.Append(settingName);
		settings.Append("\t");
	}

	for (int32 valueIndex = 0; valueIndex < count; valueIndex++) {
		if (valueIndex > 0 && type != B_MESSAGE_TYPE)
			settings.Append(" ");

		switch (type) {
			case B_BOOL_TYPE:
			{
				bool value;
				status_t result = message.FindBool(name, valueIndex, &value);
				if (result != B_OK)
					return result;

				settings.Append(value ? "true" : "false");
				break;
			}

			case B_STRING_TYPE:
			{
				const char* value = NULL;
				status_t result = message.FindString(name, valueIndex, &value);
				if (result != B_OK)
					return result;

				settings.Append(value);
				break;
			}

			case B_INT32_TYPE:
			{
				int32 value;
				status_t result = message.FindInt32(name, valueIndex, &value);
				if (result != B_OK)
					return result;

				char buffer[100];
				snprintf(buffer, sizeof(buffer), "%" B_PRId32, value);
				settings.Append(buffer, sizeof(buffer));
				break;
			}

			case B_MESSAGE_TYPE:
			{
				BMessage subMessage;
				status_t result = message.FindMessage(name, valueIndex,
					&subMessage);
				if (result != B_OK)
					return result;

				const settings_template* parentValueTemplate
					= _FindParentValueTemplate(valueTemplate);
				if (parentValueTemplate != NULL) {
					_AppendSettings(valueTemplate->sub_template, settings,
						subMessage, parentValueTemplate->name,
						parentValueTemplate->type, 1, name);
					subMessage.RemoveName(parentValueTemplate->name);
				}

				BString subSettings;
				ConvertToDriverSettings(valueTemplate->sub_template,
					subSettings, subMessage);
				subSettings.ReplaceAll("\n", "\n\t");
				subSettings.RemoveFirst("\n");

				settings.Append(" {\n");
				settings.Append(subSettings);
				settings.Append("\n}");
			}
		}
	}

	return B_OK;
}
