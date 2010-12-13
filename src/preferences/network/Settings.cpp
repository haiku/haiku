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
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>


Settings::Settings(const char* name)
	:
	fAuto(true),
	fDisabled(false),
	fNameServers(5, true)
{
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	fName = name;

	ReadConfiguration();
}


Settings::~Settings()
{
	close(fSocket);
}


bool
Settings::_PrepareRequest(struct ifreq& request)
{
	// This function is used for talking direct to the stack.
	// It´s used by _ShowConfiguration.

	const char* name = fName.String();

	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


void
Settings::ReadConfiguration()
{
	ifreq request;
	if (!_PrepareRequest(request))
		return;

	BString text = "dummy";
	char address[32];
	sockaddr_in* inetAddress = NULL;

	// Obtain IP.
	if (ioctl(fSocket, SIOCGIFADDR, &request, sizeof(request)) < 0)
		return;

	inetAddress = (sockaddr_in*)&request.ifr_addr;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
			sizeof(address)) == NULL) {
		return;
	}

	fIP = address;

	// Obtain netmask.
	if (ioctl(fSocket, SIOCGIFNETMASK, &request, sizeof(request)) < 0)
		return;

	inetAddress = (sockaddr_in*)&request.ifr_mask;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
			sizeof(address)) == NULL) {
		return;
	}

	fNetmask = address;

	// Obtain gateway
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
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

	if (ioctl(fSocket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq* interface = (ifreq*)buffer;
	ifreq* end = (ifreq*)((uint8*)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		if ((route.flags & RTF_GATEWAY) != 0) {
			inetAddress = (sockaddr_in*)route.gateway;
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

	uint32 flags = 0;
	if (ioctl(fSocket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0)
		flags = request.ifr_flags;

	fAuto = (flags & (IFF_AUTO_CONFIGURED | IFF_CONFIGURING)) != 0;
	fDisabled = (flags & IFF_UP) == 0;

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
