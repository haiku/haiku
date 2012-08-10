/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UnixAddress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/khash.h>

#include <net_datalink.h>
#include <NetUtilities.h>

#include "unix.h"


static const sockaddr_un kEmptyAddress = {
	4,				// sun_len
	AF_UNIX,		// sun_family
	{ '\0', '\0' }	// sun_path
};


char*
UnixAddress::ToString(char *buffer, size_t bufferSize) const
{
	if (!IsValid())
		strlcpy(buffer, "<empty>", bufferSize);
	else if (IsInternalAddress())
		snprintf(buffer, bufferSize, "<%05" B_PRIx32 ">", fInternalID);
	else {
		snprintf(buffer, bufferSize, "<%" B_PRIdDEV ", %" B_PRIdINO ">",
			fVolumeID, fNodeID);
	}

	return buffer;
}


// #pragma mark -


static status_t
unix_copy_address(const sockaddr *from, sockaddr **to, bool replaceWithZeros,
	const sockaddr *mask)
{
	if (replaceWithZeros) {
		sockaddr_un* newAddress = (sockaddr_un*)malloc(kEmptyAddress.sun_len);
		if (newAddress == NULL)
			return B_NO_MEMORY;

		memcpy(newAddress, &kEmptyAddress, kEmptyAddress.sun_len);

		*to = (sockaddr*)newAddress;
		return B_OK;
	} else {
		if (from->sa_family != AF_UNIX)
			return B_MISMATCHED_VALUES;

		*to = (sockaddr*)malloc(from->sa_len);
		if (*to == NULL)
			return B_NO_MEMORY;

		memcpy(*to, from, from->sa_len);

		return B_OK;
	}
}


static bool
unix_equal_addresses(const sockaddr *a, const sockaddr *b)
{
	// NOTE: We compare syntactically only. The real check would involve
	// looking up the node, if for FS addresses.
	if (a->sa_len != b->sa_len)
		return false;

	return memcmp(a, b, a->sa_len) == 0;
}


static bool
unix_equal_ports(const sockaddr *a, const sockaddr *b)
{
	// no ports
	return true;
}


static bool
unix_equal_addresses_and_ports(const sockaddr *a, const sockaddr *b)
{
	return unix_equal_addresses(a, b);
}


static bool
unix_equal_masked_addresses(const sockaddr *a, const sockaddr *b,
	const sockaddr *mask)
{
	// no masks
	return unix_equal_addresses(a, b);
}


static bool
unix_is_empty_address(const sockaddr *address, bool checkPort)
{
	return address == NULL || address->sa_len == 0
		|| address->sa_family == AF_UNSPEC
		|| (address->sa_len >= kEmptyAddress.sun_len
			&& memcmp(address, &kEmptyAddress, kEmptyAddress.sun_len) == 0);
}


static bool
unix_is_same_family(const sockaddr *address)
{
	if (address == NULL)
		return false;

	return address->sa_family == AF_UNIX;
}


static int32
unix_first_mask_bit(const sockaddr *mask)
{
	return 0;
}


static bool
unix_check_mask(const sockaddr *address)
{
	return false;
}


static status_t
unix_print_address_buffer(const sockaddr *_address, char *buffer,
	size_t bufferSize, bool printPort)
{
	if (!buffer)
		return B_BAD_VALUE;

	sockaddr_un* address = (sockaddr_un*)_address;

	if (address == NULL)
		strlcpy(buffer, "<none>", bufferSize);
	else if (address->sun_path[0] != '\0')
		strlcpy(buffer, address->sun_path, bufferSize);
	else if (address->sun_path[1] != '\0')
		snprintf(buffer, bufferSize, "<%.5s>", address->sun_path + 1);
	else
		strlcpy(buffer, "<empty>", bufferSize);

	return B_OK;
}


static status_t
unix_print_address(const sockaddr *address, char **_buffer, bool printPort)
{
	char buffer[128];
	status_t error = unix_print_address_buffer(address, buffer, sizeof(buffer),
		printPort);
	if (error != B_OK)
		return error;

	*_buffer = strdup(buffer);
	return *_buffer != NULL ? B_OK : B_NO_MEMORY;
}


static uint16
unix_get_port(const sockaddr *address)
{
	return 0;
}


static status_t
unix_set_port(sockaddr *address, uint16 port)
{
	return B_BAD_VALUE;
}


static status_t
unix_set_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->sa_family != AF_UNIX)
		return B_MISMATCHED_VALUES;

	memcpy(address, from, from->sa_len);
	return B_OK;
}


static status_t
unix_set_to_empty_address(sockaddr *address)
{
	return unix_set_to(address, (sockaddr*)&kEmptyAddress);
}


static status_t
unix_mask_address(const sockaddr *address, const sockaddr *mask,
	sockaddr *result)
{
	// no masks
	return unix_set_to(result, address);
}


static status_t
unix_set_to_defaults(sockaddr *defaultMask, sockaddr *defaultBroadcast,
	const sockaddr *address, const sockaddr *netmask)
{
	if (address == NULL)
		return B_BAD_VALUE;

	status_t error = B_OK;
	if (defaultMask != NULL)
		error = unix_set_to_empty_address(defaultMask);
	if (error == B_OK && defaultBroadcast != NULL)
		error = unix_set_to_empty_address(defaultBroadcast);

	return error;
}


static status_t
unix_update_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (unix_is_empty_address(from, false))
		return B_OK;

	return unix_set_to(address, from);
}


static uint32
unix_hash_address(const sockaddr* _address, bool includePort)
{
	sockaddr_un* address = (sockaddr_un*)_address;
	if (address == NULL)
		return 0;

	if (address->sun_path[0] == '\0') {
		char buffer[6];
		strlcpy(buffer, address->sun_path + 1, 6);
		return hash_hash_string(buffer);
	}

	return hash_hash_string(address->sun_path);
}


static uint32
unix_hash_address_pair(const sockaddr *ourAddress, const sockaddr *peerAddress)
{
	return unix_hash_address(ourAddress, false) * 17
		+ unix_hash_address(peerAddress, false);
}


static status_t
unix_checksum_address(struct Checksum *checksum, const sockaddr *_address)
{
	if (checksum == NULL || _address == NULL)
		return B_BAD_VALUE;

	sockaddr_un* address = (sockaddr_un*)_address;
	int len = (char*)address + address->sun_len - address->sun_path;
	for (int i = 0; i < len; i++)
		(*checksum) << (uint8)address->sun_path[i];

	return B_OK;
}


net_address_module_info gAddressModule = {
	{
		NULL,
		0,
		NULL
	},
	NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS,
	unix_copy_address,
	unix_mask_address,
	unix_equal_addresses,
	unix_equal_ports,
	unix_equal_addresses_and_ports,
	unix_equal_masked_addresses,
	unix_is_empty_address,
	unix_is_same_family,
	unix_first_mask_bit,
	unix_check_mask,
	unix_print_address,
	unix_print_address_buffer,
	unix_get_port,
	unix_set_port,
	unix_set_to,
	unix_set_to_empty_address,
	unix_set_to_defaults,
	unix_update_to,
	unix_hash_address,
	unix_hash_address_pair,
	unix_checksum_address,
	NULL	// get_loopback_address
};
