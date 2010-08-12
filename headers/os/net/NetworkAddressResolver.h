/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ADDRESS_RESOLVER_H
#define _NETWORK_ADDRESS_RESOLVER_H


#include <SupportDefs.h>


class BNetworkAddress;
struct addrinfo;


// flags for name resolution
enum {
	B_NO_ADDRESS_RESOLUTION			= 0x0001,
	B_UNCONFIGURED_ADDRESS_FAMILIES	= 0x0002,
};


class BNetworkAddressResolver {
public:
								BNetworkAddressResolver();
								BNetworkAddressResolver(const char* address,
									uint16 port = 0, uint32 flags = 0);
								BNetworkAddressResolver(const char* address,
									const char* service, uint32 flags = 0);
								BNetworkAddressResolver(int family,
									const char* address, uint16 port = 0,
									uint32 flags = 0);
								BNetworkAddressResolver(int family,
									const char* address, const char* service,
									uint32 flags = 0);
								~BNetworkAddressResolver();

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

			status_t			GetNextAddress(uint32* cookie,
									BNetworkAddress& address) const;
			status_t			GetNextAddress(int family, uint32* cookie,
									BNetworkAddress& address) const;

private:
			addrinfo*			fInfo;
			status_t			fStatus;
};


#endif	// _NETWORK_ADDRESS_RESOLVER_H
