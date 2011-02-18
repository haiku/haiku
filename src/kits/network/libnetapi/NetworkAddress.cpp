/*
 * Copyright 2010-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkAddress.h>

#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/sockio.h>


/* The GCC builtin below only exists in > GCC 3.4
 * Benefits include faster execution time as the builtin
 * uses a bitcounting cpu instruction if it exists
 */
#if __GNUC__ > 3
#	define addr_bitcount(bitfield) __builtin_popcount(bitfield)
#else
static ssize_t
addr_bitcount(uint32 bitfield)
{
	ssize_t result = 0;
	for (uint8 i = 32; i > 0; i--) {
		if ((bitfield & (1 << (i - 1))) == 0)
			break;
		result++;
	}
	return result;
}
#endif


static uint8
from_hex(char hex)
{
	if (isdigit(hex))
		return hex - '0';

	return tolower(hex) - 'a' + 10;
}


// #pragma mark -


BNetworkAddress::BNetworkAddress()
{
	Unset();
}


BNetworkAddress::BNetworkAddress(const char* host, uint16 port, uint32 flags)
{
	SetTo(host, port, flags);
}


BNetworkAddress::BNetworkAddress(const char* host, const char* service,
	uint32 flags)
{
	SetTo(host, service, flags);
}


BNetworkAddress::BNetworkAddress(int family, const char* host, uint16 port,
	uint32 flags)
{
	SetTo(family, host, port, flags);
}


BNetworkAddress::BNetworkAddress(int family, const char* host,
	const char* service, uint32 flags)
{
	SetTo(family, host, service, flags);
}


