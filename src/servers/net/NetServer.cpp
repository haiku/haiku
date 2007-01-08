/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "AutoconfigLooper.h"
#include "NetServer.h"
#include "Services.h"
#include "Settings.h"

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <TextView.h>

#include <arpa/inet.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <map>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>


typedef std::map<std::string, BLooper*> LooperMap;


class NetServer : public BApplication {
	public:
		NetServer();

		virtual void AboutRequested();
		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage* message);

	private:
		bool _IsValidInterface(int socket, const char* name);
		void _RemoveInvalidInterfaces(int socket);
		bool _TestForInterface(int socket, const char* name);
		status_t _ConfigureInterface(int socket, BMessage& interface,
			bool fromMessage = false);
		bool _QuitLooperForDevice(const char* device);
		BLooper* _LooperForDevice(const char* device);
		status_t _ConfigureDevice(int socket, const char* path);
		void _ConfigureDevices(int socket, const char* path,
			BMessage* suggestedInterface = NULL);
		void _ConfigureInterfaces(int socket, BMessage* _missingDevice = NULL);
		void _BringUpInterfaces();
		void _StartServices();

		Settings	fSettings;
		LooperMap	fDeviceMap;
		BMessenger	fServices;
};


struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
	bool		(*parse_address)(const char* string, sockaddr* _address);
	void		(*set_any_address)(sockaddr* address);
	void		(*set_port)(sockaddr* address, int32 port);
};

// AF_INET family
static bool inet_parse_address(const char* string, sockaddr* address);
static void inet_set_any_address(sockaddr* address);
static void inet_set_port(sockaddr* address, int32 port);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		inet_parse_address,
		inet_set_any_address,
		inet_set_port
	},
	{ -1, NULL, {NULL}, NULL }
};


static bool
inet_parse_address(const char* string, sockaddr* _address)
{
	in_addr inetAddress;

	if (inet_aton(string, &inetAddress) != 1)
		return false;

	sockaddr_in& address = *(sockaddr_in *)_address;
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = 0;
	address.sin_addr = inetAddress;
	memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));

	return true;
}


void
inet_set_any_address(sockaddr* _address)
{
	sockaddr_in& address = *(sockaddr_in*)_address;
	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = 0;
	address.sin_addr.s_addr = INADDR_ANY;
	memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));
}


void
inet_set_port(sockaddr* _address, int32 port)
{
	sockaddr_in& address = *(sockaddr_in*)_address;
	address.sin_port = port;
}


//	#pragma mark -


bool
get_family_index(const char* name, int32& familyIndex)
{
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		for (int32 j = 0; kFamilies[i].identifiers[j]; j++) {
			if (!strcmp(name, kFamilies[i].identifiers[j])) {
				// found a match
				familyIndex = i;
				return true;
			}
		}
	}

	// defaults to AF_INET
	familyIndex = 0;
	return false;
}


int
family_at_index(int32 index)
{
	return kFamilies[index].family;
}


bool
parse_address(int32 familyIndex, const char* argument, struct sockaddr& address)
{
	if (argument == NULL)
		return false;

	return kFamilies[familyIndex].parse_address(argument, &address);
}


void
set_any_address(int32 familyIndex, struct sockaddr& address)
{
	kFamilies[familyIndex].set_any_address(&address);
}


void
set_port(int32 familyIndex, struct sockaddr& address, int32 port)
{
	kFamilies[familyIndex].set_port(&address, port);
}


bool
prepare_request(ifreq& request, const char* name)
{
	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


status_t
get_mac_address(const char* device, uint8* address)
{
	int socket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	ifreq request;
	if (!prepare_request(request, device)) {
		close(socket);
		return B_ERROR;
	}

	if (ioctl(socket, SIOCGIFADDR, &request, sizeof(struct ifreq)) < 0) {
		close(socket);
		return errno;
	}

	close(socket);

	sockaddr_dl &link = *(sockaddr_dl *)&request.ifr_addr;
	if (link.sdl_type != IFT_ETHER)
		return B_BAD_TYPE;

	if (link.sdl_alen == 0)
		return B_ENTRY_NOT_FOUND;

	uint8 *mac = (uint8 *)LLADDR(&link);
	memcpy(address, mac, 6);

	return B_OK;
}


//	#pragma mark -


NetServer::NetServer()
	: BApplication("application/x-vnd.haiku-net_server")
{
}


void
NetServer::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Networking Server\n"
		"\tCopyright " B_UTF8_COPYRIGHT "2006, Haiku.\n", "Ok");
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
}


