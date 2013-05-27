/*
 * Copyright 2004-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Vegard Wærp, vegarwa@online.no
 */


#include "Settings.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <driver_settings.h>
#include <File.h>
#include <FindDirectory.h>
#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>


Settings::Settings(const char* name)
	:
	fAuto(true),
	fDisabled(false),
	fNameServers(5, true)
{
	fName = name;

	ReadConfiguration();
}


Settings::~Settings()
{
}


static status_t
GetDefaultGateway(BString& gateway)
{
	// TODO: This method is here because BNetworkInterface
	// doesn't yet have any route getting methods

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser fdCloser(socket);

	// Obtain gateway
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return errno;

	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return B_ERROR;

	void* buffer = malloc(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter bufferDeleter(buffer);
	config.ifc_len = size;
	config.ifc_buf = buffer;

	if (ioctl(socket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return errno;

	ifreq* interface = (ifreq*)buffer;
	ifreq* end = (ifreq*)((uint8*)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		if ((route.flags & RTF_GATEWAY) != 0) {
			sockaddr_in* inetAddress = (sockaddr_in*)route.gateway;
			gateway = inet_ntoa(inetAddress->sin_addr);
		}

		int32 addressSize = 0;
		if (route.destination != NULL)
			addressSize += route.destination->sa_len;
		if (route.mask != NULL)
			addressSize += route.mask->sa_len;
		if (route.gateway != NULL)
			addressSize += route.gateway->sa_len;

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ sizeof(route_entry) + addressSize);
	}

	return B_OK;
}


void
Settings::ReadConfiguration()
{
	BNetworkInterface interface(fName);
	BNetworkInterfaceAddress address;

	// TODO: We only get the first address
	if (interface.GetAddressAt(0, address) != B_OK)
		return;

	fIP = address.Address().ToString();
	fNetmask = address.Mask().ToString();

	if (GetDefaultGateway(fGateway) != B_OK)
		return;

	uint32 flags = interface.Flags();

	fAuto = (flags & (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;
	fDisabled = (flags & IFF_UP) == 0;

	// Read wireless network from interfaces

	fWirelessNetwork.SetTo(NULL);

	BNetworkDevice networkDevice(fName);
	if (networkDevice.IsWireless()) {
		uint32 networkIndex = 0;
		wireless_network wirelessNetwork;
		// TODO: We only get the first associated network for now
		if (networkDevice.GetNextAssociatedNetwork(networkIndex,
				wirelessNetwork) == B_OK) {
			fWirelessNetwork.SetTo(wirelessNetwork.name);
		}
	}

	// read resolv.conf for the dns.
	fNameServers.MakeEmpty();

	res_init();
	res_state state = __res_state();

	if (state != NULL) {
		for (int i = 0; i < state->nscount; i++) {
			fNameServers.AddItem(
				new BString(inet_ntoa(state->nsaddr_list[i].sin_addr)));
		}
		fDomain = state->dnsrch[0];
	}
}
