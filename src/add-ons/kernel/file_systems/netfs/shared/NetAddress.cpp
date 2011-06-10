// NetAddress.cpp

#include <new>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#if defined(HAIKU_TARGET_PLATFORM_DANO) || defined(HAIKU_TARGET_PLATFORM_DANO)
#	include <net/route.h>
#	include <sys/sockio.h>
#	include <stdlib.h>
#endif

#include <AutoLocker.h>
#include <ByteOrder.h>
#include <HashString.h>
#include <Referenceable.h>

#include "Compatibility.h"
#include "Locker.h"
#include "NetAddress.h"

// constructor
NetAddress::NetAddress()
{
	fAddress.sin_family = AF_INET;
	fAddress.sin_addr.s_addr = 0;
	fAddress.sin_port = 0;
}

// constructor
NetAddress::NetAddress(const sockaddr_in& address)
{
	fAddress = address;
}

// copy constructor
NetAddress::NetAddress(const NetAddress& address)
{
	fAddress = address.fAddress;
}

// SetIP
void
NetAddress::SetIP(int32 address)
{
	fAddress.sin_addr.s_addr = B_HOST_TO_BENDIAN_INT32(address);
}

// GetIP
int32
NetAddress::GetIP() const
{
	return B_BENDIAN_TO_HOST_INT32(fAddress.sin_addr.s_addr);
}

// SetPort
void
NetAddress::SetPort(uint16 port)
{
	fAddress.sin_port = B_HOST_TO_BENDIAN_INT32(port);
}

// GetPort
uint16
NetAddress::GetPort() const
{
	return B_BENDIAN_TO_HOST_INT16(fAddress.sin_port);
}

// SetAddress
void
NetAddress::SetAddress(const sockaddr_in& address)
{
	fAddress = address;
}

// GetAddress
const sockaddr_in&
NetAddress::GetAddress() const
{
	return fAddress;
}

// IsLocal
bool
NetAddress::IsLocal() const
{
	// special address?
	if (fAddress.sin_addr.s_addr == INADDR_ANY
		|| fAddress.sin_addr.s_addr == INADDR_BROADCAST) {
		return false;
	}
	// create a socket and try to bind it to a port of this address
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return false;
#if defined(HAIKU_TARGET_PLATFORM_DANO)
	// BONE does allow you to bind to any address!
	// Therefore, we iterate over all routes, and see if there are any local
	// ones for this address.

	bool result = false;
	uint32 count;
	if (ioctl(fd, SIOCGRTSIZE, &count) == 0) {
		route_req_t* routes = (route_req_t*)malloc(count * sizeof(route_req_t));
		if (routes != NULL) {
			route_table_req table;
			table.rrtp = routes;
			table.len = count * sizeof(route_req_t);
			table.cnt = count;
			if (ioctl(fd, SIOCGRTTABLE, &table) == 0) {
				for (uint32 i = 0; i < table.cnt; i++) {
					if ((routes[i].flags & RTF_LOCAL) == 0)
						continue;
					if (((sockaddr_in*)&routes[i].dst)->sin_addr.s_addr
							== fAddress.sin_addr.s_addr) {
						result = true;
						break;
					}
				}
			}
			free(routes);
		}
	}
#else
	// bind it to a port
	sockaddr_in addr = fAddress;
	addr.sin_port = 0;
	bool result = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
#endif
	closesocket(fd);

	return result;
}


// GetString
status_t
NetAddress::GetString(HashString* string, bool includePort) const
{
	if (!string)
		return B_BAD_VALUE;
	char buffer[32];
	uint32 ip = GetIP();
	if (includePort) {
		sprintf(buffer, "%lu.%lu.%lu.%lu:%hu",
			ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff,
			GetPort());
	} else {
		sprintf(buffer, "%lu.%lu.%lu.%lu",
			ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
	}
	return (string->SetTo(buffer) ? B_OK : B_NO_MEMORY);
}

// GetHashCode
uint32
NetAddress::GetHashCode() const
{
	return (fAddress.sin_addr.s_addr * 31 + fAddress.sin_port);
}

// =
NetAddress&
NetAddress::operator=(const NetAddress& address)
{
	fAddress = address.fAddress;
	return *this;
}

// ==
bool
NetAddress::operator==(const NetAddress& address) const
{
	return (fAddress.sin_addr.s_addr == address.fAddress.sin_addr.s_addr
		&& fAddress.sin_port == address.fAddress.sin_port);
}

// !=
bool
NetAddress::operator!=(const NetAddress& address) const
{
	return !(*this == address);
}


// #pragma mark -

// Resolver
class NetAddressResolver::Resolver : public BReferenceable {
public:
	Resolver()
		: BReferenceable(),
		  fLock()
	{
	}

	status_t InitCheck() const
	{
		return (fLock.Sem() >= 0 ? B_OK : B_NO_INIT);
	}

	status_t GetHostAddress(const char* hostName, NetAddress* address)
	{
		AutoLocker<Locker> _(fLock);
		struct hostent* host = gethostbyname(hostName);
		if (!host)
			return h_errno;
		if (host->h_addrtype != AF_INET || !host->h_addr_list[0])
			return B_BAD_VALUE;
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr = *(in_addr*)host->h_addr_list[0];
		*address = addr;
		return B_OK;
	}

protected:
	virtual void LastReferenceReleased()
	{
		// don't delete
	}

private:
	Locker	fLock;
};

// constructor
NetAddressResolver::NetAddressResolver()
{
	_Lock();
	// initialize static instance, if not done yet
	if (sResolver) {
		sResolver->AcquireReference();
		fResolver = sResolver;
	} else {
		sResolver = new(std::nothrow) Resolver;
		if (sResolver) {
			if (sResolver->InitCheck() != B_OK) {
				delete sResolver;
				sResolver = NULL;
			}
		}
		fResolver = sResolver;
	}
	_Unlock();
}

// destructor
NetAddressResolver::~NetAddressResolver()
{
	if (fResolver) {
		_Lock();
		if (sResolver->ReleaseReference() == 1) {
			delete sResolver;
			sResolver = NULL;
		}
		_Unlock();
	}
}

// InitCheck
status_t
NetAddressResolver::InitCheck() const
{
	return (fResolver ? B_OK : B_NO_INIT);
}

// GetAddress
status_t
NetAddressResolver::GetHostAddress(const char* hostName, NetAddress* address)
{
	if (!fResolver)
		return B_NO_INIT;
	if (!hostName || !address)
		return B_BAD_VALUE;
	return fResolver->GetHostAddress(hostName, address);
}

// _Lock
void
NetAddressResolver::_Lock()
{
	while (atomic_add(&sLockCounter, 1) > 0) {
		atomic_add(&sLockCounter, -1);
		snooze(10000);
	}
}

// _Unlock
void
NetAddressResolver::_Unlock()
{
	atomic_add(&sLockCounter, -1);
}


// sResolver
NetAddressResolver::Resolver* volatile NetAddressResolver::sResolver = NULL;

// sLockCounter
vint32 NetAddressResolver::sLockCounter = 0;
