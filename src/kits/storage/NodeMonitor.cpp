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


// watch_volume
/*!	\brief Subscribes a target to watch node changes on a volume.

	Depending of \a flags the action performed by this function varies:
	- \a flags contains at least one of \c B_WATCH_NAME, \c B_WATCH_STAT,
	  or \c B_WATCH_ATTR: The target is subscribed to
	  watching the specified aspects of any node on the volume.

	\param volume dev_t referring to the volume to be watched.
	\param flags Flags indicating the actions to be performed.
	\param target Messenger referring to the target. Must be valid.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
watch_volume(dev_t volume, uint32 flags, BMessenger target)
{
	if ((flags & B_WATCH_NAME) == 0 && (flags & B_WATCH_STAT) == 0
		&& (flags & B_WATCH_ATTR) == 0)
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


// watch_node
/*!	\brief Subscribes a target to node and/or mount watching, or unsubscribes
		   it from node watching.

	Depending of \a flags the action performed by this function varies:
	- \a flags is \c 0: The target is unsubscribed from watching the node.
	  \a node must not be \c NULL in this case.
	- \a flags contains \c B_WATCH_MOUNT: The target is subscribed to mount
	  watching.
	- \a flags contains at least one of \c B_WATCH_NAME, \c B_WATCH_STAT,
	  \c B_WATCH_ATTR, or \c B_WATCH_DIRECTORY: The target is subscribed to
	  watching the specified aspects of the node. \a node must not be \c NULL
	  in this case.

	Note, that the latter two cases are not mutual exlusive, i.e. mount and
	node watching can be requested with a single call.

	\param node node_ref referring to the node to be watched. May be \c NULL,
		   if only mount watching is requested.
	\param flags Flags indicating the actions to be performed.
	\param target Messenger referring to the target. Must be valid.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
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

// watch_node
/*!	\brief Subscribes a target to node and/or mount watching, or unsubscribes
		   it from node watching.

	Depending of \a flags the action performed by this function varies:
	- \a flags is \c 0: The target is unsubscribed from watching the node.
	  \a node must not be \c NULL in this case.
	- \a flags contains \c B_WATCH_MOUNT: The target is subscribed to mount
	  watching.
	- \a flags contains at least one of \c B_WATCH_NAME, \c B_WATCH_STAT,
	  \c B_WATCH_ATTR, or \c B_WATCH_DIRECTORY: The target is subscribed to
	  watching the specified aspects of the node. \a node must not be \c NULL
	  in this case.

	Note, that the latter two cases are not mutual exlusive, i.e. mount and
	node watching can be requested with a single call.

	\param node node_ref referring to the node to be watched. May be \c NULL,
		   if only mount watching is requested.
	\param flags Flags indicating the actions to be performed.
	\param handler The target handler. May be \c NULL, if \a looper is not
		   \c NULL. Then the preferred handler of the looper is targeted.
	\param looper The target looper. May be \c NULL, if \a handler is not
		   \c NULL. Then the handler's looper is the target looper.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
watch_node(const node_ref *node, uint32 flags, const BHandler *handler,
	const BLooper *looper)
{
	return watch_node(node, flags, BMessenger(handler, looper));
}


/*!	\brief Unsubscribes a target from node and mount monitoring.
	\param target Messenger referring to the target. Must be valid.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
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


/*!	\brief Unsubscribes a target from node and mount monitoring.
	\param handler The target handler. May be \c NULL, if \a looper is not
		   \c NULL. Then the preferred handler of the looper is targeted.
	\param looper The target looper. May be \c NULL, if \a handler is not
		   \c NULL. Then the handler's looper is the target looper.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
stop_watching(const BHandler *handler, const BLooper *looper)
{
	return stop_watching(BMessenger(handler, looper));
}

