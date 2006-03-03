/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/node_monitor.h>
#include <app/AppDefs.h>

#include <fs/node_monitor.h>
#include <vfs.h>
#include <fd.h>
#include <lock.h>
#include <khash.h>
#include <messaging.h>
#include <util/AutoLock.h>
#include <util/KMessage.h>
#include <util/list.h>

#include <malloc.h>
#include <stddef.h>


//#define TRACE_MONITOR
#ifdef TRACE_MONITOR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// ToDo: add more fine grained locking - maybe using a ref_count in the
//		node_monitor structure?
// ToDo: implement watching mounts/unmounts
// ToDo: return another error code than B_NO_MEMORY if the team's maximum is hit


typedef struct monitor_listener monitor_listener;
typedef struct node_monitor node_monitor;

struct monitor_listener {
	monitor_listener	*next;
	monitor_listener	*prev;
	list_link			monitor_link;
	port_id				port;
	uint32				token;
	uint32				flags;
	node_monitor		*monitor;
};

struct node_monitor {
	node_monitor		*next;
	mount_id			device;
	vnode_id			node;
	struct list			listeners;
};

struct monitor_hash_key {
	mount_id	device;
	vnode_id	node;
};

struct interested_monitor_listener_list {
	struct list			*listeners;
	monitor_listener	*first_listener;
	uint32				flags;
};

#define MONITORS_HASH_TABLE_SIZE 16

static hash_table *gMonitorHash;
static mutex gMonitorMutex;


static int
monitor_compare(void *_monitor, const void *_key)
{
	node_monitor *monitor = (node_monitor*)_monitor;
	const struct monitor_hash_key *key = (const struct monitor_hash_key*)_key;

	if (monitor->device == key->device && monitor->node == key->node)
		return 0;

	return -1;
}


static uint32
monitor_hash(void *_monitor, const void *_key, uint32 range)
{
	node_monitor *monitor = (node_monitor*)_monitor;
	const struct monitor_hash_key *key = (const struct monitor_hash_key*)_key;

#define MHASH(device, node) (((uint32)((node) >> 32) + (uint32)(node)) ^ (uint32)(device))

	if (monitor != NULL)
		return MHASH(monitor->device, monitor->node) % range;

	return MHASH(key->device, key->node) % range;
#undef MHASH
}


/** Returns the monitor that matches the specified device/node pair.
 *	Must be called with monitors lock hold.
 */

static node_monitor *
get_monitor_for(mount_id device, vnode_id node)
{
	struct monitor_hash_key key;
	key.device = device;
	key.node = node;

	return (node_monitor *)hash_lookup(gMonitorHash, &key);
}


/** Returns the listener that matches the specified port/token pair.
 *	Must be called with monitors lock hold.
 */

static monitor_listener *
get_listener_for(node_monitor *monitor, port_id port, uint32 token)
{
	monitor_listener *listener = NULL;
	
	while ((listener = (monitor_listener*)list_get_next_item(
			&monitor->listeners, listener)) != NULL) {
		// does this listener match?
		if (listener->port == port && listener->token == token)
			return listener;
	}

	return NULL;
}


/** Removes the specified node_monitor from the hashtable
 *	and free it.
 *	Must be called with monitors lock hold.
 */

static void
remove_monitor(node_monitor *monitor)
{
	hash_remove(gMonitorHash, monitor);
	free(monitor);
}


/** Removes the specified monitor_listener from all lists
 *	and free it.
 *	Must be called with monitors lock hold.
 */

static void
remove_listener(monitor_listener *listener)
{
	node_monitor *monitor = listener->monitor;

	// remove it from the listener and I/O context lists	
	list_remove_link(&listener->monitor_link);
	list_remove_link(listener);

	free(listener);

	if (list_is_empty(&monitor->listeners))
		remove_monitor(monitor);
}


