/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 */


#include "Setting.h"

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

#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <File.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>


Setting::Setting(const char *name)
	:
	fAuto(true),
	fEnabled(false)
{
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	fName = name;
	
	ReadConfiguration();	
}

Setting::~Setting()
{
	close(fSocket);
}

bool
Setting::_PrepareRequest(struct ifreq& request)
{
	//This function is used for talking direct to the stack. 
	//It´s used by _ShowConfiguration.

	const char* name = fName.String();

	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}


void
Setting::ReadConfiguration()
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
	if (ioctl(fSocket, SIOCGIFNETMASK, &request,
			sizeof(request)) < 0) {
		return;
	}

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

	void *buffer = malloc(size);
	if (buffer == NULL)
		return;

	MemoryDeleter bufferDeleter(buffer);
	config.ifc_len = size;
	config.ifc_buf = buffer;

	if (ioctl(fSocket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq *interface = (ifreq *)buffer;
	ifreq *end = (ifreq *)((uint8 *)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		if (route.flags & RTF_GATEWAY) {
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

	fAuto = (flags & IFF_AUTO_CONFIGURED) != 0;
	fEnabled = (flags & IFF_UP) != 0;
	
	if (ioctl(fSocket, SIOCGIFMEDIA, &request, sizeof(struct ifreq)) == 0)
		fMedia = request.ifr_media;

	// read resolv.conf for the dns.
	fNameservers.MakeEmpty();

	res_init();
	res_state state = __res_state();

	if (state != NULL) {
		for (int i = 0; i < state->nscount; i++) {
			fNameservers.AddItem(
				new BString(inet_ntoa(state->nsaddr_list[i].sin_addr)));
		}
	}
}
