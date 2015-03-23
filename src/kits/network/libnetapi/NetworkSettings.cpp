/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <NetworkSettings.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_interface.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <Path.h>
#include <PathMonitor.h>
#include <String.h>

#include <DriverSettingsMessageAdapter.h>
#include <NetServer.h>


using namespace BNetworkKit;


static const char* kInterfaceSettingsName = "interfaces";
static const char* kServicesSettingsName = "services";
static const char* kNetworksSettingsName = "wireless_networks";


// Interface templates

namespace BPrivate {


class InterfaceAddressFamilyConverter : public DriverSettingsConverter {
public:
	virtual	status_t			ConvertFromDriverSettings(
									const driver_parameter& parameter,
									const char* name, int32 index, uint32 type,
									BMessage& target);
	virtual	status_t			ConvertToDriverSettings(const BMessage& source,
									const char* name, int32 index,
									uint32 type, BString& value);
};


}	// namespace BPrivate

using BPrivate::InterfaceAddressFamilyConverter;


const static settings_template kInterfaceAddressTemplate[] = {
	{B_STRING_TYPE, "family", NULL, true, new InterfaceAddressFamilyConverter},
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
	{B_BOOL_TYPE, "disabled", NULL},
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


struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
};


static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
	},
	{
		AF_INET6,
		"inet6",
		{"AF_INET6", "inet6", "ipv6", NULL},
	},
	{ -1, NULL, {NULL} }
};


static const char*
get_family_name(int family)
{
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		if (kFamilies[i].family == family)
			return kFamilies[i].name;
	}
	return NULL;
}


static int
get_address_family(const char* argument)
{
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		for (int32 j = 0; kFamilies[i].identifiers[j]; j++) {
			if (!strcmp(argument, kFamilies[i].identifiers[j])) {
				// found a match
				return kFamilies[i].family;
			}
		}
	}

	return AF_UNSPEC;
}


/*!	Parses the \a argument as network \a address for the specified \a family.
	If \a family is \c AF_UNSPEC, \a family will be overwritten with the family
	of the successfully parsed address.
*/
static bool
parse_address(int32& family, const char* argument, BNetworkAddress& address)
{
	if (argument == NULL) {
		if (family != AF_UNSPEC)
			address.SetToWildcard(family);
		return false;
	}

	status_t status = address.SetTo(family, argument, (uint16)0,
		B_NO_ADDRESS_RESOLUTION);
	if (status != B_OK)
		return false;

	if (family == AF_UNSPEC) {
		// Test if we support the resulting address family
		bool supported = false;

		for (int32 i = 0; kFamilies[i].family >= 0; i++) {
			if (kFamilies[i].family == address.Family()) {
				supported = true;
				break;
			}
		}
		if (!supported)
			return false;

		// Take over family from address
		family = address.Family();
	}

	return true;
}


static int
parse_type(const char* string)
{
	if (!strcasecmp(string, "stream"))
		return SOCK_STREAM;

	return SOCK_DGRAM;
}


static int
parse_protocol(const char* string)
{
	struct protoent* proto = getprotobyname(string);
	if (proto == NULL)
		return IPPROTO_TCP;

	return proto->p_proto;
}


static int
type_for_protocol(int protocol)
{
	// default determined by protocol
	switch (protocol) {
		case IPPROTO_TCP:
			return SOCK_STREAM;

		case IPPROTO_UDP:
		default:
			return SOCK_DGRAM;
	}
}


// #pragma mark -


status_t
InterfaceAddressFamilyConverter::ConvertFromDriverSettings(
	const driver_parameter& parameter, const char* name, int32 index,
	uint32 type, BMessage& target)
{
	return B_NOT_SUPPORTED;
}


status_t
InterfaceAddressFamilyConverter::ConvertToDriverSettings(const BMessage& source,
	const char* name, int32 index, uint32 type, BString& value)
{
	int32 family;
	if (source.FindInt32("family", &family) == B_OK) {
		const char* familyName = get_family_name(family);
		if (familyName != NULL)
			value << familyName;
		else
			value << family;

		return B_OK;
	}

	return B_NOT_SUPPORTED;
}


// #pragma mark -


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
BNetworkSettings::GetInterface(const char* name, BMessage& interface) const
{
	int32 index;
	return _GetItem(fInterfaces, "interface", "device", name, index, interface);
}