static status_t
add_node_monitor(io_context *context, mount_id device, vnode_id node,
	uint32 flags, port_id port, uint32 token)
{
	monitor_listener *listener;
	node_monitor *monitor;
	status_t status = B_OK;

	TRACE(("add_node_monitor(dev = %ld, node = %Ld, flags = %ld, port = %ld, token = %ld\n",
		device, node, flags, port, token));

	mutex_lock(&gMonitorMutex);

	monitor = get_monitor_for(device, node);
	if (monitor == NULL) {
		// check if this team is allowed to have more listeners
		if (context->num_monitors >= context->max_monitors) {
			// the BeBook says to return B_NO_MEMORY in this case, but
			// we should have another one.
			status = B_NO_MEMORY;
			goto out;
		}

		// create new monitor
		monitor = (node_monitor *)malloc(sizeof(node_monitor));
		if (monitor == NULL) {
			status = B_NO_MEMORY;
			goto out;
		}

		// initialize monitor		
		monitor->device = device;
		monitor->node = node;
		list_init_etc(&monitor->listeners, offsetof(monitor_listener, monitor_link));

		hash_insert(gMonitorHash, monitor);
	} else {
		// check if the listener is already listening, and
		// if so, just add the new flags

		listener = get_listener_for(monitor, port, token);
		if (listener != NULL) {
			listener->flags |= flags;
			goto out;
		}

		// check if this team is allowed to have more listeners
		if (context->num_monitors >= context->max_monitors) {
			// the BeBook says to return B_NO_MEMORY in this case, but
			// we should have another one.
			status = B_NO_MEMORY;
			goto out;
		}
	}

	// add listener

	listener = (monitor_listener *)malloc(sizeof(monitor_listener));
	if (listener == NULL) {
		// no memory for the listener, so remove the monitor as well if needed
		if (list_is_empty(&monitor->listeners))
			remove_monitor(monitor);

		status = B_NO_MEMORY;
		goto out;
	}

	// initialize listener, and add it to the lists
	listener->port = port;
	listener->token = token;
	listener->flags = flags;
	listener->monitor = monitor;

	list_add_link_to_head(&monitor->listeners, &listener->monitor_link);
	list_add_link_to_head(&context->node_monitors, listener);

	context->num_monitors++;

out:
	mutex_unlock(&gMonitorMutex);
	return status;
}


static status_t
remove_node_monitor(struct io_context *context, mount_id device, vnode_id node,
	uint32 flags, port_id port, uint32 token)
{
	monitor_listener *listener;
	node_monitor *monitor;
	status_t status = B_OK;

	TRACE(("remove_node_monitor(dev = %ld, node = %Ld, flags = %ld, port = %ld, token = %ld\n",
		device, node, flags, port, token));

	mutex_lock(&gMonitorMutex);

	// get the monitor for this device/node pair
	monitor = get_monitor_for(device, node);
	if (monitor == NULL) {
		status = B_ENTRY_NOT_FOUND;
		goto out;
	}

	// see if it has the listener we are looking for
	listener = get_listener_for(monitor, port, token);
	if (listener == NULL) {
		status = B_ENTRY_NOT_FOUND;
		goto out;
	}

	// no flags means remove all flags
	if (flags == B_STOP_WATCHING)
		flags = ~0;

	listener->flags &= ~flags;

	// if there aren't anymore flags, remove this listener
	if (listener->flags == B_STOP_WATCHING) {
		remove_listener(listener);
		context->num_monitors--;
	}

out:
	mutex_unlock(&gMonitorMutex);
	return status;
}


