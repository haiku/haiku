/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 * 		Vegard Wærp, vegarwa@online.no
 */


#include "NetServer.h"

#include <errno.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <Alert.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <Server.h>
#include <TextView.h>
#include <FindDirectory.h>

#include <AutoDeleter.h>
#include <WPASupplicant.h>

#include "AutoconfigLooper.h"
#include "Services.h"
#include "Settings.h"

extern "C" {
#	include <net80211/ieee80211_ioctl.h>
}


typedef std::map<std::string, AutoconfigLooper*> LooperMap;


class NetServer : public BServer {
public:
								NetServer(status_t& status);
	virtual						~NetServer();

	virtual	void				AboutRequested();
	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);

private:
			bool				_IsValidInterface(BNetworkInterface& interface);
			void				_RemoveInvalidInterfaces();
			status_t			_RemoveInterface(const char* name);
			status_t			_DisableInterface(const char* name);
			bool				_TestForInterface(const char* name);
			status_t			_ConfigureInterface(BMessage& interface);
			status_t			_ConfigureResolver(
									BMessage& resolverConfiguration);
			bool				_QuitLooperForDevice(const char* device);
			AutoconfigLooper*	_LooperForDevice(const char* device);
			status_t			_ConfigureDevice(const char* path);
			void				_ConfigureDevices(const char* path,
									BMessage* suggestedInterface = NULL);
			void				_ConfigureInterfaces(
									BMessage* _missingDevice = NULL);
			void				_BringUpInterfaces();
			void				_StartServices();
			status_t			_HandleDeviceMonitor(BMessage* message);

			status_t			_AutoJoinNetwork(const char* name);
			status_t			_JoinNetwork(const BMessage& message,
									const char* name = NULL);
			status_t			_LeaveNetwork(const BMessage& message);

private:
			Settings			fSettings;
			LooperMap			fDeviceMap;
			BMessenger			fServices;
};


struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
};


// AF_INET6 family
#if INET6
static bool inet6_parse_address(const char* string, sockaddr* address);
static void inet6_set_any_address(sockaddr* address);
static void inet6_set_port(sockaddr* address, int32 port);
#endif

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


// #pragma mark - private functions


static status_t
set_80211(const char* name, int32 type, void* data,
	int32 length = 0, int32 value = 0)
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	struct ieee80211req ireq;
	strlcpy(ireq.i_name, name, IF_NAMESIZE);
	ireq.i_type = type;
	ireq.i_val = value;
	ireq.i_len = length;
	ireq.i_data = data;

	if (ioctl(socket, SIOCS80211, &ireq, sizeof(struct ieee80211req)) < 0)
		return errno;

	return B_OK;
}


// #pragma mark - exported functions


int
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
bool
parse_address(int32& family, const char* argument, BNetworkAddress& address)
{
	if (argument == NULL)
		return false;

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


#if INET6
static bool
inet6_parse_address(const char* string, sockaddr* _address)
{
	sockaddr_in6& address = *(sockaddr_in6*)_address;

	if (inet_pton(AF_INET6, string, &address.sin6_addr) != 1)
		return false;

	address.sin6_family = AF_INET6;
	address.sin6_len = sizeof(sockaddr_in6);
	address.sin6_port = 0;
	address.sin6_flowinfo = 0;
	address.sin6_scope_id = 0;

	return true;
}


void
inet6_set_any_address(sockaddr* _address)
{
	sockaddr_in6& address = *(sockaddr_in6*)_address;
	memset(&address, 0, sizeof(sockaddr_in6));
	address.sin6_family = AF_INET6;
	address.sin6_len = sizeof(struct sockaddr_in6);
}


void
inet6_set_port(sockaddr* _address, int32 port)
{
	sockaddr_in6& address = *(sockaddr_in6*)_address;
	address.sin6_port = port;
}
#endif


//	#pragma mark -


NetServer::NetServer(status_t& error)
	:
	BServer(kNetServerSignature, false, &error)
{
}


NetServer::~NetServer()
{
	BPrivate::BPathMonitor::StopWatching("/dev/net", this);
}


void
NetServer::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Networking Server\n"
		"\tCopyright " B_UTF8_COPYRIGHT "2006, Haiku.\n", "OK");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 17, &font);

	alert->Go(NULL);
}


