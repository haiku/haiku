/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Atis Elsts, the.kfx@gmail.com
 */


#include <net_datalink.h>

#include <NetUtilities.h>

#include <memory.h>
#include <netinet6/in6.h>
#include <stdio.h>
#include <stdlib.h>

#include "ipv6_address.h"
#include "ipv6_utils.h"
#include "jenkins.h"


const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;


static void
ipv6_mask_adress_inplace(sockaddr *address, const sockaddr *mask)
{
	in6_addr &i6addr = ((sockaddr_in6 *)address)->sin6_addr;
	const in6_addr &i6mask = ((const sockaddr_in6 *)mask)->sin6_addr;

	for (uint32 i = 0; i < sizeof(in6_addr); i++)
		i6addr.s6_addr[i] &= i6mask.s6_addr[i];
}


/*!	Routing utility function: copies address \a from into a new address
	that is put into \a to.
	If \a replaceWithZeros is set \a from will be replaced by an empty
	address.
	If a \a mask is given it is applied to \a from (such that \a to is the
	result of \a from & \a mask).
	\return B_OK if the address could be copied
	\return B_NO_MEMORY if the new address could not be allocated
	\return B_BAD_VALUE if any of \a from or \a mask refers to an uninitialized
			address
	\return B_MISMATCHED_VALUES if \a address does not match family AF_INET
*/
static status_t
ipv6_copy_address(const sockaddr *from, sockaddr **to,
	bool replaceWithZeros = false, const sockaddr *mask = NULL)
{
	if (replaceWithZeros) {
		*to = (sockaddr *)malloc(sizeof(sockaddr_in6));
		if (*to == NULL)
			return B_NO_MEMORY;

		memset(*to, 0, sizeof(sockaddr_in6));
		(*to)->sa_family = AF_INET6;
		(*to)->sa_len = sizeof(sockaddr_in6);
	} else {
		if (from == NULL)
			return B_OK;
		if (from->sa_len == 0 || (mask != NULL && mask->sa_len == 0))
			return B_BAD_VALUE;
		if (from->sa_family != AF_INET6)
			return B_MISMATCHED_VALUES;

		*to = (sockaddr *)malloc(sizeof(sockaddr_in6));
		if (*to == NULL)
			return B_NO_MEMORY;

		memcpy(*to, from, sizeof(sockaddr_in6));

		if (mask != NULL)
			ipv6_mask_adress_inplace(*to, mask);
	}
	return B_OK;
}


/*!	Routing utility function: applies \a mask to given \a address and puts
	the resulting address into \a result.
	\return B_OK if the mask has been applied
	\return B_BAD_VALUE if \a address is NULL or if any of \a address or \a mask
			refers to an uninitialized address
*/
static status_t
ipv6_mask_address(const sockaddr *address, const sockaddr *mask,
	sockaddr *result)
{
	if (address == NULL || address->sa_len == 0 || result == NULL
			|| (mask != NULL && mask->sa_len == 0))
		return B_BAD_VALUE;

	memcpy(result, address, sizeof(sockaddr_in6));
	if (mask != NULL)
		ipv6_mask_adress_inplace(result, mask);

	return B_OK;
}


/*!	Checks if the given \a address is the empty address. By default, the port
	is checked, too, but you can avoid that by passing \a checkPort = false.
	\return true if \a address is NULL, uninitialized or the empty address,
		false if not
*/
static bool
ipv6_is_empty_address(const sockaddr *_address, bool checkPort)
{
	if (_address == NULL || _address->sa_len == 0
		|| _address->sa_family == AF_UNSPEC)
		return true;

	const sockaddr_in6 *address = (const sockaddr_in6 *)_address;
	if (checkPort && address->sin6_port != 0)
		return false;

	return IN6_IS_ADDR_UNSPECIFIED(&address->sin6_addr);
}


/*!	Checks if the given \a address is an Ipv6 address.
	\return false if \a address is NULL, or with family different from AF_INET
		true if it has AF_INET address family
*/
static bool
ipv6_is_same_family(const sockaddr *address)
{
	if (address == NULL)
		return false;

	return address->sa_family == AF_INET6;
}


