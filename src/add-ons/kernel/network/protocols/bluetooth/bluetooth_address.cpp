/*
 * Copyright 2006-2024, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */


#include "bluetooth_address.h"

#include <net_datalink.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <net_protocol.h>
#include <net_stack.h>

#include <NetUtilities.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/L2CAP/btL2CAP.h>
#include <bluetooth/SCO/btSCO.h>
#include <bluetooth/bluetooth.h>

#include <btModules.h>

extern net_stack_module_info* gStackModule;
extern status_t bluetooth_std_ops(int32 op, ...);


static inline const bdaddr_t*
get_bdaddr(const sockaddr* address)
{
	if (address == NULL)
		return NULL;
	if (address->sa_len == sizeof(sockaddr_l2cap))
		return &((const sockaddr_l2cap*)address)->l2cap_bdaddr;
	if (address->sa_len == sizeof(sockaddr_sco))
		return &((const sockaddr_sco*)address)->sco_bdaddr;
	return NULL;
}


static inline bdaddr_t*
get_bdaddr(sockaddr* address)
{
	if (address == NULL)
		return NULL;
	if (address->sa_len == sizeof(sockaddr_l2cap))
		return &((sockaddr_l2cap*)address)->l2cap_bdaddr;
	if (address->sa_len == sizeof(sockaddr_sco))
		return &((sockaddr_sco*)address)->sco_bdaddr;
	return NULL;
}


static bool
bdaddrs_equal(const bdaddr_t& ba1, const bdaddr_t& ba2)
{
	return memcmp(&ba1, &ba2, sizeof(bdaddr_t)) == 0;
}


static bool
bluetooth_is_empty_address(const sockaddr* address, bool checkPort)
{
	if (address == NULL || address->sa_len == 0 || address->sa_family == AF_UNSPEC)
		return true;

	const bdaddr_t* addr = get_bdaddr(address);
	if (addr == NULL)
		return true;

	const bdaddr_t null_addr = {{0, 0, 0, 0, 0, 0}};
	bool emptyAddr = bdaddrs_equal(*addr, null_addr);
	if (!emptyAddr)
		return false;

	if (checkPort && address->sa_len == sizeof(sockaddr_l2cap))
		return ((const sockaddr_l2cap*)address)->l2cap_psm == 0;

	return true;
}


static status_t
bluetooth_copy_address(const sockaddr* from, sockaddr** to, bool replaceWithZeros = false,
	const sockaddr* mask = NULL)
{
	if (replaceWithZeros) {
		*to = (sockaddr*)malloc(sizeof(sockaddr_l2cap));
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

		*to = (sockaddr*)malloc(from->sa_len);
		if (*to == NULL)
			return B_NO_MEMORY;

		memcpy(*to, from, from->sa_len);
	}

	return B_OK;
}


static bool
bluetooth_equal_addresses(const sockaddr* a, const sockaddr* b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return bluetooth_is_empty_address(a, false);
	if (a == NULL && b != NULL)
		return bluetooth_is_empty_address(b, false);

	const bdaddr_t* addr_a = get_bdaddr(a);
	const bdaddr_t* addr_b = get_bdaddr(b);
	if (addr_a == NULL || addr_b == NULL)
		return false;

	return bdaddrs_equal(*addr_a, *addr_b);
}


static bool
bluetooth_equal_ports(const sockaddr* a, const sockaddr* b)
{
	uint16 portA = (a != NULL && a->sa_len == sizeof(sockaddr_l2cap))
		? ((const sockaddr_l2cap*)a)->l2cap_psm
		: 0;
	uint16 portB = (b != NULL && b->sa_len == sizeof(sockaddr_l2cap))
		? ((const sockaddr_l2cap*)b)->l2cap_psm
		: 0;
	return portA == portB;
}


static bool
bluetooth_equal_addresses_and_ports(const sockaddr* a, const sockaddr* b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return bluetooth_is_empty_address(a, true);
	if (a == NULL && b != NULL)
		return bluetooth_is_empty_address(b, true);

	if (!bluetooth_equal_addresses(a, b))
		return false;

	return bluetooth_equal_ports(a, b);
}