BNetworkAddress::BNetworkAddress(const sockaddr& address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const sockaddr_storage& address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const sockaddr_in& address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const sockaddr_in6& address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const sockaddr_dl& address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(in_addr_t address, uint16 port)
{
	SetTo(address, port);
}


BNetworkAddress::BNetworkAddress(const in6_addr& address, uint16 port)
{
	SetTo(address, port);
}


BNetworkAddress::BNetworkAddress(const BNetworkAddress& other)
	:
	fAddress(other.fAddress),
	fStatus(other.fStatus)
{
}


BNetworkAddress::~BNetworkAddress()
{
}


status_t
BNetworkAddress::InitCheck() const
{
	return fStatus;
}


void
BNetworkAddress::Unset()
{
	fAddress.ss_family = AF_UNSPEC;
	fAddress.ss_len = 2;
	fStatus = B_OK;
}


status_t
BNetworkAddress::SetTo(const char* host, uint16 port, uint32 flags)
{
	BNetworkAddressResolver resolver;
	status_t status = resolver.SetTo(host, port, flags);
	if (status != B_OK)
		return status;

	// Prefer IPv6 addresses

	uint32 cookie = 0;
	status = resolver.GetNextAddress(AF_INET6, &cookie, *this);
	if (status == B_OK)
		return B_OK;

	cookie = 0;
	return resolver.GetNextAddress(&cookie, *this);
}


status_t
BNetworkAddress::SetTo(const char* host, const char* service, uint32 flags)
{
	BNetworkAddressResolver resolver;
	status_t status = resolver.SetTo(host, service, flags);
	if (status != B_OK)
		return status;

	// Prefer IPv6 addresses

	uint32 cookie = 0;
	status = resolver.GetNextAddress(AF_INET6, &cookie, *this);
	if (status == B_OK)
		return B_OK;

	cookie = 0;
	return resolver.GetNextAddress(&cookie, *this);
}


status_t
BNetworkAddress::SetTo(int family, const char* host, uint16 port, uint32 flags)
{
	if (family == AF_LINK) {
		if (port != 0)
			return B_BAD_VALUE;
		return _ParseLinkAddress(host);
	}

	BNetworkAddressResolver resolver;
	status_t status = resolver.SetTo(family, host, port, flags);
	if (status != B_OK)
		return status;

	uint32 cookie = 0;
	return resolver.GetNextAddress(&cookie, *this);
}


status_t
BNetworkAddress::SetTo(int family, const char* host, const char* service,
	uint32 flags)
{
	if (family == AF_LINK) {
		if (service != NULL)
			return B_BAD_VALUE;
		return _ParseLinkAddress(host);
	}

	BNetworkAddressResolver resolver;
	status_t status = resolver.SetTo(family, host, service, flags);
	if (status != B_OK)
		return status;

	uint32 cookie = 0;
	return resolver.GetNextAddress(&cookie, *this);
}


void
BNetworkAddress::SetTo(const sockaddr& address)
{
	if (address.sa_family == AF_UNSPEC) {
		Unset();
		return;
	}

	size_t length = min_c(sizeof(sockaddr_storage), address.sa_len);
	switch (address.sa_family) {
		case AF_INET:
			length = sizeof(sockaddr_in);
			break;
		case AF_INET6:
			length = sizeof(sockaddr_in6);
			break;
		case AF_LINK:
		{
			sockaddr_dl& link = (sockaddr_dl&)address;
			length = sizeof(sockaddr_dl) - sizeof(link.sdl_data) + link.sdl_alen
				+ link.sdl_nlen + link.sdl_slen;
			break;
		}
	}

	SetTo(address, length);
}


void
BNetworkAddress::SetTo(const sockaddr& address, size_t length)
{
	if (address.sa_family == AF_UNSPEC || length == 0) {
		Unset();
		return;
	}

	memcpy(&fAddress, &address, length);
	fAddress.ss_len = length;
	fStatus = B_OK;
}


void
BNetworkAddress::SetTo(const sockaddr_storage& address)
{
	SetTo((sockaddr&)address);
}


void
BNetworkAddress::SetTo(const sockaddr_in& address)
{
	SetTo((sockaddr&)address);
}


void
BNetworkAddress::SetTo(const sockaddr_in6& address)
{
	SetTo((sockaddr&)address);
}


void
BNetworkAddress::SetTo(const sockaddr_dl& address)
{
	SetTo((sockaddr&)address);
}


void
BNetworkAddress::SetTo(in_addr_t inetAddress, uint16 port)
{
	memset(&fAddress, 0, sizeof(sockaddr_storage));

	fAddress.ss_family = AF_INET;
	fAddress.ss_len = sizeof(sockaddr_in);
	SetAddress(inetAddress);
	SetPort(port);

	fStatus = B_OK;
}


void
BNetworkAddress::SetTo(const in6_addr& inet6Address, uint16 port)
{
	memset(&fAddress, 0, sizeof(sockaddr_storage));

	fAddress.ss_family = AF_INET6;
	fAddress.ss_len = sizeof(sockaddr_in6);
	SetAddress(inet6Address);
	SetPort(port);

	fStatus = B_OK;
}


void
BNetworkAddress::SetTo(const BNetworkAddress& other)
{
	fAddress = other.fAddress;
	fStatus = other.fStatus;
}


status_t
BNetworkAddress::SetToBroadcast(int family, uint16 port)
{
	if (family != AF_INET)
		return fStatus = B_NOT_SUPPORTED;

	SetTo(INADDR_BROADCAST, port);
	return B_OK;
}


status_t
BNetworkAddress::SetToLocal(int family, uint16 port)
{
	// TODO: choose a local address from the network interfaces
	return fStatus = B_NOT_SUPPORTED;
}


status_t
BNetworkAddress::SetToLoopback(int family, uint16 port)
{
	switch (family) {
		// TODO: choose family depending on availability of IPv6
		case AF_UNSPEC:
		case AF_INET:
			SetTo(htonl(INADDR_LOOPBACK), port);
			break;

		case AF_INET6:
			SetTo(in6addr_loopback, port);
			break;

		default:
			return fStatus = B_NOT_SUPPORTED;
	}

	return B_OK;
}


status_t
BNetworkAddress::SetToMask(int family, uint32 prefixLength)
{
	switch (family) {
		case AF_INET:
		{
			if (prefixLength > 32)
				return B_BAD_VALUE;

			sockaddr_in& mask = (sockaddr_in&)fAddress;
			memset(&fAddress, 0, sizeof(sockaddr_storage));
			mask.sin_family = AF_INET;
			mask.sin_len = sizeof(sockaddr_in);

			uint32 hostMask = 0;
			for (uint8 i = 32; i > 32 - prefixLength; i--)
				hostMask |= 1 << (i - 1);

			mask.sin_addr.s_addr = htonl(hostMask);
			break;
		}

		case AF_INET6:
		{
			if (prefixLength > 128)
				return B_BAD_VALUE;

			sockaddr_in6& mask = (sockaddr_in6&)fAddress;
			memset(&fAddress, 0, sizeof(sockaddr_storage));
			mask.sin6_family = AF_INET6;
			mask.sin6_len = sizeof(sockaddr_in6);

			for (uint8 i = 0; i < sizeof(in6_addr); i++, prefixLength -= 8) {
				if (prefixLength < 8) {
					mask.sin6_addr.s6_addr[i]
						= (uint8)(0xff << (8 - prefixLength));
					break;
				}

				mask.sin6_addr.s6_addr[i] = 0xff;
			}
			break;
		}

		default:
			return fStatus = B_NOT_SUPPORTED;
	}

	return fStatus = B_OK;
}


status_t
BNetworkAddress::SetToWildcard(int family, uint16 port)
{
	switch (family) {
		case AF_INET:
			SetTo(INADDR_ANY, port);
			break;

		case AF_INET6:
			SetTo(in6addr_any, port);
			break;

		default:
			return fStatus = B_NOT_SUPPORTED;
	}

	return B_OK;
}


status_t
BNetworkAddress::SetAddress(in_addr_t inetAddress)
{
	if (Family() != AF_INET)
		return B_BAD_VALUE;

	sockaddr_in& address = (sockaddr_in&)fAddress;
	address.sin_addr.s_addr = inetAddress;
	return B_OK;
}


status_t
BNetworkAddress::SetAddress(const in6_addr& inet6Address)
{
	if (Family() != AF_INET6)
		return B_BAD_VALUE;

	sockaddr_in6& address = (sockaddr_in6&)fAddress;
	memcpy(address.sin6_addr.s6_addr, &inet6Address,
		sizeof(address.sin6_addr.s6_addr));
	return B_OK;
}


void
BNetworkAddress::SetPort(uint16 port)
{
	switch (fAddress.ss_family) {
		case AF_INET:
			((sockaddr_in&)fAddress).sin_port = htons(port);
			break;

		case AF_INET6:
			((sockaddr_in6&)fAddress).sin6_port = htons(port);
			break;

		default:
			break;
	}
}


void
BNetworkAddress::SetToLinkLevel(uint8* address, size_t length)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	memset(&link, 0, sizeof(sockaddr_dl));

	link.sdl_family = AF_LINK;
	link.sdl_alen = length;
	memcpy(LLADDR(&link), address, length);

	link.sdl_len = sizeof(sockaddr_dl);
	if (length > sizeof(link.sdl_data))
		link.sdl_len += length - sizeof(link.sdl_data);
}


