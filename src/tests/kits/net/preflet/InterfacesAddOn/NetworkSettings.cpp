/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Vegard Wærp, vegarwa@online.no
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "NetworkSettings.h"

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
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>


NetworkSettings::NetworkSettings(const char* name)
	:
	fIPv4Auto(true),
	fIPv6Auto(true),
	fDisabled(false),
	fNameServers(5, true)
{
	fSocket4 = socket(AF_INET, SOCK_DGRAM, 0);
	fSocket6 = socket(AF_INET6, SOCK_DGRAM, 0);

	fName = name;

	ReadConfiguration();
}


NetworkSettings::~NetworkSettings()
{
	close(fSocket4);
	close(fSocket6);
}


void
NetworkSettings::ReadConfiguration()
{
	BNetworkInterface fNetworkInterface(fName);

	int32 zeroAddrV4 = fNetworkInterface.FindFirstAddress(AF_INET);
	int32 zeroAddrV6 = fNetworkInterface.FindFirstAddress(AF_INET6);

	BNetworkInterfaceAddress netIntAddr4;
	BNetworkInterfaceAddress netIntAddr6;

	if (zeroAddrV4 != errno) {
		fNetworkInterface.GetAddressAt(zeroAddrV4, netIntAddr4);
		fIPv4Addr = netIntAddr4.Address();
		fIPv4Mask = netIntAddr4.Mask();
	}

	if (zeroAddrV6 != errno) {
		fNetworkInterface.GetAddressAt(zeroAddrV6, netIntAddr6);
		fIPv6Addr = netIntAddr6.Address();
		fIPv6Mask = netIntAddr6.Mask();
	}

	// Obtain gateway
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket4, SIOCGRTSIZE, &config, sizeof(config)) < 0)
		return;

	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return;

	void* buffer = malloc(size);
	if (buffer == NULL)
		return;

	MemoryDeleter bufferDeleter(buffer);
	config.ifc_len = size;
	config.ifc_buf = buffer;

	if (ioctl(fSocket4, SIOCGRTTABLE, &config, sizeof(config)) < 0)
		return;

	ifreq* interface = (ifreq*)buffer;
	ifreq* end = (ifreq*)((uint8*)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		if ((route.flags & RTF_GATEWAY) != 0) {
			sockaddr_in* inetAddress = (sockaddr_in*)route.gateway;
			fGateway = inet_ntoa(inetAddress->sin_addr);
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

	// Obtain selfconfiguration options

	// TODO : This needs to be determined by the protocol flags
	// instead of the interface flag... protocol flags don't seem
	// to be complete yet. (netIntAddr4.Flags() and netIntAddr6.Flags())

	fIPv4Auto = (fNetworkInterface.Flags()
		& (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;

	fIPv6Auto = (fNetworkInterface.Flags()
		& (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;

	fDisabled = (fNetworkInterface.Flags() & IFF_UP) == 0;

	// Read wireless network from interfaces

	fWirelessNetwork.SetTo(NULL);

	BPath path;
	find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
	path.Append("network");
	path.Append("interfaces");

	void* handle = load_driver_settings(path.Path());
	if (handle != NULL) {
		const driver_settings* settings = get_driver_settings(handle);
		if (settings != NULL) {
			for (int32 i = 0; i < settings->parameter_count; i++) {
				driver_parameter& top = settings->parameters[i];
				if (!strcmp(top.name, "interface")) {
					// The name of the interface can either be the value of
					// the "interface" parameter, or a separate "name" parameter
					const char* name = NULL;
					if (top.value_count > 0) {
						name = top.values[0];
						if (fName != name)
							continue;
					}

					// search "network" parameter
					for (int32 j = 0; j < top.parameter_count; j++) {
						driver_parameter& sub = top.parameters[j];
						if (name == NULL && !strcmp(sub.name, "name")
							&& sub.value_count > 0) {
							name = sub.values[0];
							if (fName != sub.values[0])
								break;
						}

						if (!strcmp(sub.name, "network")
							&& sub.value_count > 0) {
							fWirelessNetwork.SetTo(sub.values[0]);
							break;
						}
					}

					// We found our interface
					if (fName == name)
						break;
				}
			}
		}
		unload_driver_settings(handle);
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


BNetworkAddress
NetworkSettings::IPAddr(int family)
{
	if (family == AF_INET6)
		return fIPv6Addr;

	return fIPv4Addr;
}


const char*
NetworkSettings::IP(int family)
{
	if (family == AF_INET6)
		return fIPv6Addr.ToString();

	return fIPv4Addr.ToString();
}


const char*
NetworkSettings::Netmask(int family)
{
	if (family == AF_INET6)
		return fIPv6Mask.ToString();

	return fIPv4Mask.ToString();
}


int32
NetworkSettings::PrefixLen(int family)
{
	if (family == AF_INET6)
		return fIPv6Mask.PrefixLength();

	return fIPv4Mask.PrefixLength();
}

