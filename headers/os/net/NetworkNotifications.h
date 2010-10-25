/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_NOTIFICATIONS_H
#define _NETWORK_NOTIFICATIONS_H


#include <SupportDefs.h>


class BHandler;
class BLooper;
class BMessenger;


#define B_NETWORK_INTERFACE_ADDED				0x0001
#define B_NETWORK_INTERFACE_REMOVED				0x0002
#define B_NETWORK_INTERFACE_CHANGED				0x0003
#define B_NETWORK_DEVICE_LINK_CHANGED			0x0010
#define B_NETWORK_WLAN_JOINED					0x0100
#define B_NETWORK_WLAN_LEFT						0x0200
#define B_NETWORK_WLAN_SCANNED					0x0300
#define B_NETWORK_WLAN_MESSAGE_INTEGRITY_FAILED	0x0400

// TODO: add routes, stack unloaded/loaded, ... events

enum {
	B_WATCH_NETWORK_INTERFACE_CHANGES	= 0x000f,
	B_WATCH_NETWORK_LINK_CHANGES	 	= 0x00f0,
	B_WATCH_NETWORK_WLAN_CHANGES		= 0x0f00
};

#define B_NETWORK_MONITOR				'_NTN'


#ifdef __cplusplus

status_t	start_watching_network(uint32 flags, const BMessenger& target);
status_t	start_watching_network(uint32 flags, const BHandler* handler,
				const BLooper* looper = NULL);

status_t	stop_watching_network(const BMessenger& target);
status_t	stop_watching_network(const BHandler* handler,
				const BLooper* looper = NULL);

#endif	// __cplusplus


#endif	// _NETWORK_NOTIFICATIONS_H
