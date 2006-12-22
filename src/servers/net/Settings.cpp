/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Settings.h"

#include <Directory.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Interface templates

const static settings_template kInterfaceAddressTemplate[] = {
	{B_STRING_TYPE, "family", NULL},
	{B_STRING_TYPE, "address", NULL},
	{B_STRING_TYPE, "mask", NULL},
	{B_STRING_TYPE, "peer", NULL},
	{B_STRING_TYPE, "broadcast", NULL},
	{B_STRING_TYPE, "gateway", NULL},
	{B_BOOL_TYPE, "auto config", NULL},
	{0, NULL, NULL}
};

const static settings_template kInterfaceTemplate[] = {
	{B_STRING_TYPE, "device", NULL},
	{B_MESSAGE_TYPE, "address", kInterfaceAddressTemplate},
	{B_INT32_TYPE, "flags", NULL},
	{B_INT32_TYPE, "metric", NULL},
	{B_INT32_TYPE, "mtu", NULL},
	{0, NULL, NULL}
};

const static settings_template kInterfacesTemplate[] = {
	{B_MESSAGE_TYPE, "interface", kInterfaceTemplate},
	{0, NULL, NULL}
};

// Service templates

const static settings_template kServiceAddressTemplate[] = {
	{B_STRING_TYPE, "family", NULL},
	{B_STRING_TYPE, "address", NULL},
	{B_INT32_TYPE, "port", NULL},
	{0, NULL, NULL}
};

const static settings_template kServiceTemplate[] = {
	{B_STRING_TYPE, "name", NULL},
	{B_MESSAGE_TYPE, "address", kServiceAddressTemplate},
	{B_STRING_TYPE, "user", NULL},
	{B_STRING_TYPE, "group", NULL},
	{B_STRING_TYPE, "launch", NULL},
	{B_INT32_TYPE, "port", NULL},
	{0, NULL, NULL}
};

const static settings_template kServicesTemplate[] = {
	{B_MESSAGE_TYPE, "service", kServiceTemplate},
	{0, NULL, NULL}
};


Settings::Settings()
	:
	fUpdated(false)
{
	_Load();
}


Settings::~Settings()
{
	// only save the settings if something has changed
	if (!fUpdated)
		return;

#if 0
	BFile file;
	if (_Open(&file, B_CREATE_FILE | B_WRITE_ONLY) != B_OK)
		return;
#endif
}


status_t
Settings::_GetPath(const char* name, BPath& path)
{
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

	path.Append("network");
	create_directory(path.Path(), 0755);

	if (name != NULL)
		path.Append(name);
	return B_OK;
}


const settings_template*
Settings::_FindSettingsTemplate(const settings_template* settingsTemplate,
	const char* name)
{
	while (settingsTemplate->name != NULL) {
		if (!strcmp(name, settingsTemplate->name))
			return settingsTemplate;

		settingsTemplate++;
	}

	return NULL;
}


status_t
Settings::_ConvertFromDriverParameter(const driver_parameter& parameter,
	const settings_template* settingsTemplate, BMessage& message)
{
	settingsTemplate = _FindSettingsTemplate(settingsTemplate, parameter.name);
	if (settingsTemplate == NULL) {
		fprintf(stderr, "unknown parameter %s\n", parameter.name);
		return B_BAD_VALUE;
	}

	for (int32 i = 0; i < parameter.value_count; i++) {
		switch (settingsTemplate->type) {
			case B_STRING_TYPE:
				message.AddString(parameter.name, parameter.values[i]);
				break;
			case B_INT32_TYPE:
				message.AddInt32(parameter.name, atoi(parameter.values[i]));
				break;
			case B_BOOL_TYPE:
				if (!strcasecmp(parameter.values[i], "true")
					|| !strcasecmp(parameter.values[i], "on")
					|| !strcasecmp(parameter.values[i], "enabled")
					|| !strcasecmp(parameter.values[i], "1"))
					message.AddBool(parameter.name, true);
				else
					message.AddBool(parameter.name, false);
				break;
		}
	}

	if (settingsTemplate->type == B_MESSAGE_TYPE && parameter.parameter_count > 0) {
		status_t status = B_OK;
		BMessage subMessage;
		for (int32 j = 0; j < parameter.parameter_count; j++) {
			status = _ConvertFromDriverParameter(parameter.parameters[j],
				settingsTemplate->sub_template, subMessage);
			if (status < B_OK)
				break;
		}
		if (status == B_OK)
			message.AddMessage(parameter.name, &subMessage);
	}

	return B_OK;
}


