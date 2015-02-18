/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <NetworkSettings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_interface.h>
#include <NetworkDevice.h>
#include <Path.h>
#include <PathMonitor.h>
#include <String.h>

#include <DriverSettingsMessageAdapter.h>


using namespace BNetworkKit;


static const char* kInterfaceSettingsName = "interfaces";
static const char* kServicesSettingsName = "services";
static const char* kNetworksSettingsName = "wireless_networks";

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


BNetworkSettings::BNetworkSettings()
{
	_Load();
}


BNetworkSettings::~BNetworkSettings()
{
}


status_t
BNetworkSettings::GetNextInterface(uint32& cookie, BMessage& interface)
{
	status_t status = fInterfaces.FindMessage("interface", cookie, &interface);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


status_t
BNetworkSettings::AddInterface(const BMessage& interface)
{
	const char* name = NULL;
	if (interface.FindString("device", &name) != B_OK)
		return B_BAD_VALUE;

	_RemoveItem(fInterfaces, "interface", "device", name, NULL);

	status_t result = fInterfaces.AddMessage("interface", &interface);
	if (result != B_OK)
		return result;

	return _Save(kInterfaceSettingsName);
}


status_t
BNetworkSettings::RemoveInterface(const char* name)
{
	return _RemoveItem(fInterfaces, "interface", "device", name,
		kInterfaceSettingsName);
}


int32
BNetworkSettings::CountNetworks() const
{
	int32 count = 0;
	if (fNetworks.GetInfo("network", NULL, &count) != B_OK)
		return 0;

	return count;
}


status_t
BNetworkSettings::GetNextNetwork(uint32& cookie, BMessage& network) const
{
	status_t status = fNetworks.FindMessage("network", cookie, &network);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


status_t
BNetworkSettings::AddNetwork(const BMessage& network)
{
	const char* name = NULL;
	if (network.FindString("name", &name) != B_OK)
		return B_BAD_VALUE;

	_RemoveItem(fNetworks, "network", "name", name, NULL);

	status_t result = fNetworks.AddMessage("network", &network);
	if (result != B_OK)
		return result;

	return _Save(kNetworksSettingsName);
}


status_t
BNetworkSettings::RemoveNetwork(const char* name)
{
	return _RemoveItem(fNetworks, "network", "name", name,
		kNetworksSettingsName);
}


status_t
BNetworkSettings::GetNextService(uint32& cookie, BMessage& service)
{
	status_t status = fServices.FindMessage("service", cookie, &service);
	if (status != B_OK)
		return status;

	cookie++;
	return B_OK;
}


const BMessage&
BNetworkSettings::Services() const
{
	return fServices;
}


status_t
BNetworkSettings::AddService(const BMessage& service)
{
	const char* name = NULL;
	if (service.FindString("name", &name) != B_OK)
		return B_BAD_VALUE;

	_RemoveItem(fServices, "service", "name", name, NULL);

	status_t result = fServices.AddMessage("service", &service);
	if (result != B_OK)
		return result;

	return _Save(kServicesSettingsName);
}


status_t
BNetworkSettings::RemoveService(const char* name)
{
	return _RemoveItem(fServices, "service", "name", name,
		kServicesSettingsName);
}


status_t
BNetworkSettings::StartMonitoring(const BMessenger& target)
{
	if (_IsWatching(target))
		return B_OK;
	if (_IsWatching())
		StopMonitoring(fListener);

	fListener = target;

	status_t status = _StartWatching(kInterfaceSettingsName, target);
	if (status == B_OK)
		status = _StartWatching(kNetworksSettingsName, target);
	if (status == B_OK)
		status = _StartWatching(kServicesSettingsName, target);

	return status;
}


status_t
BNetworkSettings::StopMonitoring(const BMessenger& target)
{
	// TODO: this needs to be changed in case the server will watch
	//	anything else but settings
	return BPrivate::BPathMonitor::StopWatching(target);
}


status_t
BNetworkSettings::Update(BMessage* message)
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
BNetworkSettings::_Load(const char* name, uint32* _type)
{
	BPath path;
	status_t status = _GetPath(NULL, path);
	if (status != B_OK)
		return status;

	DriverSettingsMessageAdapter adapter;
	status = B_ENTRY_NOT_FOUND;

	if (name == NULL || strcmp(name, kInterfaceSettingsName) == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, kInterfaceSettingsName).Path(), kInterfacesTemplate,
			fInterfaces);
		if (status == B_OK && _type != NULL)
			*_type = kMsgInterfaceSettingsUpdated;
	}
	if (name == NULL || strcmp(name, kNetworksSettingsName) == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, kNetworksSettingsName).Path(),
			kNetworksTemplate, fNetworks);
		if (status == B_OK) {
			// Convert settings for simpler consumption
			BMessage network;
			for (int32 index = 0; fNetworks.FindMessage("network", index,
					&network); index++) {
				if (_ConvertNetworkFromSettings(network) == B_OK)
					fNetworks.ReplaceMessage("network", index, &network);
			}

			if (_type != NULL)
				*_type = kMsgInterfaceSettingsUpdated;
		}
	}
	if (name == NULL || strcmp(name, kServicesSettingsName) == 0) {
		status = adapter.ConvertFromDriverSettings(
			_Path(path, kServicesSettingsName).Path(), kServicesTemplate,
			fServices);
		if (status == B_OK && _type != NULL)
			*_type = kMsgServiceSettingsUpdated;
	}

	return status;
}


