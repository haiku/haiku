/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*! Provides the stack internal notification API.

	The actual message sending happens in another module to make the
	notification listeners independent from the stack status.
*/


#include <net_device.h>
#include <net_notifications.h>

#include <util/KMessage.h>

#include "stack_private.h"


static net_notifications_module_info* sNotificationModule;


status_t
notify_interface_added(net_interface* interface)
{
	if (sNotificationModule == NULL)
		return B_NOT_SUPPORTED;

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
	message.AddInt32("opcode", B_NETWORK_INTERFACE_ADDED);
	message.AddString("interface", interface->name);

	return sNotificationModule->send_notification(&message);
}


status_t
notify_interface_removed(net_interface* interface)
{
	if (sNotificationModule == NULL)
		return B_NOT_SUPPORTED;

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
	message.AddInt32("opcode", B_NETWORK_INTERFACE_REMOVED);
	message.AddString("interface", interface->name);

	return sNotificationModule->send_notification(&message);
}


status_t
notify_interface_changed(net_interface* interface, uint32 oldFlags,
	uint32 newFlags)
{
	if (sNotificationModule == NULL)
		return B_NOT_SUPPORTED;

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
	message.AddInt32("opcode", B_NETWORK_INTERFACE_CHANGED);
	message.AddString("interface", interface->name);
	if (oldFlags != newFlags) {
		message.AddInt32("old flags", oldFlags);
		message.AddInt32("new flags", newFlags);
	}

	return sNotificationModule->send_notification(&message);
}


status_t
notify_link_changed(net_device* device)
{
	if (sNotificationModule == NULL)
		return B_NOT_SUPPORTED;

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NETWORK_MONITOR);
	message.AddInt32("opcode", B_NETWORK_DEVICE_LINK_CHANGED);
	message.AddString("device", device->name);
	message.AddInt32("media", device->media);
	message.AddInt64("link speed", device->link_speed);
	message.AddInt32("link quality", device->link_quality);

	return sNotificationModule->send_notification(&message);
}


status_t
init_notifications()
{
	return get_module(NET_NOTIFICATIONS_MODULE_NAME,
		(module_info**)&sNotificationModule);
}


void
uninit_notifications()
{
	if (sNotificationModule != NULL)
		put_module(NET_NOTIFICATIONS_MODULE_NAME);
}