void
NetServer::ReadyToRun()
{
	fSettings.StartMonitoring(this);
	_BringUpInterfaces();
	_StartServices();

	BPrivate::BPathMonitor::StartWatching("/dev/net", B_ENTRY_CREATED
		| B_ENTRY_REMOVED | B_WATCH_FILES_ONLY | B_WATCH_RECURSIVELY, this);
}


void
NetServer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PATH_MONITOR:
		{
			fSettings.Update(message);
			_HandleDeviceMonitor(message);
			break;
		}

		case kMsgInterfaceSettingsUpdated:
		{
			_ConfigureInterfaces();
			break;
		}

		case kMsgServiceSettingsUpdated:
		{
			BMessage update = fSettings.Services();
			update.what = kMsgUpdateServices;

			fServices.SendMessage(&update);
			break;
		}

		case kMsgConfigureInterface:
		{
			status_t status = _ConfigureInterface(*message);

			BMessage reply(B_REPLY);
			reply.AddInt32("status", status);
			message->SendReply(&reply);
			break;
		}

		case kMsgConfigureResolver:
		{
			status_t status = _ConfigureResolver(*message);

			BMessage reply(B_REPLY);
			reply.AddInt32("status", status);
			message->SendReply(&reply);
			break;
		}

		case kMsgJoinNetwork:
		{
			status_t status = _JoinNetwork(*message);

			BMessage reply(B_REPLY);
			reply.AddInt32("status", status);
			message->SendReply(&reply);
			break;
		}

		case kMsgLeaveNetwork:
		{
			status_t status = _LeaveNetwork(*message);

			BMessage reply(B_REPLY);
			reply.AddInt32("status", status);
			message->SendReply(&reply);
			break;
		}

		default:
			BApplication::MessageReceived(message);
			return;
	}
}


/*!	Checks if an interface is valid, that is, if it has an address in any
	family, and, in case of ethernet, a hardware MAC address.
*/
bool
NetServer::_IsValidInterface(BNetworkInterface& interface)
{
	// check if it has an address

	if (interface.CountAddresses() == 0)
		return false;

	// check if it has a hardware address, too, in case of ethernet

	BNetworkAddress link;
	if (interface.GetHardwareAddress(link) != B_OK)
		return false;

	if (link.LinkLevelType() == IFT_ETHER && link.LinkLevelAddressLength() != 6)
		return false;

	return true;
}


void
NetServer::_RemoveInvalidInterfaces()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (!_IsValidInterface(interface)) {
			// remove invalid interface
			_RemoveInterface(interface.Name());
		}
	}
}


bool
NetServer::_TestForInterface(const char* name)
{

	BNetworkRoster& roster = BNetworkRoster::Default();
	int32 nameLength = strlen(name);
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (!strncmp(interface.Name(), name, nameLength))
			return true;
	}

	return false;
}


status_t
NetServer::_RemoveInterface(const char* name)
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	status_t status = roster.RemoveInterface(name);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not delete interface %s: %s\n",
			Name(), name, strerror(status));
		return status;
	}

	return B_OK;
}


status_t
NetServer::_DisableInterface(const char* name)
{
	BNetworkInterface interface(name);
	int32 flags = interface.Flags();

	// Set interface down
	flags &= ~(IFF_UP | IFF_AUTO_CONFIGURED | IFF_CONFIGURING);

	status_t status = interface.SetFlags(flags);
	if (status != B_OK) {
		fprintf(stderr, "%s: Setting flags failed: %s\n", Name(),
			strerror(status));
		return status;
	}

	fprintf(stderr, "%s: set %s interface down...\n", Name(), name);
	return B_OK;
}


