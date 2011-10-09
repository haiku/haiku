/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ROSTER_H
#define _NETWORK_ROSTER_H


#include <Locker.h>
#include <NetworkNotifications.h>


class BMessenger;
class BNetworkInterface;
struct wireless_network;


class BNetworkRoster {
public:
	static	BNetworkRoster&		Default();

			size_t				CountInterfaces() const;
			status_t			GetNextInterface(uint32* cookie,
									BNetworkInterface& interface) const;

			status_t			AddInterface(const char* name);
			status_t			AddInterface(
									const BNetworkInterface& interface);
			status_t			RemoveInterface(const char* name);
			status_t			RemoveInterface(
									const BNetworkInterface& interface);

			int32				CountPersistentNetworks() const;
			status_t			GetNextPersistentNetwork(uint32* cookie,
									wireless_network& network) const;
			status_t			AddPersistentNetwork(
									const wireless_network& network);
			status_t			RemovePersistentNetwork(const char* name);

			status_t			StartWatching(const BMessenger& target,
									uint32 eventMask);
			void				StopWatching(const BMessenger& target);

private:
								BNetworkRoster();
								~BNetworkRoster();

private:
	static	BNetworkRoster		sDefault;
};


#endif	// _NETWORK_ROSTER_H
