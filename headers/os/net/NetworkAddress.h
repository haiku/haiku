/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ADDRESS_H
#define _NETWORK_ADDRESS_H


#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet6/in6.h>
#include <sys/socket.h>

#include <Archivable.h>


class BNetworkAddress : public BArchivable {
public:
								BNetworkAddress();
								BNetworkAddress(int family,
									const char* address, uint16 port = 0);
								BNetworkAddress(const char* address,
									uint16 port = 0);
								BNetworkAddress(const sockaddr* address);
								BNetworkAddress(const sockaddr_in* address);
								BNetworkAddress(const sockaddr_in6* address);
								BNetworkAddress(const sockaddr_dl* address);
								BNetworkAddress(const in_addr_t address);
								BNetworkAddress(const in6_addr* address);
								BNetworkAddress(const BNetworkAddress& other);
								BNetworkAddress(BMessage* archive);
	virtual						~BNetworkAddress();

			status_t			InitCheck() const;

			void				Unset();

			status_t			SetTo(int family, const char* address,
									uint16 port = 0);
			status_t			SetTo(const char* address, uint16 port = 0);
			void				SetTo(const sockaddr* address);
			void				SetTo(const sockaddr_in* address);
			void				SetTo(const sockaddr_in6* address);
			void				SetTo(const sockaddr_dl* address);
			void				SetTo(const in_addr_t address);
			void				SetTo(const in6_addr* address);
			void				SetTo(const BNetworkAddress& other);

			status_t			SetToBroadcast(int family, uint16 port = 0);
			status_t			SetToLocal();
			status_t			SetToLoopback();
			status_t			SetToMask(int family, uint32 prefixLength);
			status_t			SetToWildcard(int family);
			void				SetPort(uint16 port);

			void				SetToLinkLevel(uint8* address, size_t length);
			void				SetToLinkLevel(const char* name);
			void				SetToLinkLevel(uint32 index);
			void				SetLinkLevelIndex(uint32 index);
			void				SetLinkLevelType(uint32 type);
			void				SetLinkLevelFrameType(uint32 frameType);

			int					Family() const;
			uint16				Port() const;
			size_t				Length() const;

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
			
			uint32				LinkLevelIndex() const;
			BString				LinkLevelInterface() const;
			uint32				LinkLevelType() const;
			uint32				LinkLevelFrameType() const;
			uint8*				LinkLevelAddress() const;
			size_t				LinkLevelAddressLength() const;

			BNetworkAddress		ResolvedForDestination(
									const BNetworkAddress& destination) const;
			void				ResolveTo(const BNetworkAddress& address);

			BString				ToString() const;
			BString				HostName() const;
			BString				PortName() const;

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

			BNetworkAddress&	operator=(const BNetworkAddress& other);

			bool				operator==(const BNetworkAddress& other) const;
			bool				operator!=(const BNetworkAddress& other) const;
			bool				operator<(const BNetworkAddress& other) const;

								operator sockaddr*() const;
								operator sockaddr&() const;

private:
			sockaddr_storage	fAddress;
			status_t			fStatus;
};


#endif	// _NETWORK_ADDRESS_H