void
BNetworkAddress::SetToLinkLevel(const char* name)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	memset(&link, 0, sizeof(sockaddr_dl));

	size_t length = strlen(name);
	if (length > sizeof(fAddress) - sizeof(sockaddr_dl) + sizeof(link.sdl_data))
		length = sizeof(fAddress) - sizeof(sockaddr_dl) + sizeof(link.sdl_data);

	link.sdl_family = AF_LINK;
	link.sdl_nlen = length;

	memcpy(link.sdl_data, name, link.sdl_nlen);

	link.sdl_len = sizeof(sockaddr_dl);
	if (link.sdl_nlen > sizeof(link.sdl_data))
		link.sdl_len += link.sdl_nlen - sizeof(link.sdl_data);
}


void
BNetworkAddress::SetToLinkLevel(uint32 index)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	memset(&link, 0, sizeof(sockaddr_dl));

	link.sdl_family = AF_LINK;
	link.sdl_len = sizeof(sockaddr_dl);
	link.sdl_index = index;
}


void
BNetworkAddress::SetLinkLevelIndex(uint32 index)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	link.sdl_index = index;
}


void
BNetworkAddress::SetLinkLevelType(uint8 type)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	link.sdl_type = type;
}


void
BNetworkAddress::SetLinkLevelFrameType(uint16 frameType)
{
	sockaddr_dl& link = (sockaddr_dl&)fAddress;
	link.sdl_e_type = htons(frameType);
}


int
BNetworkAddress::Family() const
{
	return fAddress.ss_family;
}


uint16
BNetworkAddress::Port() const
{
	switch (fAddress.ss_family) {
		case AF_INET:
			return ntohs(((sockaddr_in&)fAddress).sin_port);

		case AF_INET6:
			return ntohs(((sockaddr_in6&)fAddress).sin6_port);

		default:
			return 0;
	}
}