status_t
BNetworkSettings::AddInterface(const BMessage& interface)
{
	const char* name = NULL;
	if (interface.FindString("device", &name) != B_OK)
		return B_BAD_VALUE;

	_RemoveItem(fInterfaces, "interface", "device", name);

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


BNetworkInterfaceSettings
BNetworkSettings::Interface(const char* name)
{
	BMessage interface;
	GetInterface(name, interface);
	return BNetworkInterfaceSettings(interface);
}


const BNetworkInterfaceSettings
BNetworkSettings::Interface(const char* name) const
{
	BMessage interface;
	GetInterface(name, interface);
	return BNetworkInterfaceSettings(interface);
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
BNetworkSettings::GetNetwork(const char* name, BMessage& network) const
{
	int32 index;
	return _GetItem(fNetworks, "network", "name", name, index, network);
}


status_t
BNetworkSettings::AddNetwork(const BMessage& network)
{
	const char* name = NULL;
	if (network.FindString("name", &name) != B_OK)
		return B_BAD_VALUE;

	_RemoveItem(fNetworks, "network", "name", name);

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


const BMessage&
BNetworkSettings::Services() const
{
	return fServices;
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


status_t
BNetworkSettings::GetService(const char* name, BMessage& service) const
{
	int32 index;
	return _GetItem(fServices, "service", "name", name, index, service);
}


status_t
BNetworkSettings::AddService(const BMessage& service)
{
	const char* name = service.GetString("name");
	if (name == NULL)
		return B_BAD_VALUE;

	_RemoveItem(fServices, "service", "name", name);

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


BNetworkServiceSettings
BNetworkSettings::Service(const char* name)
{
	BMessage service;
	GetService(name, service);
	return BNetworkServiceSettings(service);
}


const BNetworkServiceSettings
BNetworkSettings::Service(const char* name) const
{
	BMessage service;
	GetService(name, service);
	return BNetworkServiceSettings(service);
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
				*_type = kMsgNetworkSettingsUpdated;
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
BNetworkSettings::_GetItem(const BMessage& container, const char* itemField,
	const char* nameField, const char* name, int32& _index,
	BMessage& item) const
{
	int32 index = 0;
	while (container.FindMessage(itemField, index, &item) == B_OK) {
		const char* itemName = NULL;
		if (item.FindString(nameField, &itemName) == B_OK
			&& strcmp(itemName, name) == 0) {
			_index = index;
			return B_OK;
		}

		index++;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
BNetworkSettings::_RemoveItem(BMessage& container, const char* itemField,
	const char* nameField, const char* name, const char* store)
{
	BMessage item;
	int32 index;
	if (_GetItem(container, itemField, nameField, name, index, item) == B_OK) {
		container.RemoveData(itemField, index);
		if (store != NULL)
			return _Save(store);
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


// #pragma mark - BNetworkInterfaceAddressSettings


BNetworkInterfaceAddressSettings::BNetworkInterfaceAddressSettings()
	:
	fFamily(AF_UNSPEC),
	fAutoConfigure(true)
{
}


BNetworkInterfaceAddressSettings::BNetworkInterfaceAddressSettings(
	const BMessage& data)
{
	if (data.FindInt32("family", &fFamily) != B_OK) {
		const char* familyString;
		if (data.FindString("family", &familyString) == B_OK) {
			fFamily = get_address_family(familyString);
			if (fFamily == AF_UNSPEC) {
				// we don't support this family
				fprintf(stderr, "Ignore unknown family: %s\n",
					familyString);
				return;
			}
		} else
			fFamily = AF_UNSPEC;
	}

	fAutoConfigure = data.GetBool("auto_config", false);

	if (!fAutoConfigure) {
		if (parse_address(fFamily, data.GetString("address", NULL), fAddress))
			parse_address(fFamily, data.GetString("mask", NULL), fMask);

		parse_address(fFamily, data.GetString("peer", NULL), fPeer);
		parse_address(fFamily, data.GetString("broadcast", NULL), fBroadcast);
		parse_address(fFamily, data.GetString("gateway", NULL), fGateway);
	}
}


BNetworkInterfaceAddressSettings::BNetworkInterfaceAddressSettings(
	const BNetworkInterfaceAddressSettings& other)
	:
	fFamily(other.fFamily),
	fAutoConfigure(other.fAutoConfigure),
	fAddress(other.fAddress),
	fMask(other.fMask),
	fPeer(other.fPeer),
	fBroadcast(other.fBroadcast),
	fGateway(other.fGateway)
{
}


BNetworkInterfaceAddressSettings::~BNetworkInterfaceAddressSettings()
{
}


int
BNetworkInterfaceAddressSettings::Family() const
{
	return fFamily;
}


void
BNetworkInterfaceAddressSettings::SetFamily(int family)
{
	fFamily = family;
}


bool
BNetworkInterfaceAddressSettings::IsAutoConfigure() const
{
	return fAutoConfigure;
}


void
BNetworkInterfaceAddressSettings::SetAutoConfigure(bool configure)
{
	fAutoConfigure = configure;
}


const BNetworkAddress&
BNetworkInterfaceAddressSettings::Address() const
{
	return fAddress;
}


BNetworkAddress&
BNetworkInterfaceAddressSettings::Address()
{
	return fAddress;
}


const BNetworkAddress&
BNetworkInterfaceAddressSettings::Mask() const
{
	return fMask;
}


BNetworkAddress&
BNetworkInterfaceAddressSettings::Mask()
{
	return fMask;
}


const BNetworkAddress&
BNetworkInterfaceAddressSettings::Peer() const
{
	return fPeer;
}


BNetworkAddress&
BNetworkInterfaceAddressSettings::Peer()
{
	return fPeer;
}


const BNetworkAddress&
BNetworkInterfaceAddressSettings::Broadcast() const
{
	return fBroadcast;
}


BNetworkAddress&
BNetworkInterfaceAddressSettings::Broadcast()
{
	return fBroadcast;
}


const BNetworkAddress&
BNetworkInterfaceAddressSettings::Gateway() const
{
	return fGateway;
}


BNetworkAddress&
BNetworkInterfaceAddressSettings::Gateway()
{
	return fGateway;
}


status_t
BNetworkInterfaceAddressSettings::GetMessage(BMessage& data) const
{
	status_t status = B_OK;
	if (fFamily != AF_UNSPEC)
		status = data.SetInt32("family", fFamily);
	if (status == B_OK && fAutoConfigure)
		return data.SetBool("auto_config", fAutoConfigure);

	if (status == B_OK && !fAddress.IsEmpty()) {
		status = data.SetString("address", fAddress.ToString());
		if (status == B_OK && !fMask.IsEmpty())
			status = data.SetString("mask", fMask.ToString());
	}
	if (status == B_OK && !fPeer.IsEmpty())
		status = data.SetString("peer", fPeer.ToString());
	if (status == B_OK && !fBroadcast.IsEmpty())
		status = data.SetString("broadcast", fBroadcast.ToString());
	if (status == B_OK && !fGateway.IsEmpty())
		status = data.SetString("gateway", fGateway.ToString());

	return status;
}


BNetworkInterfaceAddressSettings&
BNetworkInterfaceAddressSettings::operator=(
	const BNetworkInterfaceAddressSettings& other)
{
	fFamily = other.fFamily;
	fAutoConfigure = other.fAutoConfigure;
	fAddress = other.fAddress;
	fMask = other.fMask;
	fPeer = other.fPeer;
	fBroadcast = other.fBroadcast;
	fGateway = other.fGateway;

	return *this;
}


// #pragma mark - BNetworkInterfaceSettings


BNetworkInterfaceSettings::BNetworkInterfaceSettings()
	:
	fFlags(0),
	fMTU(0),
	fMetric(0)
{
}


BNetworkInterfaceSettings::BNetworkInterfaceSettings(const BMessage& message)
{
	fName = message.GetString("device");
	fFlags = message.GetInt32("flags", 0);
	fMTU = message.GetInt32("mtu", 0);
	fMetric = message.GetInt32("metric", 0);

	BMessage addressData;
	for (int32 index = 0; message.FindMessage("address", index,
			&addressData) == B_OK; index++) {
		BNetworkInterfaceAddressSettings address(addressData);
		fAddresses.push_back(address);
	}
}


BNetworkInterfaceSettings::~BNetworkInterfaceSettings()
{
}


const char*
BNetworkInterfaceSettings::Name() const
{
	return fName;
}


void
BNetworkInterfaceSettings::SetName(const char* name)
{
	fName = name;
}


int32
BNetworkInterfaceSettings::Flags() const
{
	return fFlags;
}


void
BNetworkInterfaceSettings::SetFlags(int32 flags)
{
	fFlags = flags;
}


int32
BNetworkInterfaceSettings::MTU() const
{
	return fMTU;
}


void
BNetworkInterfaceSettings::SetMTU(int32 mtu)
{
	fMTU = mtu;
}


int32
BNetworkInterfaceSettings::Metric() const
{
	return fMetric;
}


void
BNetworkInterfaceSettings::SetMetric(int32 metric)
{
	fMetric = metric;
}


int32
BNetworkInterfaceSettings::CountAddresses() const
{
	return fAddresses.size();
}


const BNetworkInterfaceAddressSettings&
BNetworkInterfaceSettings::AddressAt(int32 index) const
{
	return fAddresses[index];
}


BNetworkInterfaceAddressSettings&
BNetworkInterfaceSettings::AddressAt(int32 index)
{
	return fAddresses[index];
}


int32
BNetworkInterfaceSettings::FindFirstAddress(int family) const
{
	for (int32 index = 0; index < CountAddresses(); index++) {
		const BNetworkInterfaceAddressSettings address = AddressAt(index);
		if (address.Family() == family)
			return index;
	}
	return -1;
}


void
BNetworkInterfaceSettings::AddAddress(
	const BNetworkInterfaceAddressSettings& address)
{
	fAddresses.push_back(address);
}


void
BNetworkInterfaceSettings::RemoveAddress(int32 index)
{
	fAddresses.erase(fAddresses.begin() + index);
}


/*!	This is a convenience method that returns the current state of the
	interface, not just the one configured.

	This means, even if the settings say: auto configured, this method
	may still return false, if the configuration has been manually tempered
	with.
*/
bool
BNetworkInterfaceSettings::IsAutoConfigure(int family) const
{
	BNetworkInterface interface(fName);
	// TODO: this needs to happen at protocol level
	if ((interface.Flags() & (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0)
		return true;

	BNetworkInterfaceAddress address;
	status_t status = B_ERROR;

	int32 index = interface.FindFirstAddress(family);
	if (index >= 0)
		status = interface.GetAddressAt(index, address);
	if (index < 0 || status != B_OK || address.Address().IsEmpty()) {
		if (status == B_OK) {
			// Check persistent settings for the mode -- the address
			// can also be empty if the automatic configuration hasn't
			// started yet (for example, because there is no link).
			int32 index = FindFirstAddress(family);
			if (index < 0)
				index = FindFirstAddress(AF_UNSPEC);
			if (index >= 0) {
				const BNetworkInterfaceAddressSettings& address
					= AddressAt(index);
				return address.IsAutoConfigure();
			}
		}
	}

	return false;
}


status_t
BNetworkInterfaceSettings::GetMessage(BMessage& data) const
{
	status_t status = data.SetString("device", fName);
	if (status == B_OK && fFlags != 0)
		status = data.SetInt32("flags", fFlags);
	if (status == B_OK && fMTU != 0)
		status = data.SetInt32("mtu", fMTU);
	if (status == B_OK && fMetric != 0)
		status = data.SetInt32("metric", fMetric);

	for (int32 i = 0; i < CountAddresses(); i++) {
		BMessage address;
		status = AddressAt(i).GetMessage(address);
		if (status == B_OK)
			status = data.AddMessage("address", &address);
		if (status != B_OK)
			break;
	}
	return status;
}


// #pragma mark - BNetworkServiceAddressSettings


BNetworkServiceAddressSettings::BNetworkServiceAddressSettings()
{
}


BNetworkServiceAddressSettings::BNetworkServiceAddressSettings(
	const BMessage& data, int serviceFamily, int serviceType,
	int serviceProtocol, int servicePort)
{
	// TODO: dump problems in the settings to syslog
	if (data.FindInt32("family", &fFamily) != B_OK) {
		const char* familyString;
		if (data.FindString("family", &familyString) == B_OK) {
			fFamily = get_address_family(familyString);
			if (fFamily == AF_UNSPEC) {
				// we don't support this family
				fprintf(stderr, "Ignore unknown family: %s\n",
					familyString);
				return;
			}
		} else
			fFamily = serviceFamily;
	}

	if (!parse_address(fFamily, data.GetString("address"), fAddress))
		fAddress.SetToWildcard(fFamily);

	const char* string;
	if (data.FindString("protocol", &string) == B_OK)
		fProtocol = parse_protocol(string);
	else
		fProtocol = serviceProtocol;

	if (data.FindString("type", &string) == B_OK)
		fType = parse_type(string);
	else if (fProtocol != serviceProtocol)
		fType = type_for_protocol(fProtocol);
	else
		fType = serviceType;

	fAddress.SetPort(data.GetInt32("port", servicePort));
}


BNetworkServiceAddressSettings::~BNetworkServiceAddressSettings()
{
}


int
BNetworkServiceAddressSettings::Family() const
{
	return fFamily;
}


void
BNetworkServiceAddressSettings::SetFamily(int family)
{
	fFamily = family;
}


int
BNetworkServiceAddressSettings::Protocol() const
{
	return fProtocol;
}


void
BNetworkServiceAddressSettings::SetProtocol(int protocol)
{
	fProtocol = protocol;
}


int
BNetworkServiceAddressSettings::Type() const
{
	return fType;
}


void
BNetworkServiceAddressSettings::SetType(int type)
{
	fType = type;
}


const BNetworkAddress&
BNetworkServiceAddressSettings::Address() const
{
	return fAddress;
}


BNetworkAddress&
BNetworkServiceAddressSettings::Address()
{
	return fAddress;
}


status_t
BNetworkServiceAddressSettings::GetMessage(BMessage& data) const
{
	// TODO!
	return B_NOT_SUPPORTED;
}


bool
BNetworkServiceAddressSettings::operator==(
	const BNetworkServiceAddressSettings& other) const
{
	return Family() == other.Family()
		&& Type() == other.Type()
		&& Protocol() == other.Protocol()
		&& Address() == other.Address();
}


// #pragma mark - BNetworkServiceSettings


BNetworkServiceSettings::BNetworkServiceSettings()
	:
	fFamily(AF_UNSPEC),
	fType(-1),
	fProtocol(-1),
	fPort(-1),
	fEnabled(true),
	fStandAlone(false)
{
}


BNetworkServiceSettings::BNetworkServiceSettings(const BMessage& message)
	:
	fType(-1),
	fProtocol(-1),
	fPort(-1),
	fEnabled(true),
	fStandAlone(false)
{
	// TODO: user/group is currently ignored!

	fName = message.GetString("name");

	// Default family/port/protocol/type for all addresses

	// we default to inet/tcp/port-from-service-name if nothing is specified
	const char* string;
	if (message.FindString("family", &string) != B_OK)
		string = "inet";

	fFamily = get_address_family(string);
	if (fFamily == AF_UNSPEC)
		fFamily = AF_INET;

	if (message.FindString("protocol", &string) == B_OK)
		fProtocol = parse_protocol(string);
	else {
		string = "tcp";
			// we set 'string' here for an eventual call to getservbyname()
			// below
		fProtocol = IPPROTO_TCP;
	}

	if (message.FindInt32("port", &fPort) != B_OK) {
		struct servent* servent = getservbyname(Name(), string);
		if (servent != NULL)
			fPort = ntohs(servent->s_port);
		else
			fPort = -1;
	}

	if (message.FindString("type", &string) == B_OK)
		fType = parse_type(string);
	else
		fType = type_for_protocol(fProtocol);

	fStandAlone = message.GetBool("stand_alone");

	const char* argument;
	for (int i = 0; message.FindString("launch", i, &argument) == B_OK; i++) {
		fArguments.Add(argument);
	}

	BMessage addressData;
	int32 i = 0;
	for (; message.FindMessage("address", i, &addressData) == B_OK; i++) {
		BNetworkServiceAddressSettings address(addressData, fFamily,
			fType, fProtocol, fPort);
		fAddresses.push_back(address);
	}

	if (i == 0 && (fFamily < 0 || fPort < 0)) {
		// no address specified
		printf("service %s has no address specified\n", Name());
		return;
	}

	if (i == 0) {
		// no address specified, but family/port were given; add empty address
		BNetworkServiceAddressSettings address;
		address.SetFamily(fFamily);
		address.SetType(fType);
		address.SetProtocol(fProtocol);
		address.Address().SetToWildcard(fFamily, fPort);

		fAddresses.push_back(address);
	}
}


BNetworkServiceSettings::~BNetworkServiceSettings()
{
}


status_t
BNetworkServiceSettings::InitCheck() const
{
	if (!fName.IsEmpty() && !fArguments.IsEmpty() && CountAddresses() > 0)
		return B_OK;

	return B_BAD_VALUE;
}


const char*
BNetworkServiceSettings::Name() const
{
	return fName.String();
}


void
BNetworkServiceSettings::SetName(const char* name)
{
	fName = name;
}


bool
BNetworkServiceSettings::IsStandAlone() const
{
	return fStandAlone;
}


void
BNetworkServiceSettings::SetStandAlone(bool alone)
{
	fStandAlone = alone;
}


bool
BNetworkServiceSettings::IsEnabled() const
{
	return InitCheck() == B_OK && fEnabled;
}


void
BNetworkServiceSettings::SetEnabled(bool enable)
{
	fEnabled = enable;
}


int
BNetworkServiceSettings::Family() const
{
	return fFamily;
}


void
BNetworkServiceSettings::SetFamily(int family)
{
	fFamily = family;
}


int
BNetworkServiceSettings::Protocol() const
{
	return fProtocol;
}


void
BNetworkServiceSettings::SetProtocol(int protocol)
{
	fProtocol = protocol;
}


int
BNetworkServiceSettings::Type() const
{
	return fType;
}


void
BNetworkServiceSettings::SetType(int type)
{
	fType = type;
}


int
BNetworkServiceSettings::Port() const
{
	return fPort;
}


void
BNetworkServiceSettings::SetPort(int port)
{
	fPort = port;
}


int32
BNetworkServiceSettings::CountArguments() const
{
	return fArguments.CountStrings();
}


const char*
BNetworkServiceSettings::ArgumentAt(int32 index) const
{
	return fArguments.StringAt(index);
}


void
BNetworkServiceSettings::AddArgument(const char* argument)
{
	fArguments.Add(argument);
}


void
BNetworkServiceSettings::RemoveArgument(int32 index)
{
	fArguments.Remove(index);
}


int32
BNetworkServiceSettings::CountAddresses() const
{
	return fAddresses.size();
}


const BNetworkServiceAddressSettings&
BNetworkServiceSettings::AddressAt(int32 index) const
{
	return fAddresses[index];
}


void
BNetworkServiceSettings::AddAddress(
	const BNetworkServiceAddressSettings& address)
{
	fAddresses.push_back(address);
}


void
BNetworkServiceSettings::RemoveAddress(int32 index)
{
	fAddresses.erase(fAddresses.begin() + index);
}


/*!	This is a convenience method that returns the current state of the
	service, independent of the current settings.
*/
bool
BNetworkServiceSettings::IsRunning() const
{
	BMessage request(kMsgIsServiceRunning);
	request.AddString("name", fName);

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status_t status = networkServer.SendMessage(&request, &reply);
	if (status == B_OK)
		return reply.GetBool("running");

	return false;
}


status_t
BNetworkServiceSettings::GetMessage(BMessage& data) const
{
	status_t status = data.SetString("name", fName);
	if (status == B_OK && !fEnabled)
		status = data.SetBool("disabled", true);
	if (status == B_OK && fStandAlone)
		status = data.SetBool("stand_alone", true);

	if (fFamily != AF_UNSPEC)
		status = data.SetInt32("family", fFamily);
	if (fType != -1)
		status = data.SetInt32("type", fType);
	if (fProtocol != -1)
		status = data.SetInt32("protocol", fProtocol);
	if (fPort != -1)
		status = data.SetInt32("port", fPort);

	for (int32 i = 0; i < fArguments.CountStrings(); i++) {
		if (status == B_OK)
			status = data.AddString("launch", fArguments.StringAt(i));
		if (status != B_OK)
			break;
	}

	for (int32 i = 0; i < CountAddresses(); i++) {
		BNetworkServiceAddressSettings address = AddressAt(i);
		if (address.Family() == Family()
			&& address.Type() == Type()
			&& address.Protocol() == Protocol()
			&& address.Address().IsWildcard()
			&& address.Address().Port() == Port()) {
			// This address will be created automatically, no need to store it
			continue;
		}

		BMessage addressMessage;
		status = AddressAt(i).GetMessage(addressMessage);
		if (status == B_OK)
			status = data.AddMessage("address", &addressMessage);
		if (status != B_OK)
			break;
	}
	return status;
}
