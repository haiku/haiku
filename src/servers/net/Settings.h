/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <driver_settings.h>
#include <Message.h>
#include <Messenger.h>


class BPath;
struct settings_template;


class Settings {
public:
								Settings();
								~Settings();

			status_t			GetNextInterface(uint32& cookie,
									BMessage& interface);

			status_t			GetNextNetwork(uint32& cookie,
									BMessage& network);
			status_t			AddNetwork(const BMessage& network);

			status_t			GetNextService(uint32& cookie,
									BMessage& service);
			const BMessage&		Services() const;

			status_t			StartMonitoring(const BMessenger& target);
			status_t			StopMonitoring(const BMessenger& target);

			status_t			Update(BMessage* message);

private:
			status_t			_Load(const char* name = NULL,
									uint32* _type = NULL);
			status_t			_Save(const char* name = NULL);
			status_t			_GetPath(const char* name, BPath& path);

			status_t			_StartWatching(const char* name,
									const BMessenger& target);
			const settings_template* _FindSettingsTemplate(
									const settings_template* settingsTemplate,
									const char* name);
			const settings_template* _FindParentValueTemplate(
									const settings_template* settingsTemplate);
			status_t			_AddParameter(const driver_parameter& parameter,
									const char* name, uint32 type,
									BMessage& message);

			status_t			_ConvertFromDriverParameter(
									const driver_parameter& parameter,
									const settings_template* settingsTemplate,
									BMessage& message);
			status_t			_ConvertFromDriverSettings(
									const driver_settings& settings,
									const settings_template* settingsTemplate,
									BMessage& message);
			status_t			_ConvertFromDriverSettings(const char* path,
									const settings_template* settingsTemplate,
									BMessage& message);

			status_t			_AppendSettings(
									const settings_template* settingsTemplate,
									BString& settings, const BMessage& message,
									const char* name, type_code type,
									int32 count,
									const char* settingName = NULL);
			status_t			_ConvertToDriverSettings(
									const settings_template* settingsTemplate,
									BString& settings, const BMessage& message);
			status_t			_ConvertToDriverSettings(const char* path,
									const settings_template* settingsTemplate,
									const BMessage& message);

			bool				_IsWatching(const BMessenger& target) const
									{ return fListener == target; }
			bool				_IsWatching() const
									{ return fListener.IsValid(); }

private:
			BMessenger			fListener;
			BMessage			fInterfaces;
			BMessage			fNetworks;
			BMessage			fServices;
};


static const uint32 kMsgInterfaceSettingsUpdated = 'SUif';
static const uint32 kMsgServiceSettingsUpdated = 'SUsv';


#endif	// SETTINGS_H
