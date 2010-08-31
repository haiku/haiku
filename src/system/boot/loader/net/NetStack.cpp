/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/NetStack.h>

#include <new>

#include <stdio.h>

#include <boot/net/ARP.h>
#include <boot/net/Ethernet.h>
#include <boot/net/IP.h>
#include <boot/net/UDP.h>
#include <boot/net/TCP.h>


// sNetStack
NetStack *NetStack::sNetStack = NULL;

// constructor
NetStack::NetStack()
	: fEthernetInterface(NULL),
		fEthernetService(NULL),
		fARPService(NULL),
		fIPService(NULL),
		fUDPService(NULL),
		fTCPService(NULL)
{
}

// destructor
NetStack::~NetStack()
{
	delete fTCPService;
	delete fUDPService;
	delete fIPService;
	delete fARPService;
	delete fEthernetService;
	delete fEthernetInterface;
}

// Init
status_t
NetStack::Init()
{
	// create services

	// ethernet service
	fEthernetService = new(nothrow) EthernetService;
	if (!fEthernetService)
		return B_NO_MEMORY;

	// ARP service
	fARPService = new(nothrow) ARPService(fEthernetService);
	if (!fARPService)
		return B_NO_MEMORY;
	status_t error = fARPService->Init();
	if (error != B_OK)
		return error;

	// IP service
	fIPService = new(nothrow) IPService(fEthernetService, fARPService);
	if (!fIPService)
		return B_NO_MEMORY;
	error = fIPService->Init();
	if (error != B_OK)
		return error;

	// UDP service
	fUDPService = new(nothrow) UDPService(fIPService);
	if (!fUDPService)
		return B_NO_MEMORY;
	error = fUDPService->Init();
	if (error != B_OK)
		return error;

#ifdef __POWERPC__
	// TCP service
	fTCPService = new(nothrow) TCPService(fIPService);
	if (fTCPService == NULL)
		return B_NO_MEMORY;
	error = fTCPService->Init();
	if (error != B_OK)
		return error;
#endif

	return B_OK;
}

// CreateDefault
status_t
NetStack::CreateDefault()
{
	if (sNetStack)
		return B_OK;

	NetStack *netStack = new(nothrow) NetStack;
	if (!netStack)
		return B_NO_MEMORY;

	status_t error = netStack->Init();
	if (error != B_OK) {
		delete netStack;
		return error;
	}

	sNetStack = netStack;
	return B_OK;
}

// Default
NetStack *
NetStack::Default()
{
	return sNetStack;
}

// AddEthernetInterface
status_t
NetStack::AddEthernetInterface(EthernetInterface *interface)
{
	if (!interface)
		return B_BAD_VALUE;

	// we support only one network interface at the moment
	if (fEthernetInterface)
		return B_BAD_VALUE;

	if (!fEthernetService)
		return B_NO_INIT;

	status_t error = fEthernetService->Init(interface);
	if (error != B_OK)
		return error;

	fEthernetInterface = interface;
	return B_OK;
}


// #pragma mark -

status_t
net_stack_init()
{
	status_t error = NetStack::CreateDefault();
	if (error != B_OK)
		return error;

	return platform_net_stack_init();
}