static bool
bluetooth_equal_masked_addresses(const sockaddr* a, const sockaddr* b, const sockaddr* mask)
{
	// no masks
	return bluetooth_equal_addresses(a, b);
}


static bool
bluetooth_is_same_family(const sockaddr* address)
{
	if (address == NULL)
		return false;

	return address->sa_family == AF_BLUETOOTH;
}


static int32
bluetooth_first_mask_bit(const sockaddr* mask)
{
	return 0;
}


static bool
bluetooth_check_mask(const sockaddr* mask)
{
	return false;
}


static status_t
bluetooth_print_address(const sockaddr* address, char** _buffer, bool printPort)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;

	if (address == NULL) {
		*_buffer = strdup("<none>");
		return B_OK;
	}

	const bdaddr_t* addr = get_bdaddr(address);
	if (addr != NULL) {
		*_buffer = strdup("<invalid>");
		return B_OK;
	}

	char tmp[32];
	if (printPort && address->sa_len == sizeof(sockaddr_l2cap)) {
		snprintf(tmp, sizeof(tmp), "%02X:%02X:%02X:%02X:%02X:%02X|%u", addr->b[5], addr->b[4],
			addr->b[3], addr->b[2], addr->b[1], addr->b[0],
			((const sockaddr_l2cap*)address)->l2cap_psm);
	} else {
		snprintf(tmp, sizeof(tmp), "%02X:%02X:%02X:%02X:%02X:%02X", addr->b[5], addr->b[4],
			addr->b[3], addr->b[2], addr->b[1], addr->b[0]);
	}

	*_buffer = strdup(tmp);
	return *_buffer ? B_OK : B_NO_MEMORY;
}


static status_t
bluetooth_print_address_buffer(const sockaddr* address, char* buffer, size_t bufferSize,
	bool printPort)
{
	if (address == NULL) {
		strlcpy(buffer, "<none>", bufferSize);
		return B_OK;
	}

	const bdaddr_t* addr = get_bdaddr(address);
	if (addr == NULL) {
		strlcpy(buffer, "<invalid>", bufferSize);
		return B_OK;
	}

	if (printPort && address->sa_len == sizeof(sockaddr_l2cap)) {
		snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X|%u", addr->b[5], addr->b[4],
			addr->b[3], addr->b[2], addr->b[1], addr->b[0],
			((const sockaddr_l2cap*)address)->l2cap_psm);
	} else {
		snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X", addr->b[5], addr->b[4],
			addr->b[3], addr->b[2], addr->b[1], addr->b[0]);
	}

	return B_OK;
}


static uint16
bluetooth_get_port(const sockaddr* address)
{
	if (address != NULL && address->sa_len == sizeof(sockaddr_l2cap))
		return ((const sockaddr_l2cap*)address)->l2cap_psm;
	return 0;
}


static status_t
bluetooth_set_port(sockaddr* address, uint16 port)
{
	if (address != NULL && address->sa_len == sizeof(sockaddr_l2cap)) {
		((sockaddr_l2cap*)address)->l2cap_psm = port;
		return B_OK;
	}
	return B_BAD_VALUE;
}


static status_t
bluetooth_set_to(sockaddr* address, const sockaddr* from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->sa_family != AF_BLUETOOTH)
		return B_MISMATCHED_VALUES;

	if (address->sa_len == sizeof(sockaddr_l2cap) && from->sa_len == sizeof(sockaddr_l2cap)) {
		memcpy(address, from, sizeof(sockaddr_l2cap));
		return B_OK;
	}
	if (address->sa_len == sizeof(sockaddr_sco) && from->sa_len == sizeof(sockaddr_sco)) {
		memcpy(address, from, sizeof(sockaddr_sco));
		return B_OK;
	}

	return B_BAD_VALUE;
}


static status_t
bluetooth_mask_address(const sockaddr* address, const sockaddr* mask, sockaddr* result)
{
	// no masks
	return bluetooth_set_to(result, address);
}


