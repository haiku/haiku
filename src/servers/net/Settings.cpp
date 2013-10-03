/*
 * Copyright 2006-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_interface.h>
#include <Path.h>
#include <PathMonitor.h>
#include <String.h>

#include <DriverSettingsMessageAdapter.h>


// Interface templates

const static settings_template kInterfaceAddressTemplate[] = {
	{B_STRING_TYPE, "family", NULL, true},
	{B_STRING_TYPE, "address", NULL},
	{B_STRING_TYPE, "mask", NULL},
	{B_STRING_TYPE, "peer", NULL},
	{B_STRING_TYPE, "broadcast", NULL},
	{B_STRING_TYPE, "gateway", NULL},
	{B_BOOL_TYPE, "auto_config", NULL},
	{0, NULL, NULL}
};

const static settings_template kInterfaceNetworkTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_STRING_TYPE, "mac", NULL},
};

const static settings_template kInterfaceTemplate[] = {
	{B_STRING_TYPE, "device", NULL, true},
	{B_BOOL_TYPE, "disabled", NULL},
	{B_MESSAGE_TYPE, "address", kInterfaceAddressTemplate},
	{B_MESSAGE_TYPE, "network", kInterfaceNetworkTemplate},
	{B_INT32_TYPE, "flags", NULL},
	{B_INT32_TYPE, "metric", NULL},
	{B_INT32_TYPE, "mtu", NULL},
	{0, NULL, NULL}
};

const static settings_template kInterfacesTemplate[] = {
	{B_MESSAGE_TYPE, "interface", kInterfaceTemplate},
	{0, NULL, NULL}
};

// Network templates

const static settings_template kNetworkTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_STRING_TYPE, "mac", NULL},
	{B_STRING_TYPE, "password", NULL},
	{B_STRING_TYPE, "authentication", NULL},
	{B_STRING_TYPE, "cipher", NULL},
	{B_STRING_TYPE, "group_cipher", NULL},
	{B_STRING_TYPE, "key", NULL},
	{0, NULL, NULL}
};

const static settings_template kNetworksTemplate[] = {
	{B_MESSAGE_TYPE, "network", kNetworkTemplate},
	{0, NULL, NULL}
};

// Service templates

const static settings_template kServiceAddressTemplate[] = {
	{B_STRING_TYPE, "family", NULL, true},
	{B_STRING_TYPE, "type", NULL},
	{B_STRING_TYPE, "protocol", NULL},
	{B_STRING_TYPE, "address", NULL},
	{B_INT32_TYPE, "port", NULL},
	{0, NULL, NULL}
};

const static settings_template kServiceTemplate[] = {
	{B_STRING_TYPE, "name", NULL, true},
	{B_MESSAGE_TYPE, "address", kServiceAddressTemplate},
	{B_STRING_TYPE, "user", NULL},
	{B_STRING_TYPE, "group", NULL},
	{B_STRING_TYPE, "launch", NULL},
	{B_STRING_TYPE, "family", NULL},
	{B_STRING_TYPE, "type", NULL},
	{B_STRING_TYPE, "protocol", NULL},
	{B_INT32_TYPE, "port", NULL},
	{B_BOOL_TYPE, "stand_alone", NULL},
	{0, NULL, NULL}
};

const static settings_template kServicesTemplate[] = {
	{B_MESSAGE_TYPE, "service", kServiceTemplate},
	{0, NULL, NULL}
};


Settings::Settings()
{
	_Load();
}


Settings::~Settings()
{
}


status_t
Settings::GetNextInterface(uint32& cookie, BMessage& interface)
{
	status_t status = fInterfaces.FindMessage("interface", cookie, &interface);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


int32
Settings::CountNetworks() const
{
	int32 count = 0;
	if (fNetworks.GetInfo("network", NULL, &count) != B_OK)
		return 0;

	return count;
}


status_t
Settings::GetNextNetwork(uint32& cookie, BMessage& network) const
{
	status_t status = fNetworks.FindMessage("network", cookie, &network);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


status_t
Settings::AddNetwork(const BMessage& network)
{
	const char* name = NULL;
	network.FindString("name", &name);
	RemoveNetwork(name);

	status_t result = fNetworks.AddMessage("network", &network);
	if (result != B_OK)
		return result;

	return _Save("wireless_networks");
}


status_t
Settings::RemoveNetwork(const char* name)
{
	int32 index = 0;
	BMessage network;
	while (fNetworks.FindMessage("network", index, &network) == B_OK) {
		const char* networkName = NULL;
		if (network.FindString("name", &networkName) == B_OK
			&& strcmp(networkName, name) == 0) {
			fNetworks.RemoveData("network", index);
			return _Save("wireless_networks");
		}

		index++;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Settings::GetNextService(uint32& cookie, BMessage& service)
{
	status_t status = fServices.FindMessage("service", cookie, &service);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


const BMessage&
Settings::Services() const
{
	return fServices;
}


status_t
Settings::StartMonitoring(const BMessenger& target)
{
	if (_IsWatching(target))
		return B_OK;
	if (_IsWatching())
		StopMonitoring(fListener);

	fListener = target;

	status_t status = _StartWatching("interfaces", target);
	if (status == B_OK)
		status = _StartWatching("wireless_networks", target);
	if (status == B_OK)
		status = _StartWatching("services", target);

	return status;
}


status_t
Settings::StopMonitoring(const BMessenger& target)
{
	// TODO: this needs to be changed in case the server will watch
	//	anything else but settings
	return BPrivate::BPathMonitor::StopWatching(target);
}


status_t
Settings::Update(BMessage* message)
{
	const char* pathName;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| message->FindString("path", &pathName) != B_OK)
		return B_BAD_VALUE;

	BPath settingsFolderPath;
	_GetPath(NULL, settingsFolderPath);
	if (strncmp(pathName, settingsFolderPath.Path(),
			strlen(settingsFolderPath.Path()))) {
		return B_NAME_NOT_FOUND;
	}

	if (message->FindBool("removed")) {
		// for now, we only consider existing settings files
		// (ie. deleting "services" won't stop any)
		return B_OK;
	}

	int32 fields;
	if (opcode == B_STAT_CHANGED
		&& message->FindInt32("fields", &fields) == B_OK
		&& (fields & (B_STAT_MODIFICATION_TIME | B_STAT_SIZE)) == 0) {
		// only update when the modified time or size has changed
		return B_OK;
	}

	BPath path(pathName);
	uint32 type;
	if (_Load(path.Leaf(), &type) == B_OK) {
		BMessage update(type);
		fListener.SendMessage(&update);
	}

	return B_OK;
}


// #pragma mark - private


status_t
Settings::_Load(const char* name, uint32* _type)
{
	BPath path;
	status_t status = _GetPath(NULL, path);
	if (status != B_OK)
		return status;

	DriverSettingsMessageAdapter adapter;
	status = B_ENTRY_NOT_FOUND;

	if (name == NULL || strcmp(name, "interfaces") == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, "interfaces").Path(), kInterfacesTemplate, fInterfaces);
		if (status == B_OK && _type != NULL)
			*_type = kMsgInterfaceSettingsUpdated;
	}
	if (name == NULL || strcmp(name, "wireless_networks") == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, "wireless_networks").Path(),
			kNetworksTemplate, fNetworks);
		if (status == B_OK && _type != NULL)
			*_type = kMsgInterfaceSettingsUpdated;
	}
	if (name == NULL || strcmp(name, "services") == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, "services").Path(), kServicesTemplate, fServices);
		if (status == B_OK && _type != NULL)
			*_type = kMsgServiceSettingsUpdated;
	}

	return status;
}


status_t
Settings::_Save(const char* name)
{
	BPath path;
	status_t status = _GetPath(NULL, path);
	if (status != B_OK)
		return status;

	DriverSettingsMessageAdapter adapter;
	status = B_ENTRY_NOT_FOUND;

	if (name == NULL || strcmp(name, "interfaces") == 0) {
		status = adapter.ConvertToDriverSettings(
			_Path(path, "interfaces").Path(), kInterfacesTemplate, fInterfaces);
	}
	if (name == NULL || strcmp(name, "wireless_networks") == 0) {
		status = adapter.ConvertToDriverSettings(
			_Path(path, "wireless_networks").Path(),
			kNetworksTemplate, fNetworks);
	}
	if (name == NULL || strcmp(name, "services") == 0) {
		status = adapter.ConvertToDriverSettings(_Path(path, "services").Path(),
			kServicesTemplate, fServices);
	}

	return status;
}


BPath
Settings::_Path(BPath& parent, const char* leaf)
{
	return BPath(parent.Path(), leaf);
}


status_t
Settings::_GetPath(const char* name, BPath& path)
{
	if (find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

	path.Append("network");
	create_directory(path.Path(), 0755);

	if (name != NULL)
		path.Append(name);
	return B_OK;
}


status_t
Settings::_StartWatching(const char* name, const BMessenger& target)
{
	BPath path;
	status_t status = _GetPath(name, path);
	if (status != B_OK)
		return status;

	return BPrivate::BPathMonitor::StartWatching(path.Path(), B_WATCH_STAT,
		target);
}

