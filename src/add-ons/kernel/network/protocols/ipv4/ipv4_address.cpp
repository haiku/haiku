/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <net_datalink.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <NetUtilities.h>

#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>


/*!
	Routing utility function: copies address \a from into a new address
	that is put into \a to.
	If \a replaceWithZeros is set \a from will be replaced by an empty
	address.
	If a \a mask is given it is applied to \a from (such that \a to is the 
	result of \a from & \a mask).
	\return B_OK if the address could be copied
	\return B_NO_MEMORY if the new address could not be allocated
	\return B_MISMATCHED_VALUES if \a address does not match family AF_INET
*/
static status_t
ipv4_copy_address(const sockaddr *from, sockaddr **to,
	bool replaceWithZeros = false, const sockaddr *mask = NULL)
{
	if (replaceWithZeros) {
		*to = (sockaddr *)malloc(sizeof(sockaddr_in));
		if (*to == NULL)
			return B_NO_MEMORY;

		memset(*to, 0, sizeof(sockaddr_in));
		(*to)->sa_family = AF_INET;
		(*to)->sa_len = sizeof(sockaddr_in);
	} else {
		if (from == NULL)
			return B_OK;
		if (from->sa_family != AF_INET)
			return B_MISMATCHED_VALUES;

		*to = (sockaddr *)malloc(sizeof(sockaddr_in));
		if (*to == NULL)
			return B_NO_MEMORY;

		memcpy(*to, from, sizeof(sockaddr_in));

		if (mask != NULL) {
			((sockaddr_in *)*to)->sin_addr.s_addr
				&= ((const sockaddr_in *)mask)->sin_addr.s_addr;
		}
	}
	return B_OK;
}


/*!
	Routing utility function: applies \a mask to given \a address and puts
	the resulting address into \a result.
	\return B_OK if the mask has been applied
	\return B_BAD_VALUE if \a address or \a mask is NULL
*/
static status_t
ipv4_mask_address(const sockaddr *address, const sockaddr *mask, sockaddr *result)
{
	if (address == NULL || result == NULL)
		return B_BAD_VALUE;

	memcpy(result, address, sizeof(sockaddr_in));
	if (mask != NULL) {
		((sockaddr_in *)result)->sin_addr.s_addr 
			&= ((sockaddr_in *)mask)->sin_addr.s_addr;
	}

	return B_OK;
}


/*!
	Checks if the given \a address is the empty address. By default, the port
	is checked, too, but you can avoid that by passing \a checkPort = false.
	\return true if \a address is NULL, uninitialized or the empty address, 
		false if not
*/
static bool
ipv4_is_empty_address(const sockaddr *address, bool checkPort)
{
	if (address == NULL || address->sa_len == 0)
		return true;

	return ((sockaddr_in *)address)->sin_addr.s_addr == 0
		&& (!checkPort || ((sockaddr_in *)address)->sin_port == 0);
}


/*!
	Compares the IP-addresses of the two given address structures \a a and \a b.
	\return true if IP-addresses of \a a and \a b are equal, false if not
*/
static bool
ipv4_equal_addresses(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return ipv4_is_empty_address(a, false);
	if (a == NULL && b != NULL)
		return ipv4_is_empty_address(b, false);

	return ((sockaddr_in *)a)->sin_addr.s_addr == ((sockaddr_in *)b)->sin_addr.s_addr;
}


/*!
	Compares the ports of the two given address structures \a a and \a b.
	\return true if ports of \a a and \a b are equal, false if not
*/
static bool
ipv4_equal_ports(const sockaddr *a, const sockaddr *b)
{
	uint16 portA = a ? ((sockaddr_in *)a)->sin_port : 0;
	uint16 portB = b ? ((sockaddr_in *)b)->sin_port : 0;
	return portA == portB;
}


/*!
	Compares the IP-addresses and ports of the two given address structures 
	\a a and \a b.
	\return true if IP-addresses and ports of \a a and \a b are equal, false if not
*/
static bool
ipv4_equal_addresses_and_ports(const sockaddr *a, const sockaddr *b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a != NULL && b == NULL)
		return ipv4_is_empty_address(a, true);
	if (a == NULL && b != NULL)
		return ipv4_is_empty_address(b, true);

	return ((sockaddr_in *)a)->sin_addr.s_addr == ((sockaddr_in *)b)->sin_addr.s_addr
		&& ((sockaddr_in *)a)->sin_port == ((sockaddr_in *)b)->sin_port;
}


/*!
	Applies the given \a mask two \a a and \a b and then checks whether 
	the masked addresses match.
	\return true if \a a matches \a b after masking both, false if not
*/
static bool
ipv4_equal_masked_addresses(const sockaddr *a, const sockaddr *b, 
	const sockaddr *mask)
{
	if (a == NULL && b == NULL)
		return true;

	sockaddr emptyAddr;
	if (a == NULL || b == NULL) {
		memset(&emptyAddr, 0, sizeof(sockaddr_in));
		if (a == NULL)
			a = &emptyAddr;
		else if (b == NULL)
			b = &emptyAddr;
	}

	uint32 aValue = ((sockaddr_in *)a)->sin_addr.s_addr;
	uint32 bValue = ((sockaddr_in *)b)->sin_addr.s_addr;

	if (!mask)
		return aValue == bValue;

	uint32 maskValue = ((sockaddr_in *)mask)->sin_addr.s_addr;
	return (aValue & maskValue) == (bValue & maskValue);
}