void
NetServer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			fSettings.Update(message);
			break;

		case kMsgInterfaceSettingsUpdated:
		{
			// we need a socket to talk to the networking stack
			int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
			if (socket < 0)
				break;

			_ConfigureInterfaces(socket);
			close(socket);
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
			if (!message->ReturnAddress().IsTargetLocal()) {
				// for now, we only accept this message from add-ons
				break;
			}

			// we need a socket to talk to the networking stack
			int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
			if (socket < 0)
				break;

			status_t status = _ConfigureInterface(socket, *message, true);

			BMessage reply(B_REPLY);
			reply.AddInt32("status", status);
			message->SendReply(&reply);

			close(socket);
			break;
		}

		default:
			BApplication::MessageReceived(message);
			return;
	}
}


/*!
	Checks if an interface is valid, that is, if it has an address in any
	family, and, in case of ethernet, a hardware MAC address.
*/
bool
NetServer::_IsValidInterface(int socket, const char* name)
{
	ifreq request;
	if (!prepare_request(request, name))
		return B_ERROR;

	// check if it has an address

	int32 addresses = 0;

	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		int familySocket = ::socket(kFamilies[i].family, SOCK_DGRAM, 0);
		if (familySocket < 0)
			continue;

		if (ioctl(familySocket, SIOCGIFADDR, &request, sizeof(struct ifreq)) == 0) {
			if (request.ifr_addr.sa_family == kFamilies[i].family)
				addresses++;
		}

		close(familySocket);
	}

	if (addresses == 0)
		return false;

	// check if it has a hardware address, too, in case of ethernet	

	if (ioctl(socket, SIOCGIFPARAM, &request, sizeof(struct ifreq)) < 0)
		return false;

	int linkSocket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (linkSocket < 0)
		return false;

	prepare_request(request, request.ifr_parameter.device);
	if (ioctl(linkSocket, SIOCGIFADDR, &request, sizeof(struct ifreq)) < 0) {
		close(linkSocket);
		return false;
	}

	close(linkSocket);

	sockaddr_dl &link = *(sockaddr_dl *)&request.ifr_addr;
	if (link.sdl_type == IFT_ETHER && link.sdl_alen < 6)
		return false;

	return true;
}


void
NetServer::_RemoveInvalidInterfaces(int socket)
{
	// get a list of all interfaces

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0) {
		// there are no interfaces yet
		return;
	}

	void *buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory.\n", Name());
		return;
	}

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq *interface = (ifreq *)buffer;

	for (uint32 i = 0; i < count; i++) {
		if (!_IsValidInterface(socket, interface->ifr_name)) {
			// remove invalid interface
			ifreq request;
			if (!prepare_request(request, interface->ifr_name))
				return;

			if (ioctl(socket, SIOCDIFADDR, &request, sizeof(request)) < 0) {
				fprintf(stderr, "%s: Could not delete interface %s: %s\n",
					Name(), interface->ifr_name, strerror(errno));
			}
		}

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE + interface->ifr_addr.sa_len);
	}

	free(buffer);
}


bool
NetServer::_TestForInterface(int socket, const char* name)
{
	// get a list of all interfaces

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return false;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0) {
		// there are no interfaces yet
		return false;
	}

	void *buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory.\n", Name());
		return false;
	}

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return false;

	ifreq *interface = (ifreq *)buffer;
	int32 nameLength = strlen(name);
	bool success = false;

	for (uint32 i = 0; i < count; i++) {
		if (!strncmp(interface->ifr_name, name, nameLength)) {
			success = true;
			break;
		}

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE + interface->ifr_addr.sa_len);
	}

	free(buffer);
	return success;
}


