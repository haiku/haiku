/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Faerber <andreas.faerber@web.de>
 * Copyright 2025, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <util/convertutf.h>

#include <boot/platform.h>
#include <boot/net/Ethernet.h>
#include <boot/net/IP.h>
#include <boot/net/NetStack.h>

#include "efi_platform.h"
#include <efi/protocol/simple-network.h>
#include <efi/protocol/loaded-image.h>


static efi_guid sNetworkProtocolGUID = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
static efi_guid sLoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;

#if 0
static efi_guid sDevicePathProtocolGUID = EFI_DEVICE_PATH_PROTOCOL_GUID;
static efi_guid sLoadedImageDevicePathProtocolGUID = EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID;
#endif


//#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf("efi/network: " x)
#	define CALLED(x...) dprintf("efi/network: CALLED %s\n", __func__)
#else
#	define TRACE(x...) ;
#	define CALLED(x...) ;
#endif

#define TRACE_ALWAYS(x...) dprintf("efi/network: " x)


class EFIEthernetInterface : public EthernetInterface {
public:
						EFIEthernetInterface();
	virtual				~EFIEthernetInterface();

			status_t	Init();

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
			status_t	FindMACAddress();

private:
			efi_simple_network_protocol* fNetwork;
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
					dprintf("\n");
				dprintf("%03x: ", i);
			} else
				dprintf(" ");
		}

		dprintf("%02x", data[i]);
	}
	dprintf("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


// #pragma mark -


EFIEthernetInterface::EFIEthernetInterface()
	:
	EthernetInterface(),
	fNetwork(NULL),
	fMACAddress(kNoMACAddress)
{
}


EFIEthernetInterface::~EFIEthernetInterface()
{
	if (fNetwork) {
		fNetwork->Shutdown(fNetwork);
		fNetwork->Stop(fNetwork);
	}
}


status_t
EFIEthernetInterface::FindMACAddress()
{
	if (fNetwork == NULL)
		return B_ERROR;

	memcpy(&fMACAddress, &fNetwork->Mode->CurrentAddress,
		sizeof(fMACAddress));

	if (fMACAddress == kNoMACAddress) {
		memcpy(&fMACAddress, &fNetwork->Mode->PermanentAddress,
			sizeof(fMACAddress));
	}

	if (fMACAddress == kNoMACAddress)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


status_t
EFIEthernetInterface::Init()
{
	CALLED();

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&sNetworkProtocolGUID, NULL, (void**)&fNetwork);

	if (status != EFI_SUCCESS || fNetwork == NULL) {
		fNetwork = NULL;
		TRACE_ALWAYS("EFI reports network unavaiable\n");
		return B_ERROR;
	}

	TRACE_ALWAYS("EFI reports network avaiable\n");

	// Attempt to locate IP address from command line arguments passed by
	// the bootloader (for example, U-Boot bootargs variable)
	// Recognize a subset of the ip= parameter as defined for Linux:
	// https://www.kernel.org/doc/Documentation/filesystems/nfs/nfsroot.txt
	efi_loaded_image_protocol* loadedImageProtocol;
	status = kSystemTable->BootServices->LocateProtocol(
		&sLoadedImageProtocolGUID, NULL, (void**)&loadedImageProtocol);

	if (status == EFI_SUCCESS) {
		char buffer[256];
		utf16le_to_utf8((uint16*)loadedImageProtocol->LoadOptions,
			loadedImageProtocol->LoadOptionsSize / 2, buffer, sizeof(buffer));
		TRACE("Load options: %s\n", buffer);

		char* base = buffer;
		char* ptr = base;
		while (*ptr != '\0') {
			if (*ptr == '=') {
				TRACE("Key at %ld: %s\n", ptr - base, base);
				// Found a key-value separator, see if it's a known option
				if (ptr - base != 2)
					continue;

				if (strncmp(base, "ip", 2) == 0) {
					TRACE("Found ip configuration in command line\n");
					break;
				}
			}

			// At any space, reset the base pointer to the start of the next
			// key
			if (*ptr == ' ') {
				base = ptr + 1;
			}
			ptr++;
		}

		if (*ptr != '=') {
			TRACE("No keys found in load options\n");
			return B_ERROR;
		}

		// Point after the = sign
		ptr++;

		// Try to parse next part of the string as an IP address
		// TODO also recognize autoconfig parameters (such as 'ip=dhcp')
		TRACE("Found IP address from load options: %s\n", ptr);
		ip_addr_t address = ip_parse_address(ptr);
		SetIPAddress(address);
	}

#if 0
	// TODO u-boot does not provide the IP address or the TFTP server URL in
	// "loaded image" and "device path" protocols. That would allow to detect
	// network booting without having to specify the ip in bootargs. Enable
	// this if the information is available when network booting with other
	// EFI implementations or if U-Boot grows support for it.
	bool bootFromNetwork = false;

	// Try the basic "device path" protocol
	efi_device_path_protocol* devicePathProtocol;
	status = kSystemTable->BootServices->LocateProtocol(
		&sDevicePathProtocolGUID, NULL, (void**)&devicePathProtocol);

	if (status == EFI_SUCCESS) {
		for (;;) {
			TRACE("Now scanning: Type %d SubType %d\n", devicePathProtocol->Type, devicePathProtocol->SubType);
			if (devicePathProtocol->Type == DEVICE_PATH_END
				&& devicePathProtocol->SubType == DEVICE_PATH_ENTIRE_END) {
				break;
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MESSAGING
				&& devicePathProtocol->SubType == DEVICE_PATH_MESSAGING_MAC) {
				bootFromNetwork = true;
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MEDIA
				&& devicePathProtocol->SubType == MEDIA_FILEPATH_DP) {
				TRACE("Found device path, length %d: %s\n", devicePathProtocol->Length, ((char*)devicePathProtocol) + sizeof(efi_device_path_protocol));
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MESSAGING
				&& devicePathProtocol->SubType == DEVICE_PATH_MESSAGING_IPV4) {
				bootFromNetwork = true;
				ip_addr_t* address = (ip_addr_t*)(devicePathProtocol + 1);
				SetIPAddress(*address);
				return B_OK;
			}

			devicePathProtocol = (efi_device_path_protocol*)(((uint8_t*)devicePathProtocol) + devicePathProtocol->Length);
		}
	} else {
		TRACE("No device path protocol\n");
	}

	TRACE_ALWAYS("IP address not found in device path protocol, try in loaded image\n");

	// Try the "loaded image device path protocol" which provides more specific
	// information
	status = kSystemTable->BootServices->LocateProtocol(
		&sLoadedImageDevicePathProtocolGUID, NULL, (void**)&devicePathProtocol);

	if (status == EFI_SUCCESS) {
		for (;;) {
			TRACE("Now scanning: Type %d SubType %d\n", devicePathProtocol->Type, devicePathProtocol->SubType);
			if (devicePathProtocol->Type == DEVICE_PATH_END
				&& devicePathProtocol->SubType == DEVICE_PATH_ENTIRE_END) {
				break;
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MESSAGING
				&& devicePathProtocol->SubType == DEVICE_PATH_MESSAGING_MAC) {
				bootFromNetwork = true;
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MEDIA
				&& devicePathProtocol->SubType == MEDIA_FILEPATH_DP) {
				TRACE("Found device path, length %d: %s\n", devicePathProtocol->Length, ((char*)devicePathProtocol) + sizeof(efi_device_path_protocol));
			}

			if (devicePathProtocol->Type == DEVICE_PATH_MESSAGING
				&& devicePathProtocol->SubType == DEVICE_PATH_MESSAGING_IPV4) {
				bootFromNetwork = true;
				ip_addr_t* address = (ip_addr_t*)(devicePathProtocol + 1);
				SetIPAddress(*address);
				return B_OK;
			}

			devicePathProtocol = (efi_device_path_protocol*)(((uint8_t*)devicePathProtocol) + devicePathProtocol->Length);
		}
	} else {
		TRACE("No loaded image device path protocol\n");
	}

	if (!bootFromNetwork)
		return B_ENTRY_NOT_FOUND;
#endif

	status = fNetwork->Start(fNetwork);

	if (status != EFI_SUCCESS) {
		TRACE_ALWAYS("error starting network\n");
		return B_ERROR;
	}

	status = fNetwork->Initialize(fNetwork, 0, 0);

	if (status != EFI_SUCCESS) {
		TRACE_ALWAYS("error initializing network\n");
		return B_ERROR;
	}

	if (FindMACAddress() != B_OK) {
		TRACE_ALWAYS("failed to get MAC address\n");
		return B_ERROR;
	}

	return B_OK;
}


mac_addr_t
EFIEthernetInterface::MACAddress() const
{
	return fMACAddress;
}


void *
EFIEthernetInterface::AllocateSendReceiveBuffer(size_t size)
{
	return malloc(size);
}


void
EFIEthernetInterface::FreeSendReceiveBuffer(void *buffer)
{
	free(buffer);
}


ssize_t
EFIEthernetInterface::Send(const void *buffer, size_t size)
{
	TRACE("EFIEthernetInterface::Send(%p, %lu)\n", buffer, size);

	if (!buffer)
		return B_BAD_VALUE;

	hex_dump(buffer, size);

	efi_status status = fNetwork->Transmit(fNetwork, 0, size,
		const_cast<void*>(buffer), NULL, NULL, NULL);

	if (status != EFI_SUCCESS) {
		TRACE("Send: Result is %lx\n", status);
		return B_ERROR;
	}
	return size;
}


ssize_t
EFIEthernetInterface::Receive(void *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;

	efi_status status = fNetwork->Receive(fNetwork, NULL, &size, buffer,
		NULL, NULL, NULL);

	if (status == EFI_NOT_READY)
		return B_WOULD_BLOCK;

	if (status != EFI_SUCCESS) {
		TRACE("EFIEthernetInterface::Receive(%p, %lu): %ld\n", buffer, size, status);
		return B_ERROR;
	}

	hex_dump(buffer, size);

	return size;
}


// #pragma mark -


status_t
platform_net_stack_init()
{
	// create an EthernetInterface object for the device
	EFIEthernetInterface *interface = new(nothrow) EFIEthernetInterface;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init();
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