/*!
	Routing utility function: determines the least significant bit that is set
	in the given \a mask. 
	\return the number of the first bit that is set (0-32, where 32 means 
		that there's no bit set in the mask).
*/
static int32
ipv4_first_mask_bit(const sockaddr *_mask)
{
	if (_mask == NULL)
		return 0;

	uint32 mask = ntohl(((const sockaddr_in *)_mask)->sin_addr.s_addr);

	// TODO: this can be optimized, there are also some nice assembler mnemonics for this
	int8 bit = 0;
	for (uint32 bitMask = 1; bit < 32; bitMask <<= 1, bit++) {
		if (mask & bitMask)
			return bit;
	}

	return 32;
}


/*!
	Routing utility function: checks the given \a mask for correctness (which
	means that (starting with LSB) consists zero or more unset bits, followed
	by bits that are all set).
	\return true if \a mask is ok, false if not
*/
static bool
ipv4_check_mask(const sockaddr *_mask)
{
	if (_mask == NULL)
		return true;

	uint32 mask = ntohl(((const sockaddr_in *)_mask)->sin_addr.s_addr);

	// A mask (from LSB) starts with zeros, after the first one, only ones 
	// are allowed:
	bool zero = true;
	int8 bit = 0;
	for (uint32 bitMask = 1; bit < 32; bitMask <<= 1, bit++) {
		if (mask & bitMask) {
			if (zero)
				zero = false;
		} else if (!zero)
			return false;
	}
	return true;
}


/*!
	Creates a buffer for the given \a address and prints the address into
	it (hexadecimal representation in host byte order or '<none>'). 
	If \a printPort is set, the port is printed, too.
	\return B_OK if the address could be printed, \a buffer will point to
		the resulting string
	\return B_BAD_VALUE if no buffer has been given
	\return B_NO_MEMORY if the buffer could not be allocated
*/
static status_t
ipv4_print_address(const sockaddr *address, char **_buffer, bool printPort)
{
	if (_buffer == NULL)
		return B_BAD_VALUE;

	int bufLen = printPort ? 15 : 9;
	char *buffer  = (char *)malloc(bufLen);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (address == NULL)
		strcpy(buffer, "<none>");
	else if (printPort) {
		sprintf(buffer, "%08lx:%u", ntohl(((sockaddr_in *)address)->sin_addr.s_addr),
			ntohs(((sockaddr_in *)address)->sin_port));
	} else
		sprintf(buffer, "%08lx", ntohl(((sockaddr_in *)address)->sin_addr.s_addr));
	*_buffer = buffer;
	return B_OK;
}


/*!
	Determines the port of the given \a address. 
	\return uint16 representing the port-nr
*/
static uint16
ipv4_get_port(const sockaddr *address)
{
	if (address == NULL)
		return 0;

	return ((sockaddr_in *)address)->sin_port;
}


/*!
	Sets the port of the given \a address to \a port.
	\return B_OK if the port has been set
	\return B_BAD_VALUE if \a address is NULL
*/
static status_t
ipv4_set_port(sockaddr *address, uint16 port)
{
	if (address == NULL)
		return B_BAD_VALUE;

	((sockaddr_in *)address)->sin_port = port;
	return B_OK;
}


/*!
	Sets \a address to \a from.
	\return B_OK if \a from has been copied into \a address
	\return B_BAD_VALUE if either \a address or \a from is NULL
	\return B_MISMATCHED_VALUES if from is not of family AF_INET
*/
static status_t
ipv4_set_to(sockaddr *address, const sockaddr *from)
{
	if (address == NULL || from == NULL)
		return B_BAD_VALUE;

	if (from->sa_family != AF_INET)
		return B_MISMATCHED_VALUES;

	memcpy(address, from, sizeof(sockaddr_in));
	address->sa_len = sizeof(sockaddr_in);
	return B_OK;
}


/*!
	Sets \a address to the empty address (0.0.0.0).
	\return B_OK if \a address has been set
	\return B_BAD_VALUE if \a address is NULL
*/
static status_t
ipv4_set_to_empty_address(sockaddr *address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	memset(address, 0, sizeof(sockaddr_in));
	address->sa_len = sizeof(sockaddr_in);
	address->sa_family = AF_INET;
	return B_OK;
}


/*!
	Computes a hash-value of the given addresses \a ourAddress 
	and \a peerAddress.
	\return uint32 representing the hash-value
*/
static uint32
ipv4_hash_address_pair(const sockaddr *ourAddress, const sockaddr *peerAddress)
{
	int32 hash = (((sockaddr_in *)ourAddress)->sin_port
						| ((sockaddr_in *)peerAddress)->sin_port << 16)
					^ ((sockaddr_in *)ourAddress)->sin_addr.s_addr
					^ ((sockaddr_in *)peerAddress)->sin_addr.s_addr;
	return hash;
}


/*!
	Adds the given \a address to the IP-checksum \a checksum.
	\return B_OK if \a address has been added to the checksum
	\return B_BAD_VALUE if either \a address or \a checksum is NULL
*/
static status_t
ipv4_checksum_address(struct Checksum *checksum, const sockaddr *address)
{
	if (checksum == NULL || address == NULL)
		return B_BAD_VALUE;

	(*checksum) << (uint32)((sockaddr_in *)address)->sin_addr.s_addr;
	return B_OK;
}


net_address_module_info gIPv4AddressModule = {
	{
		NULL,
		0,
		NULL
	},
	ipv4_copy_address,
	ipv4_mask_address,
	ipv4_equal_addresses,
	ipv4_equal_ports,
	ipv4_equal_addresses_and_ports,
	ipv4_equal_masked_addresses,
	ipv4_is_empty_address,
	ipv4_first_mask_bit,
	ipv4_check_mask,
	ipv4_print_address,
	ipv4_get_port,
	ipv4_set_port,
	ipv4_set_to,
	ipv4_set_to_empty_address,
	ipv4_hash_address_pair,
	ipv4_checksum_address,
	NULL // ipv4_matches_broadcast_address,
};
