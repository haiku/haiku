/*
 * Copyright 2006-2024, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */


#include <net_datalink.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <NetUtilities.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/L2CAP/btL2CAP.h>


static status_t
l2cap_copy_address(const sockaddr *from, sockaddr **to,
	bool replaceWithZeros = false, const sockaddr *mask = NULL)
{
	if (replaceWithZeros) {
		*to = (sockaddr *)malloc(sizeof(sockaddr_in));
		if (*to == NULL)
			return B_NO_MEMORY;

		memset(*to, 0, sizeof(sockaddr_l2cap));
		(*to)->sa_family = AF_BLUETOOTH;
		(*to)->sa_len = sizeof(sockaddr_l2cap);
	} else {
		if (from == NULL)
			return B_OK;
		if (from->sa_family != AF_BLUETOOTH)
			return B_MISMATCHED_VALUES;

		*to = (sockaddr *)malloc(sizeof(sockaddr_in));
		if (*to == NULL)
			return B_NO_MEMORY;

		memcpy(*to, from, sizeof(sockaddr_l2cap));

	}
	return B_OK;
}


static bool
l2cap_is_empty_address(const sockaddr *address, bool checkPort)
{
	if (address == NULL || address->sa_len == 0
		|| address->sa_family == AF_UNSPEC)
		return true;

	return ((bdaddrUtils::Compare(
		((const sockaddr_l2cap *)address)->l2cap_bdaddr, BDADDR_NULL))
		&& (!checkPort || ((sockaddr_l2cap *)address)->l2cap_psm == 0));
}


static bool
l2cap_is_same_family(const sockaddr *address)
{
	if (address == NULL)
		return false;

	return address->sa_family == AF_BLUETOOTH;
}


static bool
l2cap_equal_addresses(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return l2cap_is_empty_address(a, false);
	if (a == NULL && b != NULL)
		return l2cap_is_empty_address(b, false);

	return bdaddrUtils::Compare(((const sockaddr_l2cap*)a)->l2cap_bdaddr,
		((sockaddr_l2cap*)b)->l2cap_bdaddr);
}


static bool
l2cap_equal_ports(const sockaddr *a, const sockaddr *b)
{
	uint16 portA = a ? ((sockaddr_l2cap *)a)->l2cap_psm : 0;
	uint16 portB = b ? ((sockaddr_l2cap *)b)->l2cap_psm : 0;
	return portA == portB;
}


static bool
l2cap_equal_addresses_and_ports(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return l2cap_is_empty_address(a, true);
	if (a == NULL && b != NULL)
		return l2cap_is_empty_address(b, true);

	return (bdaddrUtils::Compare(((const sockaddr_l2cap *)a)->l2cap_bdaddr,
		((const sockaddr_l2cap *)b)->l2cap_bdaddr))
		&& ((sockaddr_l2cap *)a)->l2cap_psm == ((sockaddr_l2cap *)b)->l2cap_psm;
}


static bool
l2cap_equal_masked_addresses(const sockaddr *a, const sockaddr *b,
	const sockaddr *mask)
{
	// no masks
	return l2cap_equal_addresses(a, b);
}


static int32
l2cap_first_mask_bit(const sockaddr *_mask)
{
	return 0;
}


static bool
l2cap_check_mask(const sockaddr *_mask)
{
	return false;
}


static status_t
l2cap_print_address_buffer(const sockaddr *_address, char *buffer,
	size_t bufferSize, bool printPort)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	const sockaddr_l2cap *address = (const sockaddr_l2cap *)_address;
	if (address == NULL) {
		strlcpy(buffer, "<none>", bufferSize);
	} else {
		bdaddr_t addr = address->l2cap_bdaddr;
		if (printPort) {
			snprintf(buffer, bufferSize,
				"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X|%u", addr.b[0],
				addr.b[1],addr.b[2],addr.b[3],addr.b[4],addr.b[5],
				address->l2cap_psm);
		} else {
			snprintf(buffer, bufferSize,
				"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",addr.b[0],
				addr.b[1],addr.b[2],addr.b[3],addr.b[4],addr.b[5]);
		}
	}

	return B_OK;
}