static status_t
bluetooth_set_to_empty_address(sockaddr* address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	bdaddr_t* addr = get_bdaddr(address);
	if (addr != NULL) {
		memset(addr, 0, sizeof(bdaddr_t));
		if (address->sa_len == sizeof(sockaddr_l2cap))
			((sockaddr_l2cap*)address)->l2cap_psm = 0;
		return B_OK;
	}
	return B_BAD_VALUE;
}


static status_t
bluetooth_set_to_defaults(sockaddr* _defaultMask, sockaddr* _defaultBroadcast,
	const sockaddr* _address, const sockaddr* _netmask)
{
	status_t error = B_OK;
	if (_defaultMask != NULL)
		error = bluetooth_set_to_empty_address(_defaultMask);
	if (error == B_OK && _defaultBroadcast != NULL)
		error = bluetooth_set_to_empty_address(_defaultBroadcast);

	return error;
}


static status_t
bluetooth_update_to(sockaddr* address, const sockaddr* from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	bdaddr_t* toAddr = get_bdaddr(address);
	const bdaddr_t* fromAddr = get_bdaddr(from);
	if (toAddr != NULL && fromAddr != NULL) {
		const bdaddr_t null_addr = {{0, 0, 0, 0, 0, 0}};
		if (bdaddrs_equal(*toAddr, null_addr))
			*toAddr = *fromAddr;

		if (address->sa_len == sizeof(sockaddr_l2cap) && from->sa_len == sizeof(sockaddr_l2cap)) {
			sockaddr_l2cap* l2cap_to = (sockaddr_l2cap*)address;
			const sockaddr_l2cap* l2cap_from = (const sockaddr_l2cap*)from;
			if (l2cap_to->l2cap_psm == 0)
				l2cap_to->l2cap_psm = l2cap_from->l2cap_psm;
		}
		return B_OK;
	}
	return B_BAD_VALUE;
}


static uint32
bluetooth_hash_address(const sockaddr* address, bool includePort)
{
	if (address == NULL || address->sa_len == 0)
		return 0;

	const bdaddr_t* addr = get_bdaddr(address);
	if (addr == NULL)
		return 0;

	uint32 hash = 0;
	for (size_t i = 0; i < sizeof(addr->b); i++)
		hash += addr->b[i] << (i * 2);

	if (includePort && address->sa_len == sizeof(sockaddr_l2cap))
		hash += ((const sockaddr_l2cap*)address)->l2cap_psm;

	return hash;
}


static uint32
bluetooth_hash_address_pair(const sockaddr* ourAddress, const sockaddr* peerAddress)
{
	return bluetooth_hash_address(ourAddress, true) * 17
		^ bluetooth_hash_address(peerAddress, true);
}


static status_t
bluetooth_checksum_address(Checksum* checksum, const sockaddr* address)
{
	if (checksum == NULL || address == NULL)
		return B_BAD_VALUE;

	const bdaddr_t* addr = get_bdaddr(address);
	if (addr == NULL)
		return B_BAD_VALUE;

	for (uint i = 0; i < sizeof(bdaddr_t); i++)
		(*checksum) << addr->b[i];

	return B_OK;
}


net_address_module_info gBluetoothAddressModule = {
	{
		NET_BLUETOOTH_CORE_NAME,
		0,
		bluetooth_std_ops
	},
	0,

	bluetooth_copy_address,
	bluetooth_mask_address,
	bluetooth_equal_addresses,
	bluetooth_equal_ports,
	bluetooth_equal_addresses_and_ports,
	bluetooth_equal_masked_addresses,
	bluetooth_is_empty_address,
	bluetooth_is_same_family,
	bluetooth_first_mask_bit,
	bluetooth_check_mask,
	bluetooth_print_address,
	bluetooth_print_address_buffer,
	bluetooth_get_port,
	bluetooth_set_port,
	bluetooth_set_to,
	bluetooth_set_to_empty_address,
	bluetooth_set_to_defaults,
	bluetooth_update_to,
	bluetooth_hash_address,
	bluetooth_hash_address_pair,
	bluetooth_checksum_address,
	NULL	// get_loopback_address
};
