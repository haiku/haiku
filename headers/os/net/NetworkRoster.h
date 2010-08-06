/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ROSTER_H
#define _NETWORK_ROSTER_H


#include <SupportDefs.h>


class BMessenger;
class BNetworkInterface;


class BNetworkRoster {
public:
			BNetworkRoster&		Default();

			uint32				CountInterfaces();
			BNetworkInterface*	InterfaceAt(uint32 index);
			
			status_t			AddInterface(
									const BNetworkInterface& interface);
			status_t			RemoveInterface(
									const BNetworkInterface& interface);
			status_t			RemoveInterfaceAt(uint32 index);

			status_t			StartWatching(const BMessenger& target,
									uint32 eventMask);
			void				StopWatching(const BMessenger& target);

private:
								BNetworkRoster();
								~BNetworkRoster();
};


#endif	// _NETWORK_ROSTER_H