static status_t
l2cap_print_address(const sockaddr *_address, char **_buffer, bool printPort)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;

	char tmp[32];
	l2cap_print_address_buffer(_address, tmp, sizeof(tmp), printPort);

	*_buffer = strdup(tmp);
	if (*_buffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static uint16
l2cap_get_port(const sockaddr *address)
{
	if (address == NULL)
		return 0;

	return ((sockaddr_l2cap *)address)->l2cap_psm;
}


static status_t
l2cap_set_port(sockaddr *address, uint16 port)
{
	if (address == NULL)
		return B_BAD_VALUE;

	((sockaddr_l2cap *)address)->l2cap_psm = port;
	return B_OK;
}


static status_t
l2cap_set_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->sa_family != AF_BLUETOOTH)
		return B_MISMATCHED_VALUES;

	memcpy(address, from, sizeof(sockaddr_l2cap));
	return B_OK;
}


static status_t
l2cap_mask_address(const sockaddr *address, const sockaddr *mask,
	sockaddr *result)
{
	// no masks
	return l2cap_set_to(result, address);
}


static status_t
l2cap_update_to(sockaddr *_address, const sockaddr *_from)
{
	sockaddr_l2cap *address = (sockaddr_l2cap *)_address;
	const sockaddr_l2cap *from = (const sockaddr_l2cap *)_from;

	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->l2cap_family != AF_BLUETOOTH)
		return B_BAD_VALUE;

	address->l2cap_family = AF_BLUETOOTH;
	address->l2cap_len = sizeof(sockaddr_l2cap);

	if (bdaddrUtils::Compare(address->l2cap_bdaddr, BDADDR_NULL))
		address->l2cap_bdaddr = from->l2cap_bdaddr;

	if (address->l2cap_psm == 0)
		address->l2cap_psm = from->l2cap_psm;

	return B_OK;
}


static status_t
l2cap_set_to_empty_address(sockaddr *address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	memset(address, 0, sizeof(sockaddr_l2cap));
	address->sa_len = sizeof(sockaddr_l2cap);
	address->sa_family = AF_BLUETOOTH;
	return B_OK;
}


static status_t
l2cap_set_to_defaults(sockaddr *_defaultMask, sockaddr *_defaultBroadcast,
	const sockaddr *_address, const sockaddr *_mask)
{
	if (_address == NULL)
		return B_BAD_VALUE;

	status_t error = B_OK;
	if (_defaultMask != NULL)
		error = l2cap_set_to_empty_address(_defaultMask);
	if (error == B_OK && _defaultBroadcast != NULL)
		error = l2cap_set_to_empty_address(_defaultBroadcast);

	return error;
}


static uint32
l2cap_hash_address(const struct sockaddr* _address, bool includePort)
{
	const sockaddr_l2cap* address = (const sockaddr_l2cap*)_address;
	if (address == NULL || address->l2cap_len == 0)
		return 0;

	uint32 hash = 0;
	for (size_t i = 0; i < sizeof(address->l2cap_bdaddr.b); i++)
		hash += address->l2cap_bdaddr.b[i] << (i * 2);

	if (includePort)
		hash += address->l2cap_psm;
	return hash;
}


static uint32
l2cap_hash_address_pair(const sockaddr *ourAddress, const sockaddr *peerAddress)
{
	return l2cap_hash_address(ourAddress, true) * 17
		+ l2cap_hash_address(peerAddress, true);
}


static status_t
l2cap_checksum_address(struct Checksum *checksum, const sockaddr *address)
{
	if (checksum == NULL || address == NULL)
		return B_BAD_VALUE;

	for (uint i = 0; i < sizeof(bdaddr_t); i++)
		(*checksum) << ((sockaddr_l2cap*)address)->l2cap_bdaddr.b[i];

	return B_OK;
}


net_address_module_info gL2capAddressModule = {
	{
		NULL,
		0,
		NULL
	},
	NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS,
	l2cap_copy_address,
	l2cap_mask_address,
	l2cap_equal_addresses,
	l2cap_equal_ports,
	l2cap_equal_addresses_and_ports,
	l2cap_equal_masked_addresses,
	l2cap_is_empty_address,
	l2cap_is_same_family,
	l2cap_first_mask_bit,
	l2cap_check_mask,
	l2cap_print_address,
	l2cap_print_address_buffer,
	l2cap_get_port,
	l2cap_set_port,
	l2cap_set_to,
	l2cap_set_to_empty_address,
	l2cap_set_to_defaults,
	l2cap_update_to,
	l2cap_hash_address,
	l2cap_hash_address_pair,
	l2cap_checksum_address,
	NULL	// get_loopback_address
};
