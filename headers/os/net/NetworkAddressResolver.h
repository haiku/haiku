/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ADDRESS_RESOLVER_H
#define _NETWORK_ADDRESS_RESOLVER_H


#include <ObjectList.h>
#include <Referenceable.h>
#include <String.h>
#include <SupportDefs.h>


class BNetworkAddress;
struct addrinfo;


// flags for name resolution
enum {
	B_NO_ADDRESS_RESOLUTION			= 0x0001,
	B_UNCONFIGURED_ADDRESS_FAMILIES	= 0x0002,
};


class BNetworkAddressResolver: public BReferenceable {
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

	// TODO all the ::Resolve variants are needed. Maybe the SetTo and
	// constructors could be removed as ::Resolve is the right way to get a
	// resolver (using the cache).
	static	BReference<const BNetworkAddressResolver> Resolve(
									const char* address, const char* service,
									uint32 flags = 0);
	static	BReference<const BNetworkAddressResolver> Resolve(
									const char* address, uint16 port = 0,
									uint32 flags = 0);
	static	BReference<const BNetworkAddressResolver> Resolve(int family,
									const char* address, const char* service,
									uint32 flags = 0);
	static	BReference<const BNetworkAddressResolver> Resolve(int family,
									const char* address, uint16 port = 0,
									uint32 flags = 0);

private:
			addrinfo*			fInfo;
			status_t			fStatus;


	struct CacheEntry {
		CacheEntry(int family, const char* address, const char* service,
			uint32 flags, BNetworkAddressResolver* resolver)
			:
			fFamily(family),
			fAddress(address),
			fService(service),
			fFlags(flags),
			fResolver(resolver, false)
		{
		}

		bool Matches(int family, BString address, BString service, uint32 flags)
		{
			return family == fFamily && flags == fFlags && address == fAddress
				&& service == fService;
		}

		int fFamily;
		BString fAddress;
		BString fService;
		uint32 fFlags;

		BReference<const BNetworkAddressResolver> fResolver;
	};

	static	BObjectList<CacheEntry> sCacheMap;
};


#endif	// _NETWORK_ADDRESS_RESOLVER_H
