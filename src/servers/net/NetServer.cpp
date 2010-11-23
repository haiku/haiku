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
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <Server.h>
#include <TextView.h>
#include <FindDirectory.h>

#include "AutoconfigLooper.h"
#include "Services.h"
#include "Settings.h"


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
			void				_HandleDeviceMonitor(BMessage* message);

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
NetServer::_ConfigureInterface(BMessage& interface)
{
	const char* device;
	if (interface.FindString("device", &device) != B_OK)
		return B_BAD_VALUE;

	bool startAutoConfig = false;

	int32 flags;
	if (interface.FindInt32("flags", &flags) < B_OK)
		flags = IFF_UP;

	bool autoConfigured;
	if (interface.FindBool("auto", &autoConfigured) == B_OK && autoConfigured)
		flags |= IFF_AUTO_CONFIGURED;

	int32 mtu;
	if (interface.FindInt32("mtu", &mtu) < B_OK)
		mtu = -1;

	int32 metric;
	if (interface.FindInt32("metric", &metric) < B_OK)
		metric = -1;

	BMessage addressMessage;
	for (int32 index = 0; interface.FindMessage("address", index,
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

		BNetworkInterface interface(device);
		if (!interface.Exists()) {
			// the interface does not exist yet, we have to add it first
			BNetworkRoster& roster = BNetworkRoster::Default();

			status_t status = roster.AddInterface(interface);
			if (status != B_OK) {
				fprintf(stderr, "%s: Could not add interface: %s\n", Name(),
					strerror(status));
				return status;
			}
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
			_QuitLooperForDevice(device);
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
					Name(), device, strerror(errno));
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
		AutoconfigLooper* looper = new AutoconfigLooper(this, device);
		looper->Run();

		fDeviceMap[device] = looper;
	}

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


void
NetServer::_HandleDeviceMonitor(BMessage* message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED))
		return;

	const char* path;
	const char* watchedPath;
	if (message->FindString("watched_path", &watchedPath) != B_OK
		|| message->FindString("path", &path) != B_OK)
		return;

	if (opcode == B_ENTRY_CREATED)
		_ConfigureDevice(path);
	else
		_RemoveInterface(path);
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

