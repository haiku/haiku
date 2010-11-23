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

			status_t			SetTo(const BNetworkInterface& interface,
									int32 index);

			void				SetAddress(const BNetworkAddress& address);
			void				SetMask(const BNetworkAddress& mask);
			void				SetBroadcast(const BNetworkAddress& broadcast);
			void				SetDestination(
									const BNetworkAddress& destination);

			BNetworkAddress&	Address() { return fAddress; }
			BNetworkAddress&	Mask() { return fMask; }
			BNetworkAddress&	Broadcast() { return fBroadcast; }
			BNetworkAddress&	Destination() { return fBroadcast; }

			const BNetworkAddress& Address() const { return fAddress; }
			const BNetworkAddress& Mask() const { return fMask; }
			const BNetworkAddress& Broadcast() const { return fBroadcast; }
			const BNetworkAddress& Destination() const { return fBroadcast; }

			void				SetFlags(uint32 flags);
			uint32				Flags() const { return fFlags; }

			int32				Index() const { return fIndex; }

private:
			int32				fIndex;
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

			void				Unset();
			void				SetTo(const char* name);
			status_t			SetTo(uint32 index);

			bool				Exists() const;

			const char*			Name() const;
			uint32				Index() const;
			uint32				Flags() const;
			uint32				MTU() const;
			int32				Media() const;
			uint32				Metric() const;
			uint32				Type() const;
			status_t			GetStats(ifreq_stats& stats);
			bool				HasLink() const;

			status_t			SetFlags(uint32 flags);
			status_t			SetMTU(uint32 mtu);
			status_t			SetMedia(int32 media);
			status_t			SetMetric(uint32 metric);

			int32				CountAddresses() const;
			status_t			GetAddressAt(int32 index,
									BNetworkInterfaceAddress& address);
			int32				FindAddress(const BNetworkAddress& address);
			int32				FindFirstAddress(int family);

			status_t			AddAddress(
									const BNetworkInterfaceAddress& address);
			status_t			AddAddress(const BNetworkAddress& address);
			status_t			SetAddress(
									const BNetworkInterfaceAddress& address);
			status_t			RemoveAddress(
									const BNetworkInterfaceAddress& address);
			status_t			RemoveAddress(const BNetworkAddress& address);
			status_t			RemoveAddressAt(int32 index);

			status_t			GetHardwareAddress(BNetworkAddress& address);

			status_t			AddRoute(const route_entry& route);
			status_t			AddDefaultRoute(const BNetworkAddress& gateway);
			status_t			RemoveRoute(const route_entry& route);
			status_t			RemoveRoute(int family,
									const route_entry& route);
			status_t			RemoveDefaultRoute(int family);

			status_t			AutoConfigure(int family);

private:
			char				fName[IF_NAMESIZE];
			BList				fAddresses;
};


#endif	// _NETWORK_INTERFACE_H