status_t
NetServer::_ConfigureInterface(BMessage& message)
{
	const char* name;
	if (message.FindString("device", &name) != B_OK)
		return B_BAD_VALUE;

	bool startAutoConfig = false;

	int32 flags;
	if (message.FindInt32("flags", &flags) != B_OK)
		flags = IFF_UP;

	bool autoConfigured;
	if (message.FindBool("auto_configured", &autoConfigured) == B_OK
			&& autoConfigured) {
		flags |= IFF_AUTO_CONFIGURED;
	}

	int32 mtu;
	if (message.FindInt32("mtu", &mtu) != B_OK)
		mtu = -1;

	int32 metric;
	if (message.FindInt32("metric", &metric) != B_OK)
		metric = -1;

	BNetworkInterface interface(name);
	if (!interface.Exists()) {
		// the interface does not exist yet, we have to add it first
		BNetworkRoster& roster = BNetworkRoster::Default();

		status_t status = roster.AddInterface(interface);
		if (status != B_OK) {
			fprintf(stderr, "%s: Could not add interface: %s\n",
				interface.Name(), strerror(status));
			return status;
		}
	}

	BNetworkDevice device(name);
	if (device.IsWireless()) {
		const char* networkName;
		if (message.FindString("network", &networkName) != B_OK) {
			// join configured network
			status_t status = _JoinNetwork(message, networkName);
			if (status != B_OK) {
				fprintf(stderr, "%s: joining network \"%s\" failed: %s\n",
					interface.Name(), networkName, strerror(status));
			}
		} else {
			// auto select network to join
			status_t status = _AutoJoinNetwork(name);
			if (status != B_OK) {
				fprintf(stderr, "%s: auto joining network failed: %s\n",
					interface.Name(), strerror(status));
			}
		}
	}

	BMessage addressMessage;
	for (int32 index = 0; message.FindMessage("address", index,
			&addressMessage) == B_OK; index++) {
		int32 family;
		if (addressMessage.FindInt32("family", &family) != B_OK) {
			const char* familyString;
			if (addressMessage.FindString("family", &familyString) == B_OK) {
				if (get_address_family(familyString) == AF_UNSPEC) {
					// we don't support this family
					fprintf(stderr, "%s: Ignore unknown family: %s\n", Name(),
						familyString);
					continue;
				}
			} else
				family = AF_UNSPEC;
		}

		// retrieve addresses

		bool autoConfig;
		if (addressMessage.FindBool("auto_config", &autoConfig) != B_OK)
			autoConfig = false;

		BNetworkAddress address;
		BNetworkAddress mask;
		BNetworkAddress broadcast;
		BNetworkAddress peer;
		BNetworkAddress gateway;

		const char* string;

		if (!autoConfig) {
			if (addressMessage.FindString("address", &string) == B_OK) {
				parse_address(family, string, address);

				if (addressMessage.FindString("mask", &string) == B_OK)
					parse_address(family, string, mask);
			}

			if (addressMessage.FindString("peer", &string) == B_OK)
				parse_address(family, string, peer);

			if (addressMessage.FindString("broadcast", &string) == B_OK)
				parse_address(family, string, broadcast);
		}
		
		if (autoConfig) {
			_QuitLooperForDevice(name);
			startAutoConfig = true;
		} else if (addressMessage.FindString("gateway", &string) == B_OK
			&& parse_address(family, string, gateway)) {
			// add gateway route, if we're asked for it
			interface.RemoveDefaultRoute(family);
				// Try to remove a previous default route, doesn't matter
				// if it fails.

			status_t status = interface.AddDefaultRoute(gateway);
			if (status != B_OK) {
				fprintf(stderr, "%s: Could not add route for %s: %s\n",
					Name(), name, strerror(errno));
			}
		}

		// set address/mask/broadcast/peer

		if (!address.IsEmpty() || !mask.IsEmpty() || !broadcast.IsEmpty()) {
			BNetworkInterfaceAddress interfaceAddress;
			interfaceAddress.SetAddress(address);
			interfaceAddress.SetMask(mask);
			if (!broadcast.IsEmpty())
				interfaceAddress.SetBroadcast(broadcast);
			else if (!peer.IsEmpty())
				interfaceAddress.SetDestination(peer);

			status_t status = interface.SetAddress(interfaceAddress);
			if (status != B_OK) {
				fprintf(stderr, "%s: Setting address failed: %s\n", Name(),
					strerror(status));
				return status;
			}
		}

		// set flags

		if (flags != 0) {
			int32 newFlags = interface.Flags();
			newFlags = (newFlags & ~IFF_CONFIGURING) | flags;
			if (!autoConfigured)
				newFlags &= ~IFF_AUTO_CONFIGURED;

			status_t status = interface.SetFlags(newFlags);
			if (status != B_OK) {
				fprintf(stderr, "%s: Setting flags failed: %s\n", Name(),
					strerror(status));
			}
		}

		// set options

		if (mtu != -1) {
			status_t status = interface.SetMTU(mtu);
			if (status != B_OK) {
				fprintf(stderr, "%s: Setting MTU failed: %s\n", Name(),
					strerror(status));
			}
		}

		if (metric != -1) {
			status_t status = interface.SetMetric(metric);
			if (status != B_OK) {
				fprintf(stderr, "%s: Setting metric failed: %s\n", Name(),
					strerror(status));
			}
		}
	}

	if (startAutoConfig) {
		// start auto configuration
		AutoconfigLooper* looper = new AutoconfigLooper(this, name);
		looper->Run();

		fDeviceMap[name] = looper;
	} else if (!autoConfigured)
		_QuitLooperForDevice(name);

	return B_OK;
}


