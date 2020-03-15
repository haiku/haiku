/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Faerber <andreas.faerber@web.de>
 * Copyright 2002, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <boot/platform.h>
#include <boot/net/Ethernet.h>
#include <boot/net/IP.h>
#include <boot/net/NetStack.h>
#include <platform/openfirmware/openfirmware.h>


//#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


class OFEthernetInterface : public EthernetInterface {
public:
						OFEthernetInterface();
	virtual				~OFEthernetInterface();

			status_t	Init(const char *device, const char *parameters);

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
			status_t	FindMACAddress();

private:
			intptr_t	fHandle;
			mac_addr_t	fMACAddress;
};


#ifdef TRACE_NETWORK

static void
hex_dump(const void *_data, int length)
{
	uint8 *data = (uint8*)_data;
	for (int i = 0; i < length; i++) {
		if (i % 4 == 0) {
			if (i % 32 == 0) {
				if (i != 0)
					printf("\n");
				printf("%03x: ", i);
			} else
				printf(" ");
		}

		printf("%02x", data[i]);
	}
	printf("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


// #pragma mark -


OFEthernetInterface::OFEthernetInterface()
	:
	EthernetInterface(),
	fHandle(OF_FAILED),
	fMACAddress(kNoMACAddress)
{
}


OFEthernetInterface::~OFEthernetInterface()
{
	if (fHandle != OF_FAILED)
		of_close(fHandle);
}


status_t
OFEthernetInterface::FindMACAddress()
{
	intptr_t package = of_instance_to_package(fHandle);

	// get MAC address
	int bytesRead = of_getprop(package, "local-mac-address", &fMACAddress,
		sizeof(fMACAddress));
	if (bytesRead == (int)sizeof(fMACAddress))
		return B_OK;

	// Failed to get the MAC address of the network device. The system may
	// have a global standard MAC address.
	bytesRead = of_getprop(gChosen, "mac-address", &fMACAddress,
		sizeof(fMACAddress));
	if (bytesRead == (int)sizeof(fMACAddress)) {
		return B_OK;
	}

	// On Sun machines, there is a global word 'mac-address' which returns
	// the size and a pointer to the MAC address
	size_t size;
	void* ptr;
	if (of_interpret("mac-address", 0, 2, &size, &ptr) != OF_FAILED) {
		if (size == sizeof(fMACAddress)) {
			memcpy(&fMACAddress, ptr, size);
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
OFEthernetInterface::Init(const char *device, const char *parameters)
{
	if (!device)
		return B_BAD_VALUE;

	// open device
	fHandle = of_open(device);
	if (fHandle == OF_FAILED) {
		printf("opening ethernet device failed\n");
		return B_ERROR;
	}

	if (FindMACAddress() != B_OK) {
		printf("Failed to get MAC address\n");
		return B_ERROR;
	}

	// get IP address

	// Note: This is a non-standardized way. On my Mac mini the response of the
	// DHCP server is stored as property of /chosen. We try to get it and use
	// the IP address we find in there.
	// TODO Sun machines may use bootp-response instead?
	struct {
		uint8	irrelevant[16];
		uint32	ip_address;
		// ...
	} dhcpResponse;
	int bytesRead = of_getprop(gChosen, "dhcp-response", &dhcpResponse,
		sizeof(dhcpResponse));
	if (bytesRead != OF_FAILED && bytesRead == (int)sizeof(dhcpResponse)) {
		SetIPAddress(ntohl(dhcpResponse.ip_address));
		return B_OK;
	}

	// try to read manual client IP from boot path
	if (parameters != NULL) {
		char *comma = strrchr(parameters, ',');
		if (comma != NULL && comma != strchr(parameters, ',')) {
			SetIPAddress(ip_parse_address(comma + 1));
			return B_OK;
		}
	}

	// try to read default-client-ip setting
	char defaultClientIP[16];
	intptr_t package = of_finddevice("/options");
	bytesRead = of_getprop(package, "default-client-ip",
		defaultClientIP, sizeof(defaultClientIP) - 1);
	if (bytesRead != OF_FAILED && bytesRead > 1) {
		defaultClientIP[bytesRead] = '\0';
		ip_addr_t address = ip_parse_address(defaultClientIP);
		SetIPAddress(address);
		return B_OK;
	}

	return B_ERROR;
}


mac_addr_t
OFEthernetInterface::MACAddress() const
{
	return fMACAddress;
}


void *
OFEthernetInterface::AllocateSendReceiveBuffer(size_t size)
{
	void *dmaMemory = NULL;

	if (of_call_method(fHandle, "dma-alloc", 1, 1, size, &dmaMemory)
			!= OF_FAILED) {
		return dmaMemory;
	}

	// The dma-alloc method could be on the parent node (PCI bus, for example),
	// rather than the device itself
	intptr_t parentPackage = of_parent(of_instance_to_package(fHandle));

	// FIXME surely there's a way to create an instance without going through
	// the path?
	char path[256];
	of_package_to_path(parentPackage, path, sizeof(path));
	intptr_t parentInstance = of_open(path);

	if (of_call_method(parentInstance, "dma-alloc", 1, 1, size, &dmaMemory)
			!= OF_FAILED) {
		of_close(parentInstance);
		return dmaMemory;
	}

	of_close(parentInstance);

	return NULL;
}


void
OFEthernetInterface::FreeSendReceiveBuffer(void *buffer)
{
	if (buffer)
		of_call_method(fHandle, "dma-free", 1, 0, buffer);
}


ssize_t
OFEthernetInterface::Send(const void *buffer, size_t size)
{
	TRACE(("OFEthernetInterface::Send(%p, %lu)\n", buffer, size));

	if (!buffer)
		return B_BAD_VALUE;

	hex_dump(buffer, size);

	int result = of_write(fHandle, buffer, size);
	return (result == OF_FAILED ? B_ERROR : result);
}


ssize_t
OFEthernetInterface::Receive(void *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;

	int result = of_read(fHandle, buffer, size);

	if (result != OF_FAILED && result >= 0) {
		TRACE(("OFEthernetInterface::Receive(%p, %lu): received %d bytes\n",
			buffer, size, result));
		hex_dump(buffer, result);
	}

	return (result == OF_FAILED ? B_ERROR : result);
}


// #pragma mark -


status_t
platform_net_stack_init()
{
	// Note: At the moment we only do networking at all, if the boot device
	// is a network device. If it isn't, we simply fail here. For serious
	// support we would want to iterate through the device tree and add all
	// network devices.

	// get boot path
	char bootPath[192];
	int length = of_getprop(gChosen, "bootpath", bootPath, sizeof(bootPath));
	if (length <= 1)
		return B_ERROR;

	// we chop off parameters; otherwise opening the network device might have
	// side effects
	char *lastComponent = strrchr(bootPath, '/');
	char *parameters = strchr((lastComponent ? lastComponent : bootPath), ':');
	if (parameters)
		*parameters = '\0';

	// get device node
	intptr_t node = of_finddevice(bootPath);
	if (node == OF_FAILED)
		return B_ERROR;

	// get device type
	char type[16];
	if (of_getprop(node, "device_type", type, sizeof(type)) == OF_FAILED
		|| strcmp("network", type) != 0) {
		return B_ERROR;
	}

	// create an EthernetInterface object for the device
	OFEthernetInterface *interface = new(nothrow) OFEthernetInterface;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init(bootPath, parameters + 1);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	// add it to the net stack
	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	return B_OK;
}
