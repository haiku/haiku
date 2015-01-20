/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkInterface.h>

#include <errno.h>
#include <string.h>
#include <sys/sockio.h>

#include <AutoDeleter.h>


namespace BPrivate {

int
family_from_interface_address(const BNetworkInterfaceAddress& address)
{
	if (address.Address().sa_family != AF_UNSPEC)
		return address.Address().sa_family;
	if (address.Mask().sa_family != AF_UNSPEC)
		return address.Mask().sa_family;
	if (address.Destination().sa_family != AF_UNSPEC)
		return address.Destination().sa_family;

	return AF_INET;
}


status_t
do_ifaliasreq(const char* name, int32 option, BNetworkInterfaceAddress& address,
	bool readBack = false)
{
	int family = AF_INET;
	if (!readBack)
		family = family_from_interface_address(address);

	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	ifaliasreq request;
	strlcpy(request.ifra_name, name, IF_NAMESIZE);
	request.ifra_index = address.Index();
	request.ifra_flags = address.Flags();

	memcpy(&request.ifra_addr, &address.Address(),
		address.Address().sa_len);
	memcpy(&request.ifra_mask, &address.Mask(),
		address.Mask().sa_len);
	memcpy(&request.ifra_broadaddr, &address.Broadcast(),
		address.Broadcast().sa_len);

	if (ioctl(socket, option, &request, sizeof(struct ifaliasreq)) < 0)
		return errno;

	if (readBack) {
		address.SetFlags(request.ifra_flags);
		address.SetAddress((sockaddr&)request.ifra_addr);
		address.SetMask((sockaddr&)request.ifra_mask);
		address.SetBroadcast((sockaddr&)request.ifra_broadaddr);
	}

	return B_OK;
}

};

using namespace BPrivate;


// #pragma mark -


BNetworkInterfaceAddress::BNetworkInterfaceAddress()
	:
	fIndex(-1),
	fFlags(0)
{
}


BNetworkInterfaceAddress::~BNetworkInterfaceAddress()
{
}


status_t
BNetworkInterfaceAddress::SetTo(const char* name, int32 index)
{
	fIndex = index;
	return do_ifaliasreq(name, B_SOCKET_GET_ALIAS, *this, true);
}


void
BNetworkInterfaceAddress::SetAddress(const sockaddr& address)
{
	memcpy(&fAddress, &address, address.sa_len);
}


void
BNetworkInterfaceAddress::SetMask(const sockaddr& mask)
{
	memcpy(&fMask, &mask, mask.sa_len);
}


void
BNetworkInterfaceAddress::SetBroadcast(const sockaddr& broadcast)
{
	memcpy(&fBroadcast, &broadcast, broadcast.sa_len);
}


void
BNetworkInterfaceAddress::SetDestination(const sockaddr& destination)
{
	memcpy(&fBroadcast, &destination, destination.sa_len);
}


void
BNetworkInterfaceAddress::SetFlags(uint32 flags)
{
	fFlags = flags;
}