status_t
NetServer::_ConfigureResolver(BMessage& resolverConfiguration)
{
	// TODO: resolv.conf should be parsed, all information should be
	// maintained and it should be distinguished between user entered
	// and auto-generated parts of the file, with this method only re-writing
	// the auto-generated parts of course.

	BPath path;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append("network/resolv.conf") != B_OK)
		return B_ERROR;

	FILE* file = fopen(path.Path(), "w");
	if (file != NULL) {
		const char* nameserver;
		for (int32 i = 0; resolverConfiguration.FindString("nameserver", i,
				&nameserver) == B_OK; i++) {
			fprintf(file, "nameserver %s\n", nameserver);
		}

		const char* domain;
		if (resolverConfiguration.FindString("domain", &domain) == B_OK)
			fprintf(file, "domain %s\n", domain);

		fclose(file);
	}
	return B_OK;
}


bool
NetServer::_QuitLooperForDevice(const char* device)
{
	LooperMap::iterator iterator = fDeviceMap.find(device);
	if (iterator == fDeviceMap.end())
		return false;

	// there is a looper for this device - quit it
	if (iterator->second->Lock())
		iterator->second->Quit();

	fDeviceMap.erase(iterator);
	return true;
}


AutoconfigLooper*
NetServer::_LooperForDevice(const char* device)
{
	LooperMap::const_iterator iterator = fDeviceMap.find(device);
	if (iterator == fDeviceMap.end())
		return NULL;

	return iterator->second;
}


status_t
NetServer::_ConfigureDevice(const char* device)
{
	// bring interface up, but don't configure it just yet
	BMessage interface;
	interface.AddString("device", device);
	BMessage address;
	address.AddString("family", "inet");
	address.AddBool("auto_config", true);
	interface.AddMessage("address", &address);

	return _ConfigureInterface(interface);
}


void
NetServer::_ConfigureDevices(const char* startPath,
	BMessage* suggestedInterface)
{
	BDirectory directory(startPath);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		struct stat stat;
		BPath path;
		if (entry.GetName(name) != B_OK
			|| entry.GetPath(&path) != B_OK
			|| entry.GetStat(&stat) != B_OK)
			continue;

		if (S_ISBLK(stat.st_mode) || S_ISCHR(stat.st_mode)) {
			if (suggestedInterface != NULL
				&& suggestedInterface->RemoveName("device") == B_OK
				&& suggestedInterface->AddString("device", path.Path()) == B_OK
				&& _ConfigureInterface(*suggestedInterface) == B_OK)
				suggestedInterface = NULL;
			else
				_ConfigureDevice(path.Path());
		} else if (entry.IsDirectory())
			_ConfigureDevices(path.Path(), suggestedInterface);
	}
}


void
NetServer::_ConfigureInterfaces(BMessage* _missingDevice)
{
	BMessage interface;
	uint32 cookie = 0;
	bool missing = false;
	while (fSettings.GetNextInterface(cookie, interface) == B_OK) {
		const char *device;
		if (interface.FindString("device", &device) != B_OK)
			continue;

		bool disabled = false;
		if (interface.FindBool("disabled", &disabled) == B_OK && disabled) {
			// disabled by user request
			_DisableInterface(device);
			continue;
		}

		if (!strncmp(device, "/dev/net/", 9)) {
			// it's a kernel device, check if it's present
			BEntry entry(device);
			if (!entry.Exists()) {
				if (!missing && _missingDevice != NULL) {
					*_missingDevice = interface;
					missing = true;
				}
				continue;
			}
		}

		_ConfigureInterface(interface);
	}
}


