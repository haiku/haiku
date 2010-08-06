/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_INTERFACE_H
#define _NETWORK_INTERFACE_H


#include <net/if.h>
#include <net/if_types.h>

#include <List.h>
#include <NetworkAddress.h>


class BNetworkInterface;


class BNetworkInterfaceAddress {
public:
								BNetworkInterfaceAddress();
								~BNetworkInterfaceAddress();

			void				SetAddress(BNetworkAddress& address);
			void				SetMask(BNetworkAddress& mask);
			void				SetBroadcast(BNetworkAddress& mask);

			BNetworkAddress&	Address() { return fAddress; }
			BNetworkAddress&	Mask() { return fMask; }
			BNetworkAddress&	Broadcast() { return fBroadcast; }

			const BNetworkAddress& Address() const { return fAddress; }
			const BNetworkAddress& Mask() const { return fMask; }
			const BNetworkAddress& Broadcast() const { return fBroadcast; }

			void				SetFlags(uint32 flags);
			uint32				Flags() const { return fFlags; }

private:
			BNetworkInterface*	fInterface;
			uint32				fIndex;
			BNetworkAddress		fAddress;
			BNetworkAddress		fMask;
			BNetworkAddress		fBroadcast;
			uint32				fFlags;
};


class BNetworkInterface {
public:
								BNetworkInterface();
								BNetworkInterface(const char* name);
								BNetworkInterface(uint32 index);
								~BNetworkInterface();

			bool				Exists() const;

			const char*			Name() const;
			uint32				Flags() const;
			uint32				MTU() const;
			uint32				Type() const;
			status_t			GetStats(ifreq_stats& stats);
			bool				HasLink() const;

			status_t			SetFlags(uint32 flags);
			status_t			SetMTU(uint32 mtu);

			int32				CountAddresses() const;
			BNetworkInterfaceAddress* AddressAt(int32 index);

			status_t			AddAddress(
									const BNetworkInterfaceAddress& address);
			status_t			RemoveAddress(
									const BNetworkInterfaceAddress& address);
			status_t			RemoveAddressAt(int32 index);

			status_t			GetHardwareAddress(BNetworkAddress& address);

private:
			char				fName[IF_NAMESIZE];
			uint32				fIndex;
			BList				fAddresses;
};


#endif	// _NETWORK_INTERFACE_H
