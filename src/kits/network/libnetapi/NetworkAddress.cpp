/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkAddress.h>

#include <ByteOrder.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>


static bool
strip_port(BString& host, BString& port)
{
	int32 separator = host.FindFirst(':');
	if (separator != -1) {
		// looks like there is a port
		host.CopyInto(port, separator + 1, -1);
		host.Truncate(separator);

		return true;
	}

	return false;
}


static status_t
resolve_address(int family, const char* host, const char* port,
	int type, sockaddr_storage& address)
{
	addrinfo hint = {0};
	hint.ai_family = family;
	hint.ai_socktype = type;
	hint.ai_protocol = 0;

	if (host == NULL && port == NULL) {
		port = "0";
		hint.ai_flags = AI_PASSIVE;
	}

	addrinfo* info;
	int status = getaddrinfo(host, port, &hint, &info);
	if (status != 0) {
		// TODO: improve error reporting
		return B_ERROR;
	}

	bool foundAddress = false;

	if (family == AF_UNSPEC) {
		// Prefer IPv6 addresses over IPv4 addresses

		for (const addrinfo* next = info; next != NULL; next = next->ai_next) {
			if (next->ai_family == AF_INET6) {
				memcpy(&address, next->ai_addr, next->ai_addrlen);
				foundAddress = true;
				break;
			}
		}
	}

	if (!foundAddress) {
		// No preferred, or no IPv6 address found, just take the first one
		// that works
		memcpy(&address, info->ai_addr, info->ai_addrlen);
	}

	freeaddrinfo(info);
	return B_OK;
}


// #pragma mark -


BNetworkAddress::BNetworkAddress(int family, const char* host, uint16 port)
{
	SetTo(family, host, port);
}


