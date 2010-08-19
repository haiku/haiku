/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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


#define L2CAP_CHECKSUM(address) \
	(address.b[0] + address.b[1] + address.b[2] + address.b[3] \
		+ address.b[4] + address.b[5])


/*!	Routing utility function: copies address \a from into a new address
	that is put into \a to.
	If \a replaceWithZeros is set \a from will be replaced by an empty
	address.
	If a \a mask is given it is applied to \a from (such that \a to is the
	result of \a from & \a mask).
	\return B_OK if the address could be copied
	\return B_NO_MEMORY if the new address could not be allocated
	\return B_MISMATCHED_VALUES if \a address does not match family AF_BLUETOOTH
*/
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


/*!	Routing utility function: applies \a mask to given \a address and puts
	the resulting address into \a result.
	\return B_OK if the mask has been applied
	\return B_BAD_VALUE if \a address or \a mask is NULL
*/
static status_t
l2cap_mask_address(const sockaddr *address, const sockaddr *mask,
	sockaddr *result)
{
	if (address == NULL || result == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


/*!	Checks if the given \a address is the empty address. By default, the port
	is checked, too, but you can avoid that by passing \a checkPort = false.
	\return true if \a address is NULL, uninitialized or the empty address,
		false if not
*/
static bool
l2cap_is_empty_address(const sockaddr *address, bool checkPort)
{
	if (address == NULL || address->sa_len == 0
		|| address->sa_family == AF_UNSPEC)
		return true;

	return ((bdaddrUtils::Compare(
		((const sockaddr_l2cap *)address)->l2cap_bdaddr, BDADDR_NULL)==0)
		&& (!checkPort || ((sockaddr_l2cap *)address)->l2cap_psm == 0));
}


/*!	Checks if the given \a address is L2CAP address.
	\return false if \a address is NULL, or with family different from
		AF_BLUETOOTH true if it has AF_BLUETOOTH address family
*/
static bool
l2cap_is_same_family(const sockaddr *address)
{
	if (address == NULL)
		return false;

	return address->sa_family == AF_BLUETOOTH;
}


/*!	Compares the IP-addresses of the two given address structures \a a and \a b.
	\return true if IP-addresses of \a a and \a b are equal, false if not
*/
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


/*!	Compares the ports of the two given address structures \a a and \a b.
	\return true if ports of \a a and \a b are equal, false if not
*/
static bool
l2cap_equal_ports(const sockaddr *a, const sockaddr *b)
{
	uint16 portA = a ? ((sockaddr_l2cap *)a)->l2cap_psm : 0;
	uint16 portB = b ? ((sockaddr_l2cap *)b)->l2cap_psm : 0;
	return portA == portB;
}


/*!	Compares the IP-addresses and ports of the two given address structures
	\a a and \a b.
	\return true if IP-addresses and ports of \a a and \a b are equal, false if
		not.
*/
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


/*!	Applies the given \a mask two \a a and \a b and then checks whether
	the masked addresses match.
	\return true if \a a matches \a b after masking both, false if not
*/
static bool
l2cap_equal_masked_addresses(const sockaddr *a, const sockaddr *b,
	const sockaddr *mask)
{
	if (a == NULL && b == NULL)
		return true;

    return false;
}


/*!	Routing utility function: determines the least significant bit that is set
	in the given \a mask.
	\return the number of the first bit that is set (0-32, where 32 means
		that there's no bit set in the mask).
*/
static int32
l2cap_first_mask_bit(const sockaddr *_mask)
{
	if (_mask == NULL)
		return 0;

	return 0;
}


/*!	Routing utility function: checks the given \a mask for correctness (which
	means that (starting with LSB) consists zero or more unset bits, followed
	by bits that are all set).
	\return true if \a mask is ok, false if not
*/
static bool
l2cap_check_mask(const sockaddr *_mask)
{

	return true;
}


/*!	Creates a buffer for the given \a address and prints the address into
	it (hexadecimal representation in host byte order or '<none>').
	If \a printPort is set, the port is printed, too.
	\return B_OK if the address could be printed, \a buffer will point to
		the resulting string
	\return B_BAD_VALUE if no buffer has been given
	\return B_NO_MEMORY if the buffer could not be allocated
*/
static status_t
l2cap_print_address_buffer(const sockaddr *_address, char *buffer,
	size_t bufferSize, bool printPort)
{
	const sockaddr_l2cap *address = (const sockaddr_l2cap *)_address;

	if (buffer == NULL)
		return B_BAD_VALUE;

	if (address == NULL)
		strlcpy(buffer, "<none>", bufferSize);
	else {
		bdaddr_t addr = address->l2cap_bdaddr;
		if (printPort) {
			snprintf(buffer, bufferSize,
				"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X|%u", addr.b[0],
				addr.b[1],addr.b[2],addr.b[3],addr.b[4],addr.b[5],
				address->l2cap_psm);
		}
		else {
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


/*!	Determines the port of the given \a address.
	\return uint16 representing the port-nr
*/
static uint16
l2cap_get_port(const sockaddr *address)
{
	if (address == NULL)
		return 0;

	return ((sockaddr_l2cap *)address)->l2cap_psm;
}


/*!	Sets the port of the given \a address to \a port.
	\return B_OK if the port has been set
	\return B_BAD_VALUE if \a address is NULL
*/
static status_t
l2cap_set_port(sockaddr *address, uint16 port)
{
	if (address == NULL)
		return B_BAD_VALUE;

	((sockaddr_l2cap *)address)->l2cap_psm = port;
	return B_OK;
}


/*!	Sets \a address to \a from.
	\return B_OK if \a from has been copied into \a address
	\return B_BAD_VALUE if either \a address or \a from is NULL
	\return B_MISMATCHED_VALUES if from is not of family AF_BLUETOOTH
*/
static status_t
l2cap_set_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->sa_family != AF_BLUETOOTH)
		return B_MISMATCHED_VALUES;

	memcpy(address, from, sizeof(sockaddr_l2cap));
	address->sa_len = sizeof(sockaddr_l2cap);
	return B_OK;
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

	if (address->l2cap_psm == 0)
		address->l2cap_psm = from->l2cap_psm;

	if (bdaddrUtils::Compare(address->l2cap_bdaddr, BDADDR_BROADCAST))
		address->l2cap_bdaddr = from->l2cap_bdaddr;

	return B_OK;
}


/*!	Sets \a address to the empty address.
	\return B_OK if \a address has been set
	\return B_BAD_VALUE if \a address is NULL
*/
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
	// TODO: not implemented
	return B_ERROR;
}


/*!	Computes a hash value of the given \a address.
	\return uint32 representing the hash value
*/
static uint32
l2cap_hash_address(const struct sockaddr* _address, bool includePort)
{
	const sockaddr_l2cap* address = (const sockaddr_l2cap*)_address;
	if (address == NULL || address->l2cap_len == 0)
		return 0;

	return address->l2cap_psm ^ L2CAP_CHECKSUM(address->l2cap_bdaddr);
}


/*!	Computes a hash-value of the given addresses \a ourAddress
	and \a peerAddress.
	\return uint32 representing the hash-value
*/
static uint32
l2cap_hash_address_pair(const sockaddr *ourAddress, const sockaddr *peerAddress)
{
	const sockaddr_l2cap *our = (const sockaddr_l2cap *)ourAddress;
	const sockaddr_l2cap *peer = (const sockaddr_l2cap *)peerAddress;

	return ((our ? our->l2cap_psm : 0) | ((peer ? peer->l2cap_psm : 0) << 16))
		^ (our ? L2CAP_CHECKSUM(our->l2cap_bdaddr) : 0)
		^ (peer ? L2CAP_CHECKSUM(peer->l2cap_bdaddr) : 0);
}


/*!	Adds the given \a address to the IP-checksum \a checksum.
	\return B_OK if \a address has been added to the checksum
	\return B_BAD_VALUE if either \a address or \a checksum is NULL
*/
static status_t
l2cap_checksum_address(struct Checksum *checksum, const sockaddr *address)
{
	if (checksum == NULL || address == NULL)
		return B_BAD_VALUE;

	for (uint i = 0; i < sizeof(bdaddr_t); i++)
		(*checksum) << ((sockaddr_l2cap*)address)->l2cap_bdaddr.b[i];

	return B_OK;
}


net_address_module_info gL2cap4AddressModule = {
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
