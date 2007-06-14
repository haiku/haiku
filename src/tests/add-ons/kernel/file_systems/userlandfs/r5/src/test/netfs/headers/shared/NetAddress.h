// NetAddress.h

#ifndef NET_FS_NET_ADDRESS_H
#define NET_FS_NET_ADDRESS_H

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

namespace UserlandFSUtil {
class String;
};
using UserlandFSUtil::String;

// NetAddress
class NetAddress {
public:
								NetAddress();
								NetAddress(const sockaddr_in& address);
								NetAddress(const NetAddress& address);

			void				SetIP(int32 address);
			int32				GetIP() const;

			void				SetPort(uint16 port);
			uint16				GetPort() const;

			void				SetAddress(const sockaddr_in& address);
			const sockaddr_in&	GetAddress() const;

			bool				IsLocal() const;

			status_t			GetString(String* string,
									bool includePort = true) const;

			uint32				GetHashCode() const;

			NetAddress&			operator=(const NetAddress& address);
			bool				operator==(const NetAddress& address) const;
			bool				operator!=(const NetAddress& address) const;

private:
			sockaddr_in			fAddress;
};

// NetAddressResolver
class NetAddressResolver {
public:
								NetAddressResolver();
								~NetAddressResolver();

			status_t			InitCheck() const;

			status_t			GetHostAddress(const char* hostName,
									NetAddress* address);

private:
			class Resolver;

	static	void				_Lock();
	static	void				_Unlock();

			Resolver*			fResolver;

	static	Resolver* volatile	sResolver;
	static	vint32				sLockCounter;
};

#endif	// NET_FS_NET_ADDRESS_H