status_t
BNetworkSettings::_Save(const char* name)
{
	BPath path;
	status_t status = _GetPath(NULL, path);
	if (status != B_OK)
		return status;

	DriverSettingsMessageAdapter adapter;
	status = B_ENTRY_NOT_FOUND;

	if (name == NULL || strcmp(name, kInterfaceSettingsName) == 0) {
		status = adapter.ConvertToDriverSettings(
			_Path(path, kInterfaceSettingsName).Path(),
			kInterfacesTemplate, fInterfaces);
	}
	if (name == NULL || strcmp(name, kNetworksSettingsName) == 0) {
		// Convert settings to storage format
		BMessage networks = fNetworks;
		BMessage network;
		for (int32 index = 0; networks.FindMessage("network", index,
				&network); index++) {
			if (_ConvertNetworkToSettings(network) == B_OK)
				networks.ReplaceMessage("network", index, &network);
		}

		status = adapter.ConvertToDriverSettings(
			_Path(path, kNetworksSettingsName).Path(),
			kNetworksTemplate, networks);
	}
	if (name == NULL || strcmp(name, kServicesSettingsName) == 0) {
		status = adapter.ConvertToDriverSettings(
			_Path(path, kServicesSettingsName).Path(),
			kServicesTemplate, fServices);
	}

	return status;
}


BPath
BNetworkSettings::_Path(BPath& parent, const char* leaf)
{
	return BPath(parent.Path(), leaf);
}


status_t
BNetworkSettings::_GetPath(const char* name, BPath& path)
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
BNetworkSettings::_StartWatching(const char* name, const BMessenger& target)
{
	BPath path;
	status_t status = _GetPath(name, path);
	if (status != B_OK)
		return status;

	return BPrivate::BPathMonitor::StartWatching(path.Path(), B_WATCH_STAT,
		target);
}


status_t
BNetworkSettings::_ConvertNetworkToSettings(BMessage& message)
{
	BNetworkAddress address;
	status_t result = message.FindFlat("address", &address);
	if (result == B_OK)
		message.RemoveName("address");

	if (result == B_OK && address.Family() == AF_LINK) {
		size_t addressLength = address.LinkLevelAddressLength();
		uint8* macAddress = address.LinkLevelAddress();
		bool usable = false;
		BString formatted;

		for (size_t index = 0; index < addressLength; index++) {
			if (index > 0)
				formatted.Append(":");
			char buffer[3];
			snprintf(buffer, sizeof(buffer), "%2x", macAddress[index]);
			formatted.Append(buffer, sizeof(buffer));

			if (macAddress[index] != 0)
				usable = true;
		}

		if (usable)
			message.AddString("mac", formatted);
	}

	uint32 authentication = 0;
	result = message.FindUInt32("authentication_mode", &authentication);
	if (result == B_OK) {
		message.RemoveName("authentication_mode");

		const char* authenticationString = NULL;
		switch (authentication) {
			case B_NETWORK_AUTHENTICATION_NONE:
				authenticationString = "none";
				break;
			case B_NETWORK_AUTHENTICATION_WEP:
				authenticationString = "wep";
				break;
			case B_NETWORK_AUTHENTICATION_WPA:
				authenticationString = "wpa";
				break;
			case B_NETWORK_AUTHENTICATION_WPA2:
				authenticationString = "wpa2";
				break;
		}

		if (result == B_OK && authenticationString != NULL)
			message.AddString("authentication", authenticationString);
	}

	uint32 cipher = 0;
	result = message.FindUInt32("cipher", &cipher);
	if (result == B_OK) {
		message.RemoveName("cipher");

		if ((cipher & B_NETWORK_CIPHER_NONE) != 0)
			message.AddString("cipher", "none");
		if ((cipher & B_NETWORK_CIPHER_TKIP) != 0)
			message.AddString("cipher", "tkip");
		if ((cipher & B_NETWORK_CIPHER_CCMP) != 0)
			message.AddString("cipher", "ccmp");
	}

	uint32 groupCipher = 0;
	result = message.FindUInt32("group_cipher", &groupCipher);
	if (result == B_OK) {
		message.RemoveName("group_cipher");

		if ((groupCipher & B_NETWORK_CIPHER_NONE) != 0)
			message.AddString("group_cipher", "none");
		if ((groupCipher & B_NETWORK_CIPHER_WEP_40) != 0)
			message.AddString("group_cipher", "wep40");
		if ((groupCipher & B_NETWORK_CIPHER_WEP_104) != 0)
			message.AddString("group_cipher", "wep104");
		if ((groupCipher & B_NETWORK_CIPHER_TKIP) != 0)
			message.AddString("group_cipher", "tkip");
		if ((groupCipher & B_NETWORK_CIPHER_CCMP) != 0)
			message.AddString("group_cipher", "ccmp");
	}

	// TODO: the other fields aren't currently used, add them when they are
	// and when it's clear how they will be stored
	message.RemoveName("noise_level");
	message.RemoveName("signal_strength");
	message.RemoveName("flags");
	message.RemoveName("key_mode");

	return B_OK;
}


