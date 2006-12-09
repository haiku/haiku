/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>

#include "pxe_undi.h"

#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

#ifdef TRACE_NETWORK

static void
hex_dump(const void *_data, int length)
{
	uint8 *data = (uint8*)_data;
	for (int i = 0; i < length; i++) {
		if (i % 4 == 0) {
			if (i % 32 == 0) {
				if (i != 0)
					TRACE("\n");
				TRACE("%03x: ", i);
			} else
				TRACE(" ");
		}

		TRACE("%02x", data[i]);
	}
	TRACE("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


class UNDI : public EthernetInterface
{
public:
						UNDI();
	virtual 			~UNDI();

	status_t 			Init();

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void 		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
	mac_addr_t			fMACAddress;
};


UNDI::UNDI()
{
	TRACE("UNDI::UNDI\n");
}


UNDI::~UNDI()
{
	TRACE("UNDI::~UNDI\n");
}


status_t
UNDI::Init()
{
	TRACE("UNDI::Init\n");
	return B_OK;	
}


mac_addr_t
UNDI::MACAddress() const
{
	return fMACAddress;
}

void *
UNDI::AllocateSendReceiveBuffer(size_t size)
{
	TRACE("UNDI::AllocateSendReceiveBuffer, size %ld\n", size);
	return 0;
}


void
UNDI::FreeSendReceiveBuffer(void *buffer)
{
	TRACE("UNDI::FreeSendReceiveBuffer\n");
}


ssize_t
UNDI::Send(const void *buffer, size_t size)
{
	TRACE("UNDI::Send, size %ld\n", size);
	return 0;
}


ssize_t
UNDI::Receive(void *buffer, size_t size)
{
	TRACE("UNDI::Receive, size %ld\n", size);
	return 0;
}


status_t
platform_net_stack_init()
{
	TRACE("platform_net_stack_init\n");

	pxe_undi_init();

	UNDI *interface = new(nothrow) UNDI;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init();
	if (error != B_OK) {
		delete interface;
		return error;
	}

	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	return B_OK;
}
