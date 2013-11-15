/*
 * Copyright 2004-2013 Haiku Inc. All rights reserved.
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
#include <net/route.h>
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
#include <NetworkRoster.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>


static status_t
GetDefaultGateway(const char* name, BString& gateway)
{
	uint32 index = 0;
	BNetworkRoster& roster = BNetworkRoster::Default();
	route_entry routeEntry;
	while (roster.GetNextRoute(&index, routeEntry, name) == B_OK) {
		if ((routeEntry.flags & RTF_GATEWAY) != 0) {
			sockaddr_in* inetAddress = (sockaddr_in*)routeEntry.gateway;
			gateway = inet_ntoa(inetAddress->sin_addr);
			break;
		}
		index++;
	}

	return B_OK;
}


// #pragma mark -


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


/*!	Returns all associated wireless networks. */
std::vector<wireless_network>
Settings::WirelessNetworks()
{
	std::vector<wireless_network> networks;

	BNetworkDevice device(fName);
	if (device.IsWireless()) {
		wireless_network wirelessNetwork;
		uint32 cookie = 0;
		while (device.GetNextAssociatedNetwork(cookie, wirelessNetwork) == B_OK)
			networks.push_back(wirelessNetwork);
	}

	return networks;
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

	if (GetDefaultGateway(fName.String(), fGateway) != B_OK)
		return;

	uint32 flags = interface.Flags();

	fAuto = (flags & (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;
	fDisabled = (flags & IFF_UP) == 0;

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