void
NetServer::_BringUpInterfaces()
{
	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		fprintf(stderr, "%s: The networking stack doesn't seem to be "
			"available.\n", Name());
		Quit();
		return;
	}
	close(socket);

	_RemoveInvalidInterfaces();

	// First, we look into the settings, and try to bring everything up from there

	BMessage missingDevice;
	_ConfigureInterfaces(&missingDevice);

	// check configuration

	if (!_TestForInterface("loop")) {
		// there is no loopback interface, create one
		BMessage interface;
		interface.AddString("device", "loop");
		BMessage address;
		address.AddString("family", "inet");
		address.AddString("address", "127.0.0.1");
		interface.AddMessage("address", &address);

		_ConfigureInterface(interface);
	}

	// TODO: also check if the networking driver is correctly initialized!
	//	(and check for other devices to take over its configuration)

	if (!_TestForInterface("/dev/net/")) {
		// there is no driver configured - see if there is one and try to use it
		_ConfigureDevices("/dev/net",
			missingDevice.HasString("device") ? &missingDevice : NULL);
	}
}


void
NetServer::_StartServices()
{
	BHandler* services = new (std::nothrow) Services(fSettings.Services());
	if (services != NULL) {
		AddHandler(services);
		fServices = BMessenger(services);
	}
}


status_t
NetServer::_HandleDeviceMonitor(BMessage* message)
{
	int32 opcode;
	const char* path;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		|| message->FindString("path", &path) != B_OK)
		return B_BAD_VALUE;

	if (strncmp(path, "/dev/net", 9)) {
		// not a device entry, ignore
		return B_NAME_NOT_FOUND;
	}
			
	if (opcode == B_ENTRY_CREATED)
		_ConfigureDevice(path);
	else
		_RemoveInterface(path);
		
	return B_OK;
}


status_t
NetServer::_AutoJoinNetwork(const char* name)
{
	BNetworkDevice device(name);

	BMessage message;
	message.AddString("device", name);

	// Choose among configured networks

	uint32 cookie = 0;
	BMessage networkMessage;
	while (fSettings.GetNextNetwork(cookie, networkMessage) == B_OK) {
		status_t status = B_ERROR;
		wireless_network network;
		const char* networkName;
		BNetworkAddress link;

		const char* mac;
		if (networkMessage.FindString("mac", &mac) == B_OK) {
			link.SetTo(AF_LINK, mac);
			status = device.GetNetwork(link, network);
		} else if (networkMessage.FindString("name", &networkName) == B_OK)
			status = device.GetNetwork(networkName, network);

		if (status == B_OK) {
			status = _JoinNetwork(message, network.name);
			printf("auto join network \"%s\": %s\n", network.name,
				strerror(status));
			if (status == B_OK)
				return B_OK;
		}
	}

	// None found, try them all

	wireless_network network;
	cookie = 0;
	while (device.GetNextNetwork(cookie, network) == B_OK) {
		if ((network.flags & B_NETWORK_IS_ENCRYPTED) == 0) {
			status_t status = _JoinNetwork(message, network.name);
			printf("auto join open network \"%s\": %s\n", network.name,
				strerror(status));
			if (status == B_OK)
				return status;
		}

		// TODO: once we have a password manager, use that
	}

	return B_ERROR;
}


