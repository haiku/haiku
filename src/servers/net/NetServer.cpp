/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Settings.h"

#include <Alert.h>
#include <Application.h>
#include <Entry.h>
#include <TextView.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


class NetServer : public BApplication {
	public:
		NetServer();

		virtual void AboutRequested();
		virtual void ReadyToRun();

	private:
		bool _PrepareRequest(ifreq& request, const char* name);
		status_t _ConfigureInterface(int socket, BMessage& interface);
		void _BringUpInterfaces();

		Settings	fSettings;
};


struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
	bool		(*parse_address)(const char* string, sockaddr* _address);
};

// AF_INET family
static bool inet_parse_address(const char* string, sockaddr* address);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		inet_parse_address
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


//	#pragma mark -


static bool
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


static bool
parse_address(int32 familyIndex, const char* argument, struct sockaddr& address)
{
	if (argument == NULL)
		return false;

	return kFamilies[familyIndex].parse_address(argument, &address);
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
	_BringUpInterfaces();
}


bool
NetServer::_PrepareRequest(ifreq& request, const char* name)
{
	if (strlen(name) > IF_NAMESIZE) {
		fprintf(stderr, "%s: interface name \"%s\" is too long.\n", Name(), name);
		return false;
	}

	strcpy(request.ifr_name, name);
	return true;
}


status_t
NetServer::_ConfigureInterface(int socket, BMessage& interface)
{
	const char *device;
	if (interface.FindString("device", &device) != B_OK)
		return B_BAD_VALUE;

	ifreq request;
	if (!_PrepareRequest(request, device))
		return B_ERROR;

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
		if (kFamilies[familyIndex].family != AF_INET)
			socket = ::socket(kFamilies[familyIndex].family, SOCK_DGRAM, 0);
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

		bool hasAddress = false, hasMask = false, hasPeer = false, hasBroadcast = false;
		struct sockaddr address, mask, peer, broadcast;

		const char* string;
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

		if (!hasMask && hasAddress && kFamilies[familyIndex].family == AF_INET
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
			} if (IN_CLASSB(net)) {
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
			&& kFamilies[familyIndex].family == AF_INET
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

	return B_OK;
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

	// First, we look into the settings, and try to bring everything up from there

	BMessage interface;
	uint32 cookie = 0;
	while (fSettings.GetNextInterface(cookie, interface) == B_OK) {
		const char *device;
		if (interface.FindString("device", &device) != B_OK)
			continue;

		if (!strncmp(device, "/dev/net/", 9)) {
			// it's a kernel device, check if it's present
			BEntry entry(device);
			if (!entry.Exists())
				continue;
		}

		// try to bring the interface up

		status_t status = _ConfigureInterface(socket, interface);
		if (status < B_OK)
			continue;
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