static status_t
remove_node_monitors_by_target(struct io_context *context, port_id port, uint32 token)
{
	monitor_listener *listener = NULL;
	int32 count = 0;

	while ((listener = (monitor_listener*)list_get_next_item(
			&context->node_monitors, listener)) != NULL) {
		monitor_listener *removeListener;

		if (listener->port != port || listener->token != token)
			continue;

		listener = (monitor_listener*)list_get_prev_item(
			&context->node_monitors, removeListener = listener);
			// this line sets the listener one item back, allowing us
			// to remove its successor (which is saved in "removeListener")

		remove_listener(removeListener);
		count++;
	}

	return count > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


/**	\brief Given device and node ID and a node monitoring event mask, the
 *		   function checks whether there are listeners interested in any of
 *		   the events for that node and, if so, adds the respective listener
 *		   list to a supplied array of listener lists.
 *
 *	Note, that in general not all of the listeners in an appended list will be
 *	interested in the events, but it is guaranteed that
 *	interested_monitor_listener_list::first_listener is indeed
 *	the first listener in the list, that is interested.
 *
 *	\param device The ID of the mounted FS, the node lives in.
 *	\param node The ID of the node.
 *	\param flags The mask specifying the events occurred for the given node
 *		   (a combination of \c B_WATCH_* constants).
 *	\param interestedListeners An array of listener lists. If there are
 *		   interested listeners for the node, the list will be appended to
 *		   this array.
 *	\param interestedListenerCount The number of elements in the
 *		   \a interestedListeners array. Will be incremented, if a list is
 *		   appended.
 */

static void
get_interested_monitor_listeners(mount_id device, vnode_id node,
	uint32 flags, interested_monitor_listener_list *interestedListeners,
	int32 &interestedListenerCount)
{
	// get the monitor for the node
	node_monitor *monitor = get_monitor_for(device, node);
	if (!monitor)
		return;

	// iterate through the listeners until we find one with matching flags
	monitor_listener *listener = NULL;
	while ((listener = (monitor_listener*)list_get_next_item(
			&monitor->listeners, listener)) != NULL) {
		if (listener->flags & flags) {
			interested_monitor_listener_list &list
				= interestedListeners[interestedListenerCount++];
			list.listeners = &monitor->listeners;
			list.first_listener = listener;
			list.flags = flags;
			return;
		}
	}
}


/**	\brief Sends a notifcation message to the given listeners.
 *	\param message The message to be sent.
 *	\param interestedListeners An array of listener lists.
 *	\param interestedListenerCount The number of elements in the
 *		   \a interestedListeners array. 
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

static status_t
send_notification_message(KMessage &message,
	interested_monitor_listener_list *interestedListeners,
	int32 interestedListenerCount)
{
	// Since the messaging service supports broadcasting and that is more
	// efficient than sending the messages individually, we collect the
	// listener targets in an array and send the message to them at once.
	const int32 maxTargetCount = 16;
	messaging_target targets[maxTargetCount];
	int32 targetCount = 0;

	// iterate through the lists
	interested_monitor_listener_list *list = interestedListeners;
	for (int32 i = 0; i < interestedListenerCount; i++, list++) {
		// iterate through the listeners
		monitor_listener *listener = list->first_listener;
		do {
			if (listener->flags & list->flags) {
				// the listener's flags match: add it to the targets
				messaging_target &target = targets[targetCount++];
				target.port = listener->port;
				target.token = listener->token;

				// if the target array is full, send the message
				if (targetCount == maxTargetCount) {
					status_t error = send_message(&message, targets,
						targetCount);
					if (error != B_OK)
						return error;

					targetCount = 0;
				}
			}
		} while ((listener = (monitor_listener*)list_get_next_item(
				list->listeners, listener)) != NULL);
	}

	// if any targets are left (the usual case, unless the target array got
	// full early), send the message
	if (targetCount > 0)
		return send_message(&message, targets, targetCount);

	return B_OK;
}


/**	\brief Notifies all interested listeners that an entry has been created
 *		   or removed.
 *	\param opcode \c B_ENTRY_CREATED or \c B_ENTRY_REMOVED.
 *	\param device The ID of the mounted FS, the entry lives/lived in.
 *	\param directory The entry's parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry refers/referred to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

static status_t
notify_entry_created_or_removed(int32 opcode, mount_id device,
	vnode_id directory, const char *name, vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the node
	if (opcode != B_ENTRY_CREATED) {
		get_interested_monitor_listeners(device, node, B_WATCH_NAME,
			interestedListeners, interestedListenerCount);
	}
	// ... for the directory
	get_interested_monitor_listeners(device, directory, B_WATCH_DIRECTORY,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[1024];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", opcode);
	message.AddInt32("device", device);
	message.AddInt64("directory", directory);
	message.AddInt64("node", node);
	message.AddString("name", name);			// for "removed" Haiku only

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);
}


/**	\brief Notifies the listener of a live query that an entry has been added
 *		   to or removed from the query (for whatever reason).
 *	\param opcode \c B_ENTRY_CREATED or \c B_ENTRY_REMOVED.
 *	\param port The target port of the listener.
 *	\param token The BHandler token of the listener.
 *	\param device The ID of the mounted FS, the entry lives in.
 *	\param directory The entry's parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry refers to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

static status_t
notify_query_entry_created_or_removed(int32 opcode, port_id port, int32 token,
	mount_id device, vnode_id directory, const char *name, vnode_id node)
{
	if (!name)
		return B_BAD_VALUE;

	// construct the message
	char messageBuffer[1024];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_QUERY_UPDATE);
	message.AddInt32("opcode", opcode);
	message.AddInt32("device", device);
	message.AddInt64("directory", directory);
	message.AddInt64("node", node);
	message.AddString("name", name);

	// send the message
	messaging_target target;
	target.port = port;
	target.token = token;

	return send_message(&message, &target, 1);
}


//	#pragma mark - private kernel API


status_t
remove_node_monitors(struct io_context *context)
{
	mutex_lock(&gMonitorMutex);

	while (!list_is_empty(&context->node_monitors)) {
		// the remove_listener() function will also free the node_monitor
		// if it doesn't have any listeners attached anymore
		remove_listener(
			(monitor_listener*)list_get_first_item(&context->node_monitors));
	}

	mutex_unlock(&gMonitorMutex);
	return B_OK;
}


status_t
node_monitor_init(void)
{
	gMonitorHash = hash_init(MONITORS_HASH_TABLE_SIZE, offsetof(node_monitor, next),
		&monitor_compare, &monitor_hash);
	if (gMonitorHash == NULL)
			panic("node_monitor_init: error creating mounts hash table\n");

	return mutex_init(&gMonitorMutex, "node monitor");
}


status_t
notify_unmount(mount_id device)
{
	TRACE(("unmounted device: %ld\n", device));

	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	get_interested_monitor_listeners(-1, -1, B_WATCH_MOUNT,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[96];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_DEVICE_UNMOUNTED);
	message.AddInt32("device", device);

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);

	return B_OK;
}


status_t
notify_mount(mount_id device, mount_id parentDevice, vnode_id parentDirectory)
{
	TRACE(("mounted device: %ld, parent %ld:%Ld\n", device, parentDevice,
		parentDirectory));

	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	get_interested_monitor_listeners(-1, -1, B_WATCH_MOUNT,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[128];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_DEVICE_MOUNTED);
	message.AddInt32("new device", device);
	message.AddInt32("device", parentDevice);
	message.AddInt64("directory", parentDirectory);

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);

	return B_OK;
}


//	#pragma mark - public kernel API


/**	\brief Subscribes a target to node and/or mount watching.
 *
 *	Depending on \a flags, different actions are performed. If flags is \c 0,
 *	mount watching is requested. \a device and \a node must be \c -1 in this
 *	case. Otherwise node watching is requested. \a device and \a node must
 *	refer to a valid node, and \a flags must note contain the flag
 *	\c B_WATCH_MOUNT, but at least one of the other valid flags.
 *
 *	\param device The device the node resides on (node_ref::device). \c -1, if
 *		   only mount watching is requested.
 *	\param node The node ID of the node (node_ref::device). \c -1, if
 *		   only mount watching is requested.
 *	\param flags A bit mask composed of the values specified in
 *		   <NodeMonitor.h>.
 *	\param port The port of the target (a looper port).
 *	\param handlerToken The token of the target handler. \c -2, if the
 *		   preferred handler of the looper is the target.
 *	\return \c B_OK, if everything went fine, another error code otherwise.
 */

status_t
start_watching(dev_t device, ino_t node, uint32 flags, port_id port, uint32 token)
{
	io_context *context = get_current_io_context(true);

	return add_node_monitor(context, device, node, flags, port, token);
}


/**	\brief Unsubscribes a target from watching a node.
 *	\param device The device the node resides on (node_ref::device).
 *	\param node The node ID of the node (node_ref::device).
 *	\param flags Which monitors should be removed (B_STOP_WATCHING for all)
 *	\param port The port of the target (a looper port).
 *	\param handlerToken The token of the target handler. \c -2, if the
 *		   preferred handler of the looper is the target.
 *	\return \c B_OK, if everything went fine, another error code otherwise.
 */

status_t
stop_watching(dev_t device, ino_t node, uint32 flags, port_id port, uint32 token)
{
	io_context *context = get_current_io_context(true);

	return remove_node_monitor(context, device, node, flags, port, token);
}


/**	\brief Unsubscribes a target from node and mount monitoring.
 *	\param port The port of the target (a looper port).
 *	\param handlerToken The token of the target handler. \c -2, if the
 *		   preferred handler of the looper is the target.
 *	\return \c B_OK, if everything went fine, another error code otherwise.
 */

status_t
stop_notifying(port_id port, uint32 token)
{
	io_context *context = get_current_io_context(true);

	return remove_node_monitors_by_target(context, port, token);
}


status_t
notify_listener(int op, mount_id device, vnode_id parentNode, vnode_id toParentNode,
	vnode_id node, const char *name)
{
	monitor_listener *listener;
	node_monitor *monitor;

	TRACE(("notify_listener(op = %d, device = %ld, node = %Ld, parent = %Ld, toParent = %Ld"
		", name = \"%s\"\n", op, device, node, parentNode, toParentNode, name));

	mutex_lock(&gMonitorMutex);

	// check the main "node"

	if ((op == B_ENTRY_MOVED
		|| op == B_ENTRY_REMOVED
		|| op == B_STAT_CHANGED
		|| op == B_ATTR_CHANGED)
		&& (monitor = get_monitor_for(device, node)) != NULL) {
		// iterate over all listeners for this monitor, and see
		// if we have to send anything
		listener = NULL;
		while ((listener = (monitor_listener*)list_get_next_item(
				&monitor->listeners, listener)) != NULL) {
			// do we have a reason to notify this listener?
			if (((listener->flags & B_WATCH_NAME) != 0
				&& (op == B_ENTRY_MOVED || op == B_ENTRY_REMOVED))
				|| ((listener->flags & B_WATCH_STAT) != 0
				&& op == B_STAT_CHANGED)
				|| ((listener->flags & B_WATCH_ATTR) != 0
				&& op == B_ATTR_CHANGED)) {
				// then do it!
				send_notification(listener->port, listener->token, B_NODE_MONITOR,
					op, device, 0, parentNode, toParentNode, node, name);
			}
		}
	}

	// check its parent directory

	if ((op == B_ENTRY_MOVED
		|| op == B_ENTRY_REMOVED
		|| op == B_ENTRY_CREATED)
		&& (monitor = get_monitor_for(device, parentNode)) != NULL) {
		// iterate over all listeners for this monitor, and see
		// if we have to send anything
		listener = NULL;
		while ((listener = (monitor_listener*)list_get_next_item(
				&monitor->listeners, listener)) != NULL) {
			// do we have a reason to notify this listener?
			if ((listener->flags & B_WATCH_DIRECTORY) != 0) {
				send_notification(listener->port, listener->token, B_NODE_MONITOR,
					op, device, 0, parentNode, toParentNode, node, name);
			}
		}
	}

	// check its new target parent directory

	if (op == B_ENTRY_MOVED
		&& (monitor = get_monitor_for(device, toParentNode)) != NULL) {
		// iterate over all listeners for this monitor, and see
		// if we have to send anything
		listener = NULL;
		while ((listener = (monitor_listener*)list_get_next_item(
				&monitor->listeners, listener)) != NULL) {
			// do we have a reason to notify this listener?
			if ((listener->flags & B_WATCH_DIRECTORY) != 0) {
				send_notification(listener->port, listener->token, B_NODE_MONITOR,
					B_ENTRY_MOVED, device, 0, parentNode, toParentNode, node, name);
			}
		}
	}

	mutex_unlock(&gMonitorMutex);
	return B_OK;
}


/**	\brief Notifies all interested listeners that an entry has been created.
 *	\param device The ID of the mounted FS, the entry lives in.
 *	\param directory The entry's parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry refers to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_entry_created(mount_id device, vnode_id directory, const char *name,
	vnode_id node)
{
	return notify_entry_created_or_removed(B_ENTRY_CREATED, device,
		directory, name, node);
}


/**	\brief Notifies all interested listeners that an entry has been removed.
 *	\param device The ID of the mounted FS, the entry lived in.
 *	\param directory The entry's former parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry referred to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_entry_removed(mount_id device, vnode_id directory, const char *name,
	vnode_id node)
{
	return notify_entry_created_or_removed(B_ENTRY_REMOVED, device,
		directory, name, node);
}


/**	\brief Notifies all interested listeners that an entry has been moved.
 *	\param device The ID of the mounted FS, the entry lives in.
 *	\param fromDirectory The entry's previous parent directory ID.
 *	\param fromName The entry's previous name.
 *	\param toDirectory The entry's new parent directory ID.
 *	\param toName The entry's new name.
 *	\param node The ID of the node the entry refers to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_entry_moved(mount_id device, vnode_id fromDirectory,
	const char *fromName, vnode_id toDirectory, const char *toName,
	vnode_id node)
{
	if (!fromName || !toName)
		return B_BAD_VALUE;

	// If node is a mount point, we need to resolve it to the mounted
	// volume's root node.
	mount_id nodeDevice = device;
	resolve_mount_point_to_volume_root(device, node, &nodeDevice, &node);

	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the node
	get_interested_monitor_listeners(nodeDevice, node, B_WATCH_NAME,
		interestedListeners, interestedListenerCount);
	// ... for the source directory
	get_interested_monitor_listeners(device, fromDirectory, B_WATCH_DIRECTORY,
		interestedListeners, interestedListenerCount);
	// ... for the target directory
	get_interested_monitor_listeners(device, toDirectory, B_WATCH_DIRECTORY,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[1024];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_ENTRY_MOVED);
	message.AddInt32("device", device);
	message.AddInt64("from directory", fromDirectory);
	message.AddInt64("to directory", toDirectory);
	message.AddInt32("node device", nodeDevice);	// Haiku only
	message.AddInt64("node", node);
	message.AddString("from name", fromName);		// Haiku only
	message.AddString("name", toName);

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);
}


/**	\brief Notifies all interested listeners that a node's stat data have
 *		   changed.
 *	\param device The ID of the mounted FS, the node lives in.
 *	\param node The ID of the node.
 *	\param statFields A bitwise combination of one or more of the \c B_STAT_*
 *		   constants defined in <NodeMonitor.h>, indicating what fields of the
 *		   stat data have changed.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_stat_changed(mount_id device, vnode_id node, uint32 statFields)
{
	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the node
	get_interested_monitor_listeners(device, node, B_WATCH_STAT,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[1024];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_STAT_CHANGED);
	message.AddInt32("device", device);
	message.AddInt64("node", node);
	message.AddInt32("fields", statFields);		// Haiku only

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);
}


/**	\brief Notifies all interested listeners that a node attribute has changed.
 *	\param device The ID of the mounted FS, the node lives in.
 *	\param node The ID of the node.
 *	\param attribute The attribute's name.
 *	\param cause Either of \c B_ATTR_CREATED, \c B_ATTR_REMOVED, or
 *		   \c B_ATTR_CHANGED, indicating what exactly happened to the attribute.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_attribute_changed(mount_id device, vnode_id node, const char *attribute,
	int32 cause)
{
	if (!attribute)
		return B_BAD_VALUE;

	MutexLocker locker(gMonitorMutex);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the node
	get_interested_monitor_listeners(device, node, B_WATCH_ATTR,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[1024];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_ATTR_CHANGED);
	message.AddInt32("device", device);
	message.AddInt64("node", node);
	message.AddString("attr", attribute);
	message.AddInt32("cause", cause);		// Haiku only

	return send_notification_message(message, interestedListeners,
		interestedListenerCount);
}


/**	\brief Notifies the listener of a live query that an entry has been added
 *		   to the query (for whatever reason).
 *	\param port The target port of the listener.
 *	\param token The BHandler token of the listener.
 *	\param device The ID of the mounted FS, the entry lives in.
 *	\param directory The entry's parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry refers to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_query_entry_created(port_id port, int32 token, mount_id device,
	vnode_id directory, const char *name, vnode_id node)
{
	return notify_query_entry_created_or_removed(B_ENTRY_CREATED, port, token,
		device, directory, name, node);
}


/**	\brief Notifies the listener of a live query that an entry has been removed
 *		   from the query (for whatever reason).
 *	\param port The target port of the listener.
 *	\param token The BHandler token of the listener.
 *	\param device The ID of the mounted FS, the entry lives in.
 *	\param directory The entry's parent directory ID.
 *	\param name The entry's name.
 *	\param node The ID of the node the entry refers to.
 *	\return
 *	- \c B_OK, if everything went fine,
 *	- another error code otherwise.
 */

status_t
notify_query_entry_removed(port_id port, int32 token,
	mount_id device, vnode_id directory, const char *name, vnode_id node)
{
	return notify_query_entry_created_or_removed(B_ENTRY_REMOVED, port, token,
		device, directory, name, node);
}


//	#pragma mark - User syscalls


status_t
_user_stop_notifying(port_id port, uint32 token)
{
	io_context *context = get_current_io_context(false);

	return remove_node_monitors_by_target(context, port, token);
}


status_t
_user_start_watching(dev_t device, ino_t node, uint32 flags, port_id port, uint32 token)
{
	io_context *context = get_current_io_context(false);

	return add_node_monitor(context, device, node, flags, port, token);
}


status_t
_user_stop_watching(dev_t device, ino_t node, uint32 flags, port_id port, uint32 token)
{
	io_context *context = get_current_io_context(false);

	return remove_node_monitor(context, device, node, flags, port, token);
}

