/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/NetDefs.h>

#include <stdio.h>

const mac_addr_t kBroadcastMACAddress(
	(uint8[6]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
const mac_addr_t kNoMACAddress((uint8[6]){0, 0, 0, 0, 0, 0});

// net service names
const char *const kEthernetServiceName = "ethernet";
const char *const kARPServiceName = "arp";
const char *const kIPServiceName = "ip";
const char *const kUDPServiceName = "udp";
const char *const kTCPServiceName = "tcp";


// constructor
NetService::NetService(const char *name)
	: fName(name)
{
}

// destructor
NetService::~NetService()
{
}

// NetServiceName
const char *
NetService::NetServiceName()
{
	return fName;
}

// CountSubNetServices
int
NetService::CountSubNetServices() const
{
	return 0;
}

// SubNetServiceAt
NetService *
NetService::SubNetServiceAt(int index) const
{
	return NULL;
}

// FindSubNetService
NetService *
NetService::FindSubNetService(const char *name) const
{
	int count = CountSubNetServices();
	for (int i = 0; i < count; i++) {
		NetService *service = SubNetServiceAt(i);
		if (strcmp(service->NetServiceName(), name) == 0)
			return service;
	}

	return NULL;
}
