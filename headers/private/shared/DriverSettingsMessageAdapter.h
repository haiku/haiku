/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef DRIVER_SETTINGS_MESSAGE_ADAPTER_H
#define DRIVER_SETTINGS_MESSAGE_ADAPTER_H


#include <driver_settings.h>
#include <Message.h>


class DriverSettingsConverter {
public:
								DriverSettingsConverter();
	virtual						~DriverSettingsConverter();

	virtual	status_t			ConvertFromDriverSettings(
									const driver_parameter& parameter,
									const char* name, int32 index, uint32 type,
									BMessage& target);
	virtual	status_t			ConvertEmptyFromDriverSettings(
									const driver_parameter& parameter,
									const char* name, uint32 type,
									BMessage& target);

	virtual	status_t			ConvertToDriverSettings(const BMessage& source,
									const char* name, int32 index,
									uint32 type, BString& value);
};


struct settings_template {
	uint32		type;
	const char*	name;
	const settings_template* sub_template;
	bool		parent_value;
	DriverSettingsConverter* converter;
};


class DriverSettingsMessageAdapter {
public:
								DriverSettingsMessageAdapter();
								~DriverSettingsMessageAdapter();

			status_t			ConvertFromDriverSettings(
									const driver_settings& settings,
									const settings_template* settingsTemplate,
									BMessage& message);
			status_t			ConvertFromDriverSettings(const char* path,
									const settings_template* settingsTemplate,
									BMessage& message);

			status_t			ConvertToDriverSettings(
									const settings_template* settingsTemplate,
									BString& settings, const BMessage& message);
			status_t			ConvertToDriverSettings(const char* path,
									const settings_template* settingsTemplate,
									const BMessage& message);

private:
			const settings_template* _FindSettingsTemplate(
									const settings_template* settingsTemplate,
									const char* name);
			const settings_template* _FindParentValueTemplate(
									const settings_template* settingsTemplate);
			status_t			_AddParameter(const driver_parameter& parameter,
									const settings_template& settingsTemplate,
									BMessage& message);

			status_t			_ConvertFromDriverParameter(
									const driver_parameter& parameter,
									const settings_template* settingsTemplate,
									BMessage& message);

			status_t			_AppendSettings(
									const settings_template* settingsTemplate,
									BString& settings, const BMessage& message,
									const char* name, type_code type,
									int32 count,
									const char* settingName = NULL);

};


#endif	// DRIVER_SETTINGS_MESSAGE_ADAPTER_H