/*!	Compares the IP-addresses of the two given address structures \a a and \a b.
	\return true if IP-addresses of \a a and \a b are equal, false if not
*/
static bool
ipv6_equal_addresses(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return ipv6_is_empty_address(a, false);
	if (a == NULL && b != NULL)
		return ipv6_is_empty_address(b, false);

	const sockaddr_in6 *i6a = (const sockaddr_in6 *)a;
	const sockaddr_in6 *i6b = (const sockaddr_in6 *)b;
	return !memcmp(&i6a->sin6_addr, &i6b->sin6_addr, sizeof(in6_addr));
}


/*!	Compares the ports of the two given address structures \a a and \a b.
	\return true if ports of \a a and \a b are equal, false if not
*/
static bool
ipv6_equal_ports(const sockaddr *a, const sockaddr *b)
{
	uint16 portA = a ? ((sockaddr_in6 *)a)->sin6_port : 0;
	uint16 portB = b ? ((sockaddr_in6 *)b)->sin6_port : 0;
	return portA == portB;
}


/*!	Compares the IP-addresses and ports of the two given address structures
	\a a and \a b.
	\return true if IP-addresses and ports of \a a and \a b are equal, false if
			not
*/
static bool
ipv6_equal_addresses_and_ports(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return ipv6_is_empty_address(a, true);
	if (a == NULL && b != NULL)
		return ipv6_is_empty_address(b, true);

	const sockaddr_in6 *i6a = (const sockaddr_in6 *)a;
	const sockaddr_in6 *i6b = (const sockaddr_in6 *)b;
	return i6a->sin6_port == i6b->sin6_port
		&& !memcmp(&i6a->sin6_addr, &i6b->sin6_addr, sizeof(in6_addr));
}


/*!	Applies the given \a mask two \a a and \a b and then checks whether
	the masked addresses match.
	\return true if \a a matches \a b after masking both, false if not
*/
static bool
ipv6_equal_masked_addresses(const sockaddr *a, const sockaddr *b,
	const sockaddr *mask)
{
	if (a == NULL && b == NULL)
		return true;

	const in6_addr *i6a;
	if (a == NULL)
		i6a = &in6addr_any;
	else
		i6a = &((const sockaddr_in6*)a)->sin6_addr;

	const in6_addr *i6b;
	if (b == NULL)
		i6b = &in6addr_any;
	else
		i6b = &((const sockaddr_in6*)b)->sin6_addr;

 	if (!mask)
		return !memcmp(i6a, i6b, sizeof(in6_addr));

	const uint8 *pmask = ((const sockaddr_in6 *)mask)->sin6_addr.s6_addr;
	for (uint8 i = 0; i < sizeof(in6_addr); ++i) {
		if (pmask[i] != 0xff) {
			return (i6a->s6_addr[i] & pmask[i])
				== (i6b->s6_addr[i] & pmask[i]);
		}

		if (i6a->s6_addr[i] != i6b->s6_addr[i])
			return false;
	}

	return true;
}


/*!	Routing utility function: determines the least significant bit that is set
	in the given \a mask.
	\return the number of the first bit that is set (0-32, where 32 means
		that there's no bit set in the mask).
*/
static int32
ipv6_first_mask_bit(const sockaddr *_mask)
{
	if (_mask == NULL)
		return 0;

	const uint8 *pmask = ((const sockaddr_in6 *)_mask)->sin6_addr.s6_addr;
	for (uint8 i = 0; i < sizeof(in6_addr); ++i) {
		if (pmask[i] == 0xff)
			continue;

		for (uint8 bit = 0; bit < 8; bit++) {
			if ((pmask[i] & (1 << (7 - bit))) == 0)
				return i * 8 + bit;
		}
	}

	return 128;
}


/*!	Routing utility function: checks the given \a mask for correctness (which
	means that (starting with LSB) consists zero or more unset bits, followed
	by bits that are all set).
	\return true if \a mask is ok, false if not
*/
static bool
ipv6_check_mask(const sockaddr *_mask)
{
	if (_mask == NULL)
		return true;

	bool zero = false;
	const uint8 *pmask = ((const sockaddr_in6 *)_mask)->sin6_addr.s6_addr;
	for (uint8 i = 0; i < sizeof(in6_addr); ++i) {
		if (pmask[i] == 0xff) {
			if (zero)
				return false;
		} else if (pmask[i] == 0) {
			zero = true;
		} else {
			for (int8 bit = 7; bit >= 0; bit--) {
				if (pmask[i] & (1 << bit)) {
					if (zero)
						return false;
				} else {
					zero = true;
				}
			}
		}
	}

	return true;
}