status_t
NetServer::_JoinNetwork(const BMessage& message, const char* name)
{
	const char* deviceName;
	if (message.FindString("device", &deviceName) != B_OK)
		return B_BAD_VALUE;

	BNetworkAddress address;
	message.FindFlat("address", &address);

	if (name == NULL)
		message.FindString("name", &name);
	if (name == NULL) {
		// No name specified, we need a network address
		if (address.Family() != AF_LINK)
			return B_BAD_VALUE;
	}

	// Search for a network configuration that may override the defaults

	bool found = false;
	uint32 cookie = 0;
	BMessage networkMessage;
	while (fSettings.GetNextNetwork(cookie, networkMessage) == B_OK) {
		const char* networkName;
		if (networkMessage.FindString("name", &networkName) == B_OK
			&& name != NULL && address.Family() != AF_LINK
			&& !strcmp(name, networkName)) {
			found = true;
			break;
		}

		const char* mac;
		if (networkMessage.FindString("mac", &mac) == B_OK
			&& address.Family() == AF_LINK) {
			BNetworkAddress link(AF_LINK, mac);
			if (link == address) {
				found = true;
				break;
			}
		}
	}

	const char* password;
	if (message.FindString("password", &password) != B_OK && found)
		password = networkMessage.FindString("password");

	// Get network
	BNetworkDevice device(deviceName);
	wireless_network network;

	bool askForConfig = false;
	if ((address.Family() != AF_LINK
			|| device.GetNetwork(address, network) != B_OK)
		&& device.GetNetwork(name, network) != B_OK) {
		// We did not find a network - just ignore that, and continue
		// with some defaults
		strlcpy(network.name, name, sizeof(network.name));
		network.address = address;
		network.authentication_mode = B_NETWORK_AUTHENTICATION_NONE;
		network.cipher = 0;
		network.group_cipher = 0;
		network.key_mode = 0;
		askForConfig = true;
	}

	const char* string;
	if (message.FindString("authentication", &string) == B_OK
		|| (found && networkMessage.FindString("authentication", &string)
				== B_OK)) {
		askForConfig = false;
		if (!strcasecmp(string, "wpa2")) {
			network.authentication_mode = B_NETWORK_AUTHENTICATION_WPA2;
			network.key_mode = B_KEY_MODE_IEEE802_1X;
			network.cipher = network.group_cipher = B_NETWORK_CIPHER_CCMP;
		} else if (!strcasecmp(string, "wpa")) {
			network.authentication_mode = B_NETWORK_AUTHENTICATION_WPA;
			network.key_mode = B_KEY_MODE_IEEE802_1X;
			network.cipher = network.group_cipher = B_NETWORK_CIPHER_TKIP;
		} else if (!strcasecmp(string, "wep")) {
			network.authentication_mode = B_NETWORK_AUTHENTICATION_WEP;
			network.key_mode = B_KEY_MODE_NONE;
			network.cipher = network.group_cipher = B_NETWORK_CIPHER_WEP_40;
		} else if (strcasecmp(string, "none") && strcasecmp(string, "open")) {
			fprintf(stderr, "%s: invalid authentication mode.\n", name);
			askForConfig = true;
		}
	}

	if (!askForConfig
		&& network.authentication_mode == B_NETWORK_AUTHENTICATION_NONE) {
		// we join the network ourselves
		status_t status = set_80211(deviceName, IEEE80211_IOC_SSID,
			network.name, strlen(network.name));
		if (status != B_OK) {
			fprintf(stderr, "%s: joining SSID failed: %s\n", name,
				strerror(status));
			return status;
		}

		return B_OK;
	}

	// Join via wpa_supplicant

	status_t status = be_roster->Launch(kWPASupplicantSignature);
	if (status != B_OK && status != B_ALREADY_RUNNING)
		return status;

	// TODO: listen to notifications from the supplicant!

	BMessage join(kMsgWPAJoinNetwork);
	status = join.AddString("device", deviceName);
	if (status == B_OK)
		status = join.AddString("name", network.name);
	if (status == B_OK)
		status = join.AddFlat("address", &network.address);
	if (status == B_OK && !askForConfig)
		status = join.AddUInt32("authentication", network.authentication_mode);
	if (status == B_OK && password != NULL)
		status = join.AddString("password", password);
	if (status != B_OK)
		return status;

	BMessenger wpaSupplicant(kWPASupplicantSignature);
	BMessage reply;
	status = wpaSupplicant.SendMessage(&join, &reply);
	if (status != B_OK)
		return status;

	return reply.FindInt32("status");
}


status_t
NetServer::_LeaveNetwork(const BMessage& message)
{
	// TODO: not yet implemented
	return B_NOT_SUPPORTED;
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	status_t status;
	NetServer server(status);
	if (status != B_OK) {
		fprintf(stderr, "net_server: Failed to create application: %s\n",
			strerror(status));
		return 1;
	}

	server.Run();
	return 0;
}

