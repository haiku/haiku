/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include <Messenger.h>
#include <NodeMonitor.h>

#include <MessengerPrivate.h>

#include <syscalls.h>

#include "node_monitor_private.h"


// TODO: Tests!


// Subscribes a target to watch node changes on a volume.
status_t
watch_volume(dev_t volume, uint32 flags, BMessenger target)
{
	if ((flags & (B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR)) == 0)
		return B_BAD_VALUE;

	flags |= B_WATCH_VOLUME;

	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();
	return _kern_start_watching(volume, (ino_t)-1, flags, port, token);
}


status_t
watch_volume(dev_t volume, uint32 flags, const BHandler *handler,
	const BLooper *looper)
{
	return watch_volume(volume, flags, BMessenger(handler, looper));
}


// Subscribes or unsubscribes a target to node and/or mount watching.
status_t
watch_node(const node_ref *node, uint32 flags, BMessenger target)
{
	if (!target.IsValid())
		return B_BAD_VALUE;

	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();

	if (flags == B_STOP_WATCHING) {
		// unsubscribe from node node watching
		if (node == NULL)
			return B_BAD_VALUE;

		return _kern_stop_watching(node->device, node->node, port, token);
	}

	// subscribe to...
	// mount watching
	if (flags & B_WATCH_MOUNT) {
		status_t status = _kern_start_watching((dev_t)-1, (ino_t)-1,
			B_WATCH_MOUNT, port, token);
		if (status < B_OK)
			return status;

		flags &= ~B_WATCH_MOUNT;
	}

	// node watching
	if (flags != 0) {
		if (node == NULL)
			return B_BAD_VALUE;

		return _kern_start_watching(node->device, node->node, flags, port,
			token);
	}

	return B_OK;
}


// Subscribes or unsubscribes a handler or looper to node and/or mount
// watching.
status_t
watch_node(const node_ref *node, uint32 flags, const BHandler *handler,
	const BLooper *looper)
{
	return watch_node(node, flags, BMessenger(handler, looper));
}


// Unsubscribes a target from node and mount monitoring.
status_t
stop_watching(BMessenger target)
{
	if (!target.IsValid())
		return B_BAD_VALUE;

	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();

	return _kern_stop_notifying(port, token);
}


// Unsubscribes a target from node and mount monitoring.
status_t
stop_watching(const BHandler *handler, const BLooper *looper)
{
	return stop_watching(BMessenger(handler, looper));
}