BNetworkAddress::BNetworkAddress(const char* host, uint16 port)
{
	SetTo(host, port);
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


BNetworkAddress::BNetworkAddress(const in_addr_t address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const in6_addr* address)
{
	SetTo(address);
}


BNetworkAddress::BNetworkAddress(const BNetworkAddress& other)
	:
	fAddress(other.fAddress),
	fStatus(other.fStatus)
{
}


BNetworkAddress::BNetworkAddress(BMessage* archive)
{
	// TODO: implement me
	fStatus = B_NO_INIT;
}


BNetworkAddress::BNetworkAddress()
{
	Unset();
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
BNetworkAddress::SetTo(int family, const char* host, uint16 port)
{
	// Check if the address contains a port

	BString hostAddress(host);

	BString portString;
	if (strip_port(hostAddress, portString) && port == 0)
		port = strtoul(portString.String(), NULL, 0);

	// Resolve address

	memset(&fAddress, 0, sizeof(sockaddr_storage));
	fAddress.ss_family = family;

	if (host != NULL) {
		switch (family) {
			case AF_INET:
			{
				hostent* server = gethostbyname(hostAddress.String());
				if (server == NULL)
					return errno;

				struct sockaddr_in& address = (sockaddr_in&)fAddress;
				address.sin_port = htons(port);
				address.sin_addr.s_addr = *(in_addr_t*)server->h_addr_list[0];
				break;
			}
			
			default:
			{
				fStatus = resolve_address(family, host, "0", 0, fAddress);
				if (fStatus != B_OK)
					return fStatus;

				if (family == AF_INET6) {
					struct sockaddr_in6& address = (sockaddr_in6&)fAddress;
					address.sin6_port = htons(port);
				}
				break;
			}
		}
	} else {
		switch (fAddress.ss_family) {
			case AF_INET:
			{
				struct sockaddr_in& address = (sockaddr_in&)fAddress;
				address.sin_port = htons(port);
				address.sin_addr.s_addr = INADDR_ANY;
				break;
			}

			case AF_INET6:
			{
				struct sockaddr_in6& address = (sockaddr_in6&)fAddress;
				address.sin6_port = htons(port);
				address.sin6_addr = in6addr_any;
				break;
			}

			default:
				return B_NOT_SUPPORTED;
		}
	}

	return fStatus = B_OK;
}


status_t
BNetworkAddress::SetTo(const char* host, uint16 port)
{
	// Check if the address contains a port

	BString hostAddress(host);

	BString portString;
	strip_port(hostAddress, portString);

	// Resolve address

	memset(&fAddress, 0, sizeof(sockaddr_storage));

	fStatus = resolve_address(AF_UNSPEC,
		host != NULL ? hostAddress.String() : NULL,
		portString.Length() == 0 ? NULL : portString.String(), 0, fAddress);
	if (fStatus != B_OK)
		return fStatus;

	// Set port if specified separately

	switch (fAddress.ss_family) {
		case AF_INET:
		{
			struct sockaddr_in& address = (sockaddr_in&)fAddress;
			if (address.sin_port == 0)
				address.sin_port = htons(port);
			break;
		}

		case AF_INET6:
		{
			struct sockaddr_in6& address = (sockaddr_in6&)fAddress;
			if (address.sin6_port == 0)
				address.sin6_port = htons(port);
			break;
		}

		default:
			break;
	}

	return B_OK;
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
			length = sizeof(sockaddr_dl);
			break;
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
BNetworkAddress::SetTo(const in_addr_t inetAddress)
{
	sockaddr_in& address = (sockaddr_in&)fAddress;
	memset(&fAddress, 0, sizeof(sockaddr_storage));

	address.sin_family = AF_INET;
	address.sin_len = sizeof(sockaddr_in);
	address.sin_addr.s_addr = inetAddress;
	fStatus = B_OK;
}


void
BNetworkAddress::SetTo(const in6_addr* inet6Address)
{
	sockaddr_in6& address = (sockaddr_in6&)fAddress;
	memset(&fAddress, 0, sizeof(sockaddr_storage));

	address.sin6_family = AF_INET6;
	address.sin6_len = sizeof(sockaddr_in6);
	memcpy(address.sin6_addr.s6_addr, inet6Address,
		sizeof(address.sin6_addr.s6_addr));
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

	memset(&fAddress, 0, sizeof(sockaddr_storage));
	fAddress.ss_family = family;
	((sockaddr_in&)fAddress).sin_addr.s_addr = INADDR_BROADCAST;

	SetPort(port);
	return fStatus = B_OK;
}


status_t
BNetworkAddress::SetToLocal()
{
	// TODO: choose a local address from the network interfaces
	return B_NOT_SUPPORTED;
}


status_t
BNetworkAddress::SetToLoopback()
{
	return SetTo("localhost");
}


status_t
BNetworkAddress::SetToMask(int family, uint32 prefixLength)
{
	// TODO: implement SetToMask()
	return B_NOT_SUPPORTED;
}


status_t
BNetworkAddress::SetToWildcard(int family)
{
	return SetTo(family, NULL);
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
	// TODO: implement me!
}


void
BNetworkAddress::SetToLinkLevel(const char* name)
{
	// TODO: implement me!
}


void
BNetworkAddress::SetToLinkLevel(uint32 index)
{
	// TODO: implement me!
}


void
BNetworkAddress::SetLinkLevelIndex(uint32 index)
{
	// TODO: implement me!
}


void
BNetworkAddress::SetLinkLevelType(uint32 type)
{
	// TODO: implement me!
}


void
BNetworkAddress::SetLinkLevelFrameType(uint32 frameType)
{
	// TODO: implement me!
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


uint32
BNetworkAddress::LinkLevelType() const
{
	return ((sockaddr_dl&)fAddress).sdl_type;
}


uint32
BNetworkAddress::LinkLevelFrameType() const
{
	return ((sockaddr_dl&)fAddress).sdl_e_type;
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
	char buffer[128];

	switch (fAddress.ss_family) {
		case AF_INET:
			inet_ntop(AF_INET, &((sockaddr_in&)fAddress).sin_addr, buffer,
				sizeof(buffer));
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &((sockaddr_in6&)fAddress).sin6_addr,
				buffer, sizeof(buffer));
			break;

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
BNetworkAddress::PortName() const
{
	// TODO: implement service lookup
	BString portName;
	portName << Port();
	return portName;
}


status_t
BNetworkAddress::Archive(BMessage* into, bool deep) const
{
	// TODO: implement me!
	return B_ERROR;
}


/*static*/ BArchivable*
BNetworkAddress::Instantiate(BMessage* archive)
{
	if (archive != NULL)
		return new BNetworkAddress(archive);

	return NULL;
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
	}

	if (compare < 0)
		return true;
	if (compare > 0)
		return false;

	return Port() < other.Port();
}
