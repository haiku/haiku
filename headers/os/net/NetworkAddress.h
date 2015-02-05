/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ADDRESS_H
#define _NETWORK_ADDRESS_H


#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet6/in6.h>
#include <sys/socket.h>

#include <Archivable.h>
#include <NetworkAddressResolver.h>
#include <String.h>


class BNetworkAddress : public BFlattenable {
public:
								BNetworkAddress();
								BNetworkAddress(const char* address,
									uint16 port = 0, uint32 flags = 0);
								BNetworkAddress(const char* address,
									const char* service, uint32 flags = 0);
								BNetworkAddress(int family, const char* address,
									uint16 port = 0, uint32 flags = 0);
								BNetworkAddress(int family, const char* address,
									const char* service, uint32 flags = 0);
								BNetworkAddress(const sockaddr& address);
								BNetworkAddress(
									const sockaddr_storage& address);
								BNetworkAddress(const sockaddr_in& address);
								BNetworkAddress(const sockaddr_in6& address);
								BNetworkAddress(const sockaddr_dl& address);
								BNetworkAddress(in_addr_t address,
									uint16 port = 0);
								BNetworkAddress(const in6_addr& address,
									uint16 port = 0);
								BNetworkAddress(const BNetworkAddress& other);
	virtual						~BNetworkAddress();

			status_t			InitCheck() const;

			void				Unset();

			status_t			SetTo(const char* address, uint16 port = 0,
									uint32 flags = 0);
			status_t			SetTo(const char* address, const char* service,
									uint32 flags = 0);
			status_t			SetTo(int family, const char* address,
									uint16 port = 0, uint32 flags = 0);
			status_t			SetTo(int family, const char* address,
									const char* service, uint32 flags = 0);
			void				SetTo(const sockaddr& address);
			void				SetTo(const sockaddr& address, size_t length);
			void				SetTo(const sockaddr_storage& address);
			void				SetTo(const sockaddr_in& address);
			void				SetTo(const sockaddr_in6& address);
			void				SetTo(const sockaddr_dl& address);
			void				SetTo(in_addr_t address, uint16 port = 0);
			void				SetTo(const in6_addr& address, uint16 port = 0);
			void				SetTo(const BNetworkAddress& other);

			status_t			SetToBroadcast(int family, uint16 port = 0);
			status_t			SetToLocal(int family = AF_UNSPEC,
									uint16 port = 0);
			status_t			SetToLoopback(int family = AF_UNSPEC,
									uint16 port = 0);
			status_t			SetToMask(int family, uint32 prefixLength);
			status_t			SetToWildcard(int family, uint16 port = 0);

			status_t			SetAddress(in_addr_t address);
			status_t			SetAddress(const in6_addr& address);
			void				SetPort(uint16 port);
			status_t			SetPort(const char* service);

			void				SetToLinkLevel(uint8* address, size_t length);
			void				SetToLinkLevel(const char* name);
			void				SetToLinkLevel(uint32 index);
			void				SetLinkLevelIndex(uint32 index);
			void				SetLinkLevelType(uint8 type);
			void				SetLinkLevelFrameType(uint16 frameType);

			int					Family() const;
			uint16				Port() const;
			size_t				Length() const;
			const sockaddr&		SockAddr() const;
			sockaddr&			SockAddr();

			bool				IsEmpty() const;
			bool				IsWildcard() const;
			bool				IsBroadcast() const;
			bool				IsMulticast() const;
			bool				IsMulticastGlobal() const;
			bool				IsMulticastNodeLocal() const;
			bool				IsMulticastLinkLocal() const;
			bool				IsMulticastSiteLocal() const;
			bool				IsMulticastOrgLocal() const;
			bool				IsLinkLocal() const;
			bool				IsSiteLocal() const;
			bool				IsLocal() const;

			ssize_t				PrefixLength() const;

			uint32				LinkLevelIndex() const;
			BString				LinkLevelInterface() const;
			uint8				LinkLevelType() const;
			uint16				LinkLevelFrameType() const;
			uint8*				LinkLevelAddress() const;
			size_t				LinkLevelAddressLength() const;

			status_t			ResolveForDestination(
									const BNetworkAddress& destination);
			status_t			ResolveTo(const BNetworkAddress& address);

			BString				ToString(bool includePort = true) const;
			BString				HostName() const;
			BString				ServiceName() const;

			bool				Equals(const BNetworkAddress& other,
									bool includePort = true) const;

	// BFlattenable implementation
	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;

	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

			BNetworkAddress&	operator=(const BNetworkAddress& other);

			bool				operator==(const BNetworkAddress& other) const;
			bool				operator!=(const BNetworkAddress& other) const;
			bool				operator<(const BNetworkAddress& other) const;

								operator const sockaddr*() const;
								operator const sockaddr&() const;
								operator const sockaddr*();
								operator sockaddr*();
								operator const sockaddr&();
								operator sockaddr&();

private:
			status_t			_ParseLinkAddress(const char* address);

private:
			sockaddr_storage	fAddress;
			status_t			fStatus;
};


#endif	// _NETWORK_ADDRESS_H