status_t
NetServer::_ConfigureInterface(int socket, BMessage& interface, bool fromMessage)
{
	const char *device;
	if (interface.FindString("device", &device) != B_OK)
		return B_BAD_VALUE;

	ifreq request;
	if (!prepare_request(request, device))
		return B_ERROR;

	bool startAutoConfig = false;

	int32 flags;
	if (interface.FindInt32("flags", &flags) < B_OK)
		flags = IFF_UP;
		
	int32 mtu;
	if (interface.FindInt32("mtu", &mtu) < B_OK)
		mtu = -1;

	int32 metric;
	if (interface.FindInt32("metric", &metric) < B_OK)
		metric = -1;

	BMessage addressMessage;
	for (int32 index = 0; interface.FindMessage("address", index, &addressMessage) == B_OK;
			index++) {
		const char* family;
		if (addressMessage.FindString("family", &family) < B_OK)
			continue;

		int32 familyIndex;
		if (!get_family_index(family, familyIndex)) {
			// we don't support this family
			continue;
		}

		int familySocket = socket;
		if (family_at_index(familyIndex) != AF_INET)
			socket = ::socket(family_at_index(familyIndex), SOCK_DGRAM, 0);
		if (socket < 0) {
			// the family is not available in this environment
			continue;
		}

		uint32 interfaceIndex = 0;
		if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) >= 0)
			interfaceIndex = request.ifr_index;

		if (interfaceIndex == 0) {
			// we need to create the interface first
			request.ifr_parameter.base_name[0] = '\0';
			request.ifr_parameter.device[0] = '\0';
			request.ifr_parameter.sub_type = 0;
				// the default device is okay for us

			if (ioctl(socket, SIOCAIFADDR, &request, sizeof(request)) < 0) {
				fprintf(stderr, "%s: Could not add interface: %s\n", Name(),
					strerror(errno));
				return errno;
			}
		}

		// retrieve addresses

		bool autoConfig;
		if (addressMessage.FindBool("auto config", &autoConfig) != B_OK)
			autoConfig = false;
		if (autoConfig && fromMessage) {
			// we don't accept auto-config messages this way
			continue;
		}

		bool hasAddress = false, hasMask = false, hasPeer = false, hasBroadcast = false;
		struct sockaddr address, mask, peer, broadcast, gateway;
		const char* string;

		if (!autoConfig) {
			if (addressMessage.FindString("address", &string) == B_OK
				&& parse_address(familyIndex, string, address)) {
				hasAddress = true;
	
				if (addressMessage.FindString("mask", &string) == B_OK
					&& parse_address(familyIndex, string, mask))
					hasMask = true;
			}
			if (addressMessage.FindString("peer", &string) == B_OK
				&& parse_address(familyIndex, string, peer))
				hasPeer = true;
			if (addressMessage.FindString("broadcast", &string) == B_OK
				&& parse_address(familyIndex, string, broadcast))
				hasBroadcast = true;
		}

		route_entry route;
		memset(&route, 0, sizeof(route_entry));
		route.flags = RTF_STATIC | RTF_DEFAULT;

		request.ifr_route = route;
		ioctl(socket, SIOCDELRT, &request, sizeof(request));
			// Try to remove a previous default route, doesn't matter
			// if it fails.

		if (autoConfig) {
			// add a default route to make the interface accessible, even without an address
			if (ioctl(socket, SIOCADDRT, &request, sizeof(request)) < 0) {
				fprintf(stderr, "%s: Could not add route for %s: %s\n",
					Name(), device, strerror(errno));
			} else {
				_QuitLooperForDevice(device);
				startAutoConfig = true;
			}
		} else if (addressMessage.FindString("gateway", &string) == B_OK
			&& parse_address(familyIndex, string, gateway)) {
			// add gateway route, if we're asked for it
			route.flags = RTF_STATIC | RTF_DEFAULT | RTF_GATEWAY;
			route.gateway = &gateway;

			request.ifr_route = route;
			if (ioctl(socket, SIOCADDRT, &request, sizeof(request)) < 0) {
				fprintf(stderr, "%s: Could not add route for %s: %s\n",
					Name(), device, strerror(errno));
			}
		}

		// set addresses

		if (hasAddress) {
			memcpy(&request.ifr_addr, &address, address.sa_len);
	
			if (ioctl(familySocket, SIOCSIFADDR, &request, sizeof(struct ifreq)) < 0) {
				fprintf(stderr, "%s: Setting address failed: %s\n", Name(), strerror(errno));
				continue;
			}
		}
	
		if (ioctl(familySocket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Getting flags failed: %s\n", Name(), strerror(errno));
			continue;
		}
		int32 currentFlags = request.ifr_flags;

		if (!hasMask && hasAddress && family_at_index(familyIndex) == AF_INET
			&& ioctl(familySocket, SIOCGIFNETMASK, &request, sizeof(struct ifreq)) == 0
			&& request.ifr_mask.sa_family == AF_UNSPEC) {
			// generate standard netmask if it doesn't have one yet
			sockaddr_in *netmask = (sockaddr_in *)&mask;
			netmask->sin_len = sizeof(sockaddr_in);
			netmask->sin_family = AF_INET;

			// choose default netmask depending on the class of the address
			in_addr_t net = ((sockaddr_in *)&address)->sin_addr.s_addr;
			if (IN_CLASSA(net)
				|| (ntohl(net) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET) {
				// class A, or loopback
				netmask->sin_addr.s_addr = IN_CLASSA_NET;
			} else if (IN_CLASSB(net)) {
				// class B
				netmask->sin_addr.s_addr = IN_CLASSB_NET;
			} else {
				// class C and rest
				netmask->sin_addr.s_addr = IN_CLASSC_NET;
			}

			hasMask = true;
		}
		if (hasMask) {
			memcpy(&request.ifr_mask, &mask, mask.sa_len);

			if (ioctl(familySocket, SIOCSIFNETMASK, &request, sizeof(struct ifreq)) < 0) {
				fprintf(stderr, "%s: Setting subnet mask failed: %s\n", Name(), strerror(errno));
				continue;
			}
		}

		if (!hasBroadcast && hasAddress && (currentFlags & IFF_BROADCAST)
			&& family_at_index(familyIndex) == AF_INET
			&& ioctl(familySocket, SIOCGIFBRDADDR, &request, sizeof(struct ifreq)) == 0
			&& request.ifr_mask.sa_family == AF_UNSPEC) {
				// generate standard broadcast address if it doesn't have one yet
			sockaddr_in *broadcastAddr = (sockaddr_in *)&broadcast;
			uint32 maskValue = ((sockaddr_in *)&mask)->sin_addr.s_addr;
			uint32 broadcastValue = ((sockaddr_in *)&address)->sin_addr.s_addr;
			broadcastValue = (broadcastValue & maskValue) | ~maskValue;
			broadcastAddr->sin_len = sizeof(sockaddr_in);
			broadcastAddr->sin_family = AF_INET;
			broadcastAddr->sin_addr.s_addr = broadcastValue;
			hasBroadcast = true;
		}
		if (hasBroadcast) {
			memcpy(&request.ifr_broadaddr, &broadcast, broadcast.sa_len);

			if (ioctl(familySocket, SIOCSIFBRDADDR, &request, sizeof(struct ifreq)) < 0) {
				fprintf(stderr, "%s: Setting broadcast address failed: %s\n", Name(), strerror(errno));
				continue;
			}
		}

		if (hasPeer) {
			memcpy(&request.ifr_dstaddr, &peer, peer.sa_len);

			if (ioctl(familySocket, SIOCSIFDSTADDR, &request, sizeof(struct ifreq)) < 0) {
				fprintf(stderr, "%s: Setting peer address failed: %s\n", Name(), strerror(errno));
				continue;
			}
		}

		// set flags

		if (flags != 0) {
			request.ifr_flags = currentFlags | flags;
			if (ioctl(familySocket, SIOCSIFFLAGS, &request, sizeof(struct ifreq)) < 0)
				fprintf(stderr, "%s: Setting flags failed: %s\n", Name(), strerror(errno));
		}

		// set options

		if (mtu != -1) {
			request.ifr_mtu = mtu;
			if (ioctl(familySocket, SIOCSIFMTU, &request, sizeof(struct ifreq)) < 0)
				fprintf(stderr, "%s: Setting MTU failed: %s\n", Name(), strerror(errno));
		}

		if (metric != -1) {
			request.ifr_metric = metric;
			if (ioctl(familySocket, SIOCSIFMETRIC, &request, sizeof(struct ifreq)) < 0)
				fprintf(stderr, "%s: Setting metric failed: %s\n", Name(), strerror(errno));
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


bool
NetServer::_QuitLooperForDevice(const char* device)
{
	LooperMap::iterator iterator = fDeviceMap.find(device);
	if (iterator == fDeviceMap.end())
		return false;

	// there is a looper for this device - quit it
	iterator->second->Lock();
	iterator->second->Quit();

	fDeviceMap.erase(iterator);
	return true;
}


BLooper*
NetServer::_LooperForDevice(const char* device)
{
	LooperMap::const_iterator iterator = fDeviceMap.find(device);
	if (iterator == fDeviceMap.end())
		return NULL;

	return iterator->second;
}


status_t
NetServer::_ConfigureDevice(int socket, const char* path)
{
	// bring interface up, but don't configure it just yet
	BMessage interface;
	interface.AddString("device", path);
	BMessage address;
	address.AddString("family", "inet");
	address.AddBool("auto config", true);
	interface.AddMessage("address", &address);

	return _ConfigureInterface(socket, interface);
}


void
NetServer::_ConfigureDevices(int socket, const char* startPath,
	BMessage* suggestedInterface)
{
	BDirectory directory(startPath);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		struct stat stat;
		BPath path;
		if (entry.GetName(name) != B_OK
			|| !strcmp(name, "stack")
			|| entry.GetPath(&path) != B_OK
			|| entry.GetStat(&stat) != B_OK)
			continue;

		if (S_ISBLK(stat.st_mode) || S_ISCHR(stat.st_mode)) {
			if (suggestedInterface != NULL
				&& suggestedInterface->RemoveName("device") == B_OK
				&& suggestedInterface->AddString("device", path.Path()) == B_OK
				&& _ConfigureInterface(socket, *suggestedInterface) == B_OK)
				suggestedInterface = NULL;
			else
				_ConfigureDevice(socket, path.Path());
		} else if (entry.IsDirectory())
			_ConfigureDevices(socket, path.Path(), suggestedInterface);
	}
}


void
NetServer::_ConfigureInterfaces(int socket, BMessage* _missingDevice)
{
	BMessage interface;
	uint32 cookie = 0;
	bool missing = false;
	while (fSettings.GetNextInterface(cookie, interface) == B_OK) {
		const char *device;
		if (interface.FindString("device", &device) != B_OK)
			continue;

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

		_ConfigureInterface(socket, interface);
	}
}


void
NetServer::_BringUpInterfaces()
{
	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		fprintf(stderr, "%s: The networking stack doesn't seem to be available.\n",
			Name());
		Quit();
		return;
	}

	_RemoveInvalidInterfaces(socket);

	// First, we look into the settings, and try to bring everything up from there

	BMessage missingDevice;
	_ConfigureInterfaces(socket, &missingDevice);

	// check configuration

	if (!_TestForInterface(socket, "loop")) {
		// there is no loopback interface, create one
		BMessage interface;
		interface.AddString("device", "loop");
		BMessage address;
		address.AddString("family", "inet");
		address.AddString("address", "127.0.0.1");
		interface.AddMessage("address", &address);

		_ConfigureInterface(socket, interface);
	}

	// TODO: also check if the networking driver is correctly initialized!
	//	(and check for other devices to take over its configuration)

	if (!_TestForInterface(socket, "/dev/net/")) {
		// there is no driver configured - see if there is one and try to use it
		_ConfigureDevices(socket, "/dev/net",
			missingDevice.HasString("device") ? &missingDevice : NULL);
	}

	close(socket);
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


//	#pragma mark -


int
main()
{
	NetServer app;
	app.Run();

	return 0;
}

