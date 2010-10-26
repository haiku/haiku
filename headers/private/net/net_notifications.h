/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_NOTIFICATIONS_H
#define NET_NOTIFICATIONS_H


#include <module.h>
#include <NetworkNotifications.h>


#define NET_NOTIFICATIONS_MODULE_NAME "network/notifications/v1"

namespace BPrivate {
	class KMessage;
};

struct net_notifications_module_info {
	module_info info;

	status_t (*send_notification)(const BPrivate::KMessage* event);
};

// generic syscall interface
#define NET_NOTIFICATIONS_SYSCALLS "network/notifications"

#define NET_NOTIFICATIONS_CONTROL_WATCHING	1

struct net_notifications_control {
	uint32		flags;
	port_id		port;
	uint32		token;
};


#endif	// NET_NOTIFICATIONS_H