size_t
BNetworkAddress::Length() const
{
	return fAddress.ss_len;
}


const sockaddr&
BNetworkAddress::SockAddr() const
{
	return (const sockaddr&)fAddress;
}


sockaddr&
BNetworkAddress::SockAddr()
{
	return (sockaddr&)fAddress;
}


bool
BNetworkAddress::IsEmpty() const
{
	return fAddress.ss_len == 0 || fAddress.ss_family == AF_UNSPEC;
}


bool
BNetworkAddress::IsWildcard() const
{
	switch (fAddress.ss_family) {
		case AF_INET:
			return ((sockaddr_in&)fAddress).sin_addr.s_addr == INADDR_ANY;

		case AF_INET6:
			return !memcmp(&((sockaddr_in6&)fAddress).sin6_addr, &in6addr_any,
				sizeof(in6_addr));

		default:
			return false;
	}
}


bool
BNetworkAddress::IsBroadcast() const
{
	switch (fAddress.ss_family) {
		case AF_INET:
			return ((sockaddr_in&)fAddress).sin_addr.s_addr == INADDR_BROADCAST;

		case AF_INET6:
			// There is no broadcast in IPv6, only multicast/anycast
			return IN6_IS_ADDR_MULTICAST(&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticast() const
{
	switch (fAddress.ss_family) {
		case AF_INET:
			return IN_MULTICAST(((sockaddr_in&)fAddress).sin_addr.s_addr);

		case AF_INET6:
			return IN6_IS_ADDR_MULTICAST(&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticastGlobal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_MC_GLOBAL(&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticastNodeLocal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_MC_NODELOCAL(
				&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticastLinkLocal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_MC_LINKLOCAL(
				&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticastSiteLocal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_MC_SITELOCAL(
				&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsMulticastOrgLocal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_MC_ORGLOCAL(
				&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsLinkLocal() const
{
	// TODO: ipv4
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_LINKLOCAL(&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsSiteLocal() const
{
	switch (fAddress.ss_family) {
		case AF_INET6:
			return IN6_IS_ADDR_SITELOCAL(&((sockaddr_in6&)fAddress).sin6_addr);

		default:
			return false;
	}
}


bool
BNetworkAddress::IsLocal() const
{
	BNetworkRoster& roster = BNetworkRoster::Default();

	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		int32 count = interface.CountAddresses();
		for (int32 j = 0; j < count; j++) {
			BNetworkInterfaceAddress address;
			if (interface.GetAddressAt(j, address) != B_OK)
				break;

			if (Equals(address.Address(), false))
				return true;
		}
	}

	return false;
}


ssize_t
BNetworkAddress::PrefixLength() const
{
	switch (fAddress.ss_family) {
		case AF_INET:
		{
			sockaddr_in& mask = (sockaddr_in&)fAddress;

			uint32 hostMask = ntohl(mask.sin_addr.s_addr);
			return addr_bitcount(hostMask);
		}

		case AF_INET6:
		{
			sockaddr_in6& mask = (sockaddr_in6&)fAddress;

			// TODO : see if we can use the optimized addr_bitcount for this
			ssize_t result = 0;
			for (uint8 i = 0; i < sizeof(in6_addr); i++) {
				for (uint8 j = 0; j < 8; j++) {
					if (!(mask.sin6_addr.s6_addr[i] & (1 << j)))
						return result;
					result++;
				}
			}

			return 128;
		}

		default:
			return B_NOT_SUPPORTED;
	}
}


uint32
BNetworkAddress::LinkLevelIndex() const
{
	return ((sockaddr_dl&)fAddress).sdl_index;
}


BString
BNetworkAddress::LinkLevelInterface() const
{
	sockaddr_dl& address = (sockaddr_dl&)fAddress;
	if (address.sdl_nlen == 0)
		return "";

	BString name;
	name.SetTo((const char*)address.sdl_data, address.sdl_nlen);

	return name;
}


uint8
BNetworkAddress::LinkLevelType() const
{
	return ((sockaddr_dl&)fAddress).sdl_type;
}


uint16
BNetworkAddress::LinkLevelFrameType() const
{
	return ntohs(((sockaddr_dl&)fAddress).sdl_e_type);
}


uint8*
BNetworkAddress::LinkLevelAddress() const
{
	return LLADDR(&(sockaddr_dl&)fAddress);
}


size_t
BNetworkAddress::LinkLevelAddressLength() const
{
	return ((sockaddr_dl&)fAddress).sdl_alen;
}


status_t
BNetworkAddress::ResolveForDestination(const BNetworkAddress& destination)
{
	if (!IsWildcard())
		return B_OK;
	if (destination.fAddress.ss_family != fAddress.ss_family)
		return B_BAD_VALUE;

	char buffer[2048];
	memset(buffer, 0, sizeof(buffer));

	route_entry* route = (route_entry*)buffer;
	route->destination = (sockaddr*)&destination.fAddress;

	int socket = ::socket(fAddress.ss_family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	if (ioctl(socket, SIOCGETRT, route, sizeof(buffer)) != 0) {
		close(socket);
		return errno;
	}

	uint16 port = Port();
	memcpy(&fAddress, route->source, sizeof(sockaddr_storage));
	SetPort(port);

	return B_OK;
}


status_t
BNetworkAddress::ResolveTo(const BNetworkAddress& address)
{
	if (!IsWildcard())
		return B_OK;
	if (address.fAddress.ss_family != fAddress.ss_family)
		return B_BAD_VALUE;

	uint16 port = Port();
	*this = address;
	SetPort(port);

	return B_OK;
}


BString
BNetworkAddress::ToString(bool includePort) const
{
	char buffer[512];

	switch (fAddress.ss_family) {
		case AF_INET:
			inet_ntop(AF_INET, &((sockaddr_in&)fAddress).sin_addr, buffer,
				sizeof(buffer));
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &((sockaddr_in6&)fAddress).sin6_addr,
				buffer, sizeof(buffer));
			break;

		case AF_LINK:
		{
			uint8 *byte = LinkLevelAddress();
			char* target = buffer;
			int bytesLeft = sizeof(buffer);
			target[0] = '\0';

			for (size_t i = 0; i < LinkLevelAddressLength(); i++) {
				if (i != 0 && bytesLeft > 1) {
					target[0] = ':';
					target[1] = '\0';
					target++;
					bytesLeft--;
				}

				int bytesWritten = snprintf(target, bytesLeft, "%02x", byte[i]);
				if (bytesWritten >= bytesLeft)
					break;

				target += bytesWritten;
				bytesLeft -= bytesWritten;
			}
			break;
		}

		default:
			return "";
	}

	BString address = buffer;
	if (includePort && Port() != 0) {
		if (fAddress.ss_family == AF_INET6) {
			address = "[";
			address += buffer;
			address += "]";
		}

		snprintf(buffer, sizeof(buffer), ":%u", Port());
		address += buffer;
	}

	return address;
}


BString
BNetworkAddress::HostName() const
{
	// TODO: implement host name lookup
	return ToString(false);
}


BString
BNetworkAddress::ServiceName() const
{
	// TODO: implement service lookup
	BString portName;
	portName << Port();
	return portName;
}


bool
BNetworkAddress::Equals(const BNetworkAddress& other, bool includePort) const
{
	if (IsEmpty() && other.IsEmpty())
		return true;

	if (Family() != other.Family()
		|| (includePort && Port() != other.Port()))
		return false;

	switch (fAddress.ss_family) {
		case AF_INET:
		{
			sockaddr_in& address = (sockaddr_in&)fAddress;
			sockaddr_in& otherAddress = (sockaddr_in&)other.fAddress;
			return memcmp(&address.sin_addr, &otherAddress.sin_addr,
				sizeof(address.sin_addr)) == 0;
		}

		case AF_INET6:
		{
			sockaddr_in6& address = (sockaddr_in6&)fAddress;
			sockaddr_in6& otherAddress = (sockaddr_in6&)other.fAddress;
			return memcmp(&address.sin6_addr, &otherAddress.sin6_addr,
				sizeof(address.sin6_addr)) == 0;
		}

		default:
			if (fAddress.ss_len != other.fAddress.ss_len)
				return false;

			return memcmp(&fAddress, &other.fAddress, fAddress.ss_len);
	}
}


// #pragma mark - BFlattenable implementation


bool
BNetworkAddress::IsFixedSize() const
{
	return false;
}


type_code
BNetworkAddress::TypeCode() const
{
	return B_NETWORK_ADDRESS_TYPE;
}


ssize_t
BNetworkAddress::FlattenedSize() const
{
	return Length();
}


status_t
BNetworkAddress::Flatten(void* buffer, ssize_t size) const
{
	if (buffer == NULL || size < FlattenedSize())
		return B_BAD_VALUE;

	memcpy(buffer, &fAddress, Length());
	return B_OK;
}


status_t
BNetworkAddress::Unflatten(type_code code, const void* buffer, ssize_t size)
{
	// 2 bytes minimum for family, and length
	if (buffer == NULL || size < 2)
		return fStatus = B_BAD_VALUE;
	if (!AllowsTypeCode(code))
		return fStatus = B_BAD_TYPE;

	memcpy(&fAddress, buffer, min_c(size, (ssize_t)sizeof(fAddress)));

	// check if this can contain a valid address
	if (fAddress.ss_family != AF_UNSPEC && size < (ssize_t)sizeof(sockaddr))
		return fStatus = B_BAD_VALUE;

	return fStatus = B_OK;
}


// #pragma mark - operators


BNetworkAddress&
BNetworkAddress::operator=(const BNetworkAddress& other)
{
	memcpy(&fAddress, &other.fAddress, other.fAddress.ss_len);
	fStatus = other.fStatus;

	return *this;
}


bool
BNetworkAddress::operator==(const BNetworkAddress& other) const
{
	return Equals(other);
}


bool
BNetworkAddress::operator!=(const BNetworkAddress& other) const
{
	return !Equals(other);
}


bool
BNetworkAddress::operator<(const BNetworkAddress& other) const
{
	if (Family() < other.Family())
		return true;
	if (Family() > other.Family())
		return false;

	int compare;

	switch (fAddress.ss_family) {
		default:
		case AF_INET:
		{
			sockaddr_in& address = (sockaddr_in&)fAddress;
			sockaddr_in& otherAddress = (sockaddr_in&)other.fAddress;
			compare = memcmp(&address.sin_addr, &otherAddress.sin_addr,
				sizeof(address.sin_addr));
			break;
		}

		case AF_INET6:
		{
			sockaddr_in6& address = (sockaddr_in6&)fAddress;
			sockaddr_in6& otherAddress = (sockaddr_in6&)other.fAddress;
			compare = memcmp(&address.sin6_addr, &otherAddress.sin6_addr,
				sizeof(address.sin6_addr));
			break;
		}

		case AF_LINK:
			if (LinkLevelAddressLength() < other.LinkLevelAddressLength())
				return true;
			if (LinkLevelAddressLength() > other.LinkLevelAddressLength())
				return true;

			// TODO: could compare index, and name, too
			compare = memcmp(LinkLevelAddress(), other.LinkLevelAddress(),
				LinkLevelAddressLength());
			break;
	}

	if (compare < 0)
		return true;
	if (compare > 0)
		return false;

	return Port() < other.Port();
}


BNetworkAddress::operator const sockaddr*() const
{
	return (const sockaddr*)&fAddress;
}


BNetworkAddress::operator const sockaddr&() const
{
	return (const sockaddr&)fAddress;
}


BNetworkAddress::operator sockaddr*()
{
	return (sockaddr*)&fAddress;
}


BNetworkAddress::operator const sockaddr*()
{
	return (sockaddr*)&fAddress;
}


BNetworkAddress::operator sockaddr&()
{
	return (sockaddr&)fAddress;
}


BNetworkAddress::operator const sockaddr&()
{
	return (sockaddr&)fAddress;
}


// #pragma mark - private


status_t
BNetworkAddress::_ParseLinkAddress(const char* address)
{
	uint8 linkAddress[128];
	uint32 length = 0;
	while (length < sizeof(linkAddress)) {
		if (!isxdigit(address[0]) || !isxdigit(address[1]))
			return B_BAD_VALUE;

		linkAddress[length++] = (from_hex(address[0]) << 4)
			| from_hex(address[1]);

		if (address[2] == '\0')
			break;
		if (address[2] != ':')
			return B_BAD_VALUE;

		address += 3;
	}

	SetToLinkLevel(linkAddress, length);
	return B_OK;
}