/*!	Creates a buffer for the given \a address and prints the address into
	it (hexadecimal representation in network byte order or '<none>').
	If \a printPort is set, the port is printed, too.
	\return B_OK if the address could be printed, \a buffer will point to
		the resulting string
	\return B_BAD_VALUE if no buffer has been given
	\return B_NO_MEMORY if the buffer could not be allocated,
		or does not have enogh space
*/
static status_t
ipv6_print_address_buffer(const sockaddr *_address, char *buffer,
	size_t bufferSize, bool printPort)
{
	const sockaddr_in6 *address = (const sockaddr_in6 *)_address;

	if (buffer == NULL)
		return B_BAD_VALUE;

	if (address == NULL) {
		if (bufferSize < sizeof("<none>"))
			return B_NO_MEMORY;
		strcpy(buffer, "<none>");
	} else {
		if (printPort && bufferSize > 0) {
			*buffer = '[';
			buffer++;
			bufferSize--;
		}

		if (!ip6_sprintf(&address->sin6_addr, buffer, bufferSize))
			return B_NO_MEMORY;

		if (printPort) {
			char port[7];
			sprintf(port, "]:%d", ntohs(address->sin6_port));
			if (bufferSize - strlen(buffer) < strlen(port) + 1)
				return B_NO_MEMORY;
			strcat(buffer, port);
		}
	}

	return B_OK;
}


static status_t
ipv6_print_address(const sockaddr *_address, char **_buffer, bool printPort)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;

	char tmp[64];
	ipv6_print_address_buffer(_address, tmp, sizeof(tmp), printPort);

	*_buffer = strdup(tmp);
	if (*_buffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


/*!	Determines the port of the given \a address.
	\return uint16 representing the port-nr
*/
static uint16
ipv6_get_port(const sockaddr *address)
{
	if (address == NULL || address->sa_len == 0)
		return 0;

	return ((sockaddr_in6 *)address)->sin6_port;
}


/*!	Sets the port of the given \a address to \a port.
	\return B_OK if the port has been set
	\return B_BAD_VALUE if \a address is NULL or has not been initialized
*/
static status_t
ipv6_set_port(sockaddr *address, uint16 port)
{
	if (address == NULL || address->sa_len == 0)
		return B_BAD_VALUE;

	((sockaddr_in6 *)address)->sin6_port = port;
	return B_OK;
}


/*!	Sets \a address to \a from.
	\return B_OK if \a from has been copied into \a address
	\return B_BAD_VALUE if either \a address or \a from is NULL or if the
			address given in from has not been initialized
	\return B_MISMATCHED_VALUES if from is not of family AF_INET6
*/
static status_t
ipv6_set_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL || from->sa_len == 0)
		return B_BAD_VALUE;

	if (from->sa_family != AF_INET6)
		return B_MISMATCHED_VALUES;

	memcpy(address, from, sizeof(sockaddr_in6));
	address->sa_len = sizeof(sockaddr_in6);
	return B_OK;
}


/*!	Updates missing parts in \a address with the values in \a from.
	\return B_OK if \a address has been updated from \a from
	\return B_BAD_VALUE if either \a address or \a from is NULL or if the
			address given in from has not been initialized
	\return B_MISMATCHED_VALUES if from is not of family AF_INET6
*/
static status_t
ipv6_update_to(sockaddr *_address, const sockaddr *_from)
{
	sockaddr_in6 *address = (sockaddr_in6 *)_address;
	const sockaddr_in6 *from = (const sockaddr_in6 *)_from;

	if (address == NULL || from == NULL || from->sin6_len == 0)
		return B_BAD_VALUE;

	if (from->sin6_family != AF_INET6)
		return B_BAD_VALUE;

	address->sin6_family = AF_INET6;
	address->sin6_len = sizeof(sockaddr_in6);

	if (address->sin6_port == 0)
		address->sin6_port = from->sin6_port;

	if (IN6_IS_ADDR_UNSPECIFIED(&address->sin6_addr)) {
		memcpy(address->sin6_addr.s6_addr, from->sin6_addr.s6_addr,
			sizeof(in6_addr));
	}

	return B_OK;
}