status_t
BNetworkSettings::_ConvertNetworkFromSettings(BMessage& message)
{
	message.RemoveName("mac");
		// TODO: convert into a flat BNetworkAddress "address"

	const char* authentication = NULL;
	if (message.FindString("authentication", &authentication) == B_OK) {
		message.RemoveName("authentication");

		if (strcasecmp(authentication, "none") == 0) {
			message.AddUInt32("authentication_mode",
				B_NETWORK_AUTHENTICATION_NONE);
		} else if (strcasecmp(authentication, "wep") == 0) {
			message.AddUInt32("authentication_mode",
				B_NETWORK_AUTHENTICATION_WEP);
		} else if (strcasecmp(authentication, "wpa") == 0) {
			message.AddUInt32("authentication_mode",
				B_NETWORK_AUTHENTICATION_WPA);
		} else if (strcasecmp(authentication, "wpa2") == 0) {
			message.AddUInt32("authentication_mode",
				B_NETWORK_AUTHENTICATION_WPA2);
		}
	}

	int32 index = 0;
	uint32 cipher = 0;
	const char* cipherString = NULL;
	while (message.FindString("cipher", index++, &cipherString) == B_OK) {
		if (strcasecmp(cipherString, "none") == 0)
			cipher |= B_NETWORK_CIPHER_NONE;
		else if (strcasecmp(cipherString, "tkip") == 0)
			cipher |= B_NETWORK_CIPHER_TKIP;
		else if (strcasecmp(cipherString, "ccmp") == 0)
			cipher |= B_NETWORK_CIPHER_CCMP;
	}

	message.RemoveName("cipher");
	if (cipher != 0)
		message.AddUInt32("cipher", cipher);

	index = 0;
	cipher = 0;
	while (message.FindString("group_cipher", index++, &cipherString) == B_OK) {
		if (strcasecmp(cipherString, "none") == 0)
			cipher |= B_NETWORK_CIPHER_NONE;
		else if (strcasecmp(cipherString, "wep40") == 0)
			cipher |= B_NETWORK_CIPHER_WEP_40;
		else if (strcasecmp(cipherString, "wep104") == 0)
			cipher |= B_NETWORK_CIPHER_WEP_104;
		else if (strcasecmp(cipherString, "tkip") == 0)
			cipher |= B_NETWORK_CIPHER_TKIP;
		else if (strcasecmp(cipherString, "ccmp") == 0)
			cipher |= B_NETWORK_CIPHER_CCMP;
	}

	message.RemoveName("group_cipher");
	if (cipher != 0)
		message.AddUInt32("group_cipher", cipher);

	message.AddUInt32("flags", B_NETWORK_IS_PERSISTENT);

	// TODO: add the other fields
	message.RemoveName("key");
	return B_OK;
}


status_t
BNetworkSettings::_RemoveItem(BMessage& container, const char* itemField,
	const char* nameField, const char* name, const char* store)
{
	int32 index = 0;
	BMessage item;
	while (container.FindMessage(itemField, index, &item) == B_OK) {
		const char* itemName = NULL;
		if (item.FindString(nameField, &itemName) == B_OK
			&& strcmp(itemName, name) == 0) {
			container.RemoveData(itemField, index);
			if (store != NULL)
				return _Save(store);
			return B_OK;
		}

		index++;
	}

	return B_ENTRY_NOT_FOUND;
}