status_t
Settings::_ConvertFromDriverSettings(const driver_settings& settings,
	const settings_template* settingsTemplate, BMessage& message)
{
	message.MakeEmpty();

	for (int32 i = 0; i < settings.parameter_count; i++) {
		status_t status = _ConvertFromDriverParameter(settings.parameters[i],
			settingsTemplate, message);
		if (status == B_BAD_VALUE) {
			// ignore unknown entries
			continue;
		}
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


status_t
Settings::_ConvertFromDriverSettings(const char* name,
	const settings_template* settingsTemplate, BMessage& message)
{
	BPath path;
	status_t status = _GetPath(name, path);
	if (status < B_OK)
		return status;

	void* handle = load_driver_settings(path.Path());
	if (handle == NULL)
		return B_ENTRY_NOT_FOUND;

	const driver_settings* settings = get_driver_settings(handle);
	if (settings != NULL)
		status = _ConvertFromDriverSettings(*settings, settingsTemplate, message);

	unload_driver_settings(handle);
	return status;
}


status_t
Settings::_Load(const char* name, uint32* _type)
{
	status_t status = B_ENTRY_NOT_FOUND;

	if (name == NULL || !strcmp(name, "interfaces")) {
		status = _ConvertFromDriverSettings("interfaces", kInterfacesTemplate,
			fInterfaces);
		if (status == B_OK && _type != NULL)
			*_type = kMsgInterfaceSettingsUpdated;
	}
	if (name == NULL || !strcmp(name, "services")) {
		status = _ConvertFromDriverSettings("services", kServicesTemplate,
			fServices);
		if (status == B_OK && _type != NULL)
			*_type = kMsgServiceSettingsUpdated;
	}

	return status;
}


status_t
Settings::_StartWatching(const char* name, const BMessenger& target)
{
	BPath path;
	status_t status = _GetPath(name, path);
	if (status < B_OK)
		return status;

	BNode node;
	status = node.SetTo(path.Path());
	if (status < B_OK)
		return status;

	node_ref ref;
	status = node.GetNodeRef(&ref);
	if (status < B_OK)
		return status;

	return watch_node(&ref, name != NULL ? B_WATCH_STAT : B_WATCH_DIRECTORY,
		target);
}


status_t
Settings::StartMonitoring(const BMessenger& target)
{
	if (_IsWatching(target))
		return B_OK;
	if (_IsWatching())
		StopMonitoring(fListener);

	fListener = target;

	status_t status = _StartWatching(NULL, target);
	if (status == B_OK)
		status = _StartWatching("interfaces", target);
	if (status == B_OK)
		status = _StartWatching("services", target);

	return status;
}


status_t
Settings::StopMonitoring(const BMessenger& target)
{
	// TODO: this needs to be changed in case the server will watch
	//	anything else but settings
	return stop_watching(target);
}


status_t
Settings::Update(BMessage* message)
{
	const char* name;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) < B_OK
		|| message->FindString("name", &name) < B_OK)
		return B_BAD_VALUE;

	if (opcode == B_ENTRY_REMOVED)
		return B_OK;

	uint32 type;
	if (_Load(name, &type) == B_OK) {
		BMessage update(type);
		fListener.SendMessage(&update);
	}

	return B_OK;
}


status_t
Settings::GetNextInterface(uint32& cookie, BMessage& interface)
{
	status_t status = fInterfaces.FindMessage("interface", cookie, &interface);
	if (status < B_OK)
		return status;

	cookie++;
	return B_OK;
}


status_t
Settings::GetNextService(uint32& cookie, BMessage& service)
{
	status_t status = fServices.FindMessage("service", cookie, &service);
	if (status < B_OK)
		return status;

	cookie++;
	return B_OK;
}


const BMessage&
Settings::Services() const
{
	return fServices;
}