/*!	Sets \a address to the empty address (0.0.0.0).
	\return B_OK if \a address has been set
	\return B_BAD_VALUE if \a address is NULL
*/
static status_t
ipv6_set_to_empty_address(sockaddr *address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	memset(address, 0, sizeof(sockaddr_in6));
	address->sa_len = sizeof(sockaddr_in6);
	address->sa_family = AF_INET6;
	return B_OK;
}


static status_t
ipv6_set_to_defaults(sockaddr *_defaultMask, sockaddr *_defaultBroadcast,
	const sockaddr *_address, const sockaddr *_mask)
{
	sockaddr_in6 *defaultMask = (sockaddr_in6 *)_defaultMask;
	sockaddr_in6 *address = (sockaddr_in6 *)_address;
	sockaddr_in6 *mask = (sockaddr_in6 *)_mask;

	if (address == NULL || defaultMask == NULL)
		return B_BAD_VALUE;

	defaultMask->sin6_len = sizeof(sockaddr_in);
	defaultMask->sin6_family = AF_INET6;
	defaultMask->sin6_port = 0;
	if (mask != NULL) {
		memcpy(defaultMask->sin6_addr.s6_addr,
			mask->sin6_addr.s6_addr, sizeof(in6_addr));
	} else {
		// use /128 as the default mask
		memset(defaultMask->sin6_addr.s6_addr, 0xff, sizeof(in6_addr));
	}

	return B_OK;
}


/*!	Computes a hash value of the given \a address.
	\return uint32 representing the hash value
*/
static uint32
ipv6_hash_address(const struct sockaddr* _address, bool includePort)
{
	const sockaddr_in6* address = (const sockaddr_in6*)_address;
	if (address == NULL || address->sin6_len == 0)
		return 0;

	// TODO: also use sin6_flowinfo and sin6_scope_id?
	uint32 port = includePort ? address->sin6_port : 0;

	uint32 result = jenkins_hashword((const uint32*)&address->sin6_addr,
		sizeof(in6_addr) / sizeof(uint32), 0);
	return jenkins_hashword(&port, 1, result);
}


/*!	Computes a hash-value of the given addresses \a ourAddress
	and \a peerAddress.
	\return uint32 representing the hash-value
*/
static uint32
ipv6_hash_address_pair(const sockaddr *ourAddress, const sockaddr *peerAddress)
{
	uint32 result = ipv6_hash_address(peerAddress, true);
	return jenkins_hashword(&result, 1, ipv6_hash_address(ourAddress, true));
}


/*!	Adds the given \a address to the IP-checksum \a checksum.
	\return B_OK if \a address has been added to the checksum
	\return B_BAD_VALUE if either \a address or \a checksum is NULL or if
	        the given address is not initialized
*/
static status_t
ipv6_checksum_address(Checksum *checksum, const sockaddr *address)
{
	if (checksum == NULL || address == NULL || address->sa_len == 0)
		return B_BAD_VALUE;

	in6_addr &a = ((sockaddr_in6 *)address)->sin6_addr;
	for (uint32 i = 0; i < sizeof(in6_addr); i += 2)
		(*checksum) << *(uint16*)(a.s6_addr + i);

	return B_OK;
}


static void
ipv6_get_loopback_address(sockaddr *_address)
{
	sockaddr_in6 *address = (sockaddr_in6 *)_address;
	memset(address, 0, sizeof(sockaddr_in6));
	address->sin6_len = sizeof(sockaddr_in6);
	address->sin6_family = AF_INET6;
	memcpy(&address->sin6_addr, &in6addr_loopback, sizeof(in6_addr));
}


net_address_module_info gIPv6AddressModule = {
	{
		NULL,
		0,
		NULL
	},
	0, // flags
	ipv6_copy_address,
	ipv6_mask_address,
	ipv6_equal_addresses,
	ipv6_equal_ports,
	ipv6_equal_addresses_and_ports,
	ipv6_equal_masked_addresses,
	ipv6_is_empty_address,
	ipv6_is_same_family,
	ipv6_first_mask_bit,
	ipv6_check_mask,
	ipv6_print_address,
	ipv6_print_address_buffer,
	ipv6_get_port,
	ipv6_set_port,
	ipv6_set_to,
	ipv6_set_to_empty_address,
	ipv6_set_to_defaults,
	ipv6_update_to,
	ipv6_hash_address,
	ipv6_hash_address_pair,
	ipv6_checksum_address,
	ipv6_get_loopback_address
};
