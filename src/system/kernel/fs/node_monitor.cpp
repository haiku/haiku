/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005-2008, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include <fs/node_monitor.h>

#include <stddef.h>
#include <stdlib.h>

#include <AppDefs.h>
#include <NodeMonitor.h>

#include <fd.h>
#include <khash.h>
#include <lock.h>
#include <messaging.h>
#include <Notifications.h>
#include <vfs.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/KMessage.h>
#include <util/list.h>

#include "FDPath.h"
#include "node_monitor_private.h"


//#define TRACE_MONITOR
#ifdef TRACE_MONITOR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// ToDo: add more fine grained locking - maybe using a ref_count in the
//		node_monitor structure?
// ToDo: return another error code than B_NO_MEMORY if the team's maximum is hit


typedef struct monitor_listener monitor_listener;
typedef struct node_monitor node_monitor;

struct monitor_listener {
	list_link			context_link;
	DoublyLinkedListLink<monitor_listener> monitor_link;
	NotificationListener *listener;
	uint32				flags;
	node_monitor		*monitor;
};

typedef DoublyLinkedList<monitor_listener, DoublyLinkedListMemberGetLink<
	monitor_listener, &monitor_listener::monitor_link> > MonitorListenerList;

struct node_monitor {
	node_monitor*		hash_link;
	dev_t				device;
	ino_t				node;
	MonitorListenerList	listeners;
};

struct interested_monitor_listener_list {
	MonitorListenerList::Iterator iterator;
	uint32				flags;
};

static UserMessagingMessageSender sNodeMonitorSender;

class UserNodeListener : public UserMessagingListener {
	public:
		UserNodeListener(port_id port, int32 token)
			: UserMessagingListener(sNodeMonitorSender, port, token)
		{
		}

		bool operator==(const NotificationListener& _other) const
		{
			const UserNodeListener* other
				= dynamic_cast<const UserNodeListener*>(&_other);
			return other != NULL && other->Port() == Port()
				&& other->Token() == Token();
		}
};

class NodeMonitorService : public NotificationService {
	public:
		NodeMonitorService();
		virtual ~NodeMonitorService();

		status_t InitCheck();

		status_t NotifyEntryCreatedOrRemoved(int32 opcode, dev_t device,
			ino_t directory, const char *name, ino_t node);
		status_t NotifyEntryMoved(dev_t device, ino_t fromDirectory,
			const char *fromName, ino_t toDirectory, const char *toName,
			ino_t node);
		status_t NotifyStatChanged(dev_t device, ino_t node, uint32 statFields);
		status_t NotifyAttributeChanged(dev_t device, ino_t node,
			const char *attribute, int32 cause);
		status_t NotifyUnmount(dev_t device);
		status_t NotifyMount(dev_t device, dev_t parentDevice,
			ino_t parentDirectory);

		status_t RemoveListeners(io_context *context);

		status_t AddListener(const KMessage *eventSpecifier,
			NotificationListener &listener);
		status_t UpdateListener(const KMessage *eventSpecifier,
			NotificationListener &listener);
		status_t RemoveListener(const KMessage *eventSpecifier,
			NotificationListener &listener);

		status_t AddListener(io_context *context, dev_t device, ino_t node,
			uint32 flags, NotificationListener &notificationListener);
		status_t RemoveListener(io_context *context, dev_t device, ino_t node,
			NotificationListener &notificationListener);

		status_t RemoveUserListeners(struct io_context *context,
			port_id port, uint32 token);
		status_t UpdateUserListener(io_context *context, dev_t device,
			ino_t node, uint32 flags, UserNodeListener &userListener);

		virtual const char* Name() { return "node monitor"; }

	private:
		void _RemoveMonitor(node_monitor *monitor, uint32 flags);
		status_t _RemoveListener(io_context *context, dev_t device, ino_t node,
			NotificationListener& notificationListener, bool isVolumeListener);
		void _RemoveListener(monitor_listener *listener);
		node_monitor *_MonitorFor(dev_t device, ino_t node,
			bool isVolumeListener);
		status_t _GetMonitor(io_context *context, dev_t device, ino_t node,
			bool addIfNecessary, node_monitor **_monitor,
			bool isVolumeListener);
		monitor_listener *_MonitorListenerFor(node_monitor* monitor,
			NotificationListener& notificationListener);
		status_t _AddMonitorListener(io_context *context,
			node_monitor* monitor, uint32 flags,
			NotificationListener& notificationListener);
		status_t _UpdateListener(io_context *context, dev_t device, ino_t node,
			uint32 flags, bool addFlags,
			NotificationListener &notificationListener);
		void _GetInterestedMonitorListeners(dev_t device, ino_t node,
			uint32 flags, interested_monitor_listener_list *interestedListeners,
			int32 &interestedListenerCount);
		void _GetInterestedVolumeListeners(dev_t device, uint32 flags,
			interested_monitor_listener_list *interestedListeners,
			int32 &interestedListenerCount);
		status_t _SendNotificationMessage(KMessage &message,
			interested_monitor_listener_list *interestedListeners,
			int32 interestedListenerCount);

		struct monitor_hash_key {
			dev_t	device;
			ino_t	node;
		};

		struct HashDefinition {
			typedef monitor_hash_key* KeyType;
			typedef	node_monitor ValueType;

			size_t HashKey(monitor_hash_key* key) const
				{ return _Hash(key->device, key->node); }
			size_t Hash(node_monitor *monitor) const
				{ return _Hash(monitor->device, monitor->node); }

			bool Compare(monitor_hash_key* key, node_monitor *monitor) const
			{
				return key->device == monitor->device
					&& key->node == monitor->node;
			}

			node_monitor*& GetLink(node_monitor* monitor) const
				{ return monitor->hash_link; }

			uint32 _Hash(dev_t device, ino_t node) const
			{
				return ((uint32)(node >> 32) + (uint32)node) ^ (uint32)device;
			}
		};

		typedef BOpenHashTable<HashDefinition> MonitorHash;

		struct VolumeHashDefinition {
			typedef dev_t KeyType;
			typedef	node_monitor ValueType;

			size_t HashKey(dev_t key) const
				{ return _Hash(key); }
			size_t Hash(node_monitor *monitor) const
				{ return _Hash(monitor->device); }

			bool Compare(dev_t key, node_monitor *monitor) const
			{
				return key == monitor->device;
			}

			node_monitor*& GetLink(node_monitor* monitor) const
				{ return monitor->hash_link; }

			uint32 _Hash(dev_t device) const
			{
				return (uint32)(device >> 16) + (uint16)device;
			}
		};

		typedef BOpenHashTable<VolumeHashDefinition> VolumeMonitorHash;

		MonitorHash	fMonitors;
		VolumeMonitorHash fVolumeMonitors;
		recursive_lock fRecursiveLock;
};

static NodeMonitorService sNodeMonitorService;


/*!	\brief Notifies the listener of a live query that an entry has been added
  		   to or removed from or updated and still in the query (for whatever
  		   reason).
  	\param opcode \c B_ENTRY_CREATED or \c B_ENTRY_REMOVED or \c B_ATTR_CHANGED.
  	\param port The target port of the listener.
  	\param token The BHandler token of the listener.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param directory The entry's parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
static status_t
notify_query_entry_event(int32 opcode, port_id port, int32 token,
	dev_t device, ino_t directory, const char *name, ino_t node)
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


//	#pragma mark - NodeMonitorService


NodeMonitorService::NodeMonitorService()
{
	recursive_lock_init(&fRecursiveLock, "node monitor");
}


NodeMonitorService::~NodeMonitorService()
{
	recursive_lock_destroy(&fRecursiveLock);
}


status_t
NodeMonitorService::InitCheck()
{
	return B_OK;
}


/*! Removes the specified node_monitor from the hashtable
	and free it.
	Must be called with monitors lock hold.
*/
void
NodeMonitorService::_RemoveMonitor(node_monitor *monitor, uint32 flags)
{
	if ((flags & B_WATCH_VOLUME) != 0)
		fVolumeMonitors.Remove(monitor);
	else
		fMonitors.Remove(monitor);
	delete monitor;
}


//! Helper function for the RemoveListener function.
status_t
NodeMonitorService::_RemoveListener(io_context *context, dev_t device,
	ino_t node, NotificationListener& notificationListener,
	bool isVolumeListener)
{
	TRACE(("%s(dev = %ld, node = %Ld, listener = %p\n",
		__PRETTY_FUNCTION__, device, node, &notificationListener));

	RecursiveLocker _(fRecursiveLock);

	// get the monitor for this device/node pair
	node_monitor *monitor = _MonitorFor(device, node, isVolumeListener);
	if (monitor == NULL)
		return B_BAD_VALUE;

	// see if it has the listener we are looking for
	monitor_listener* listener = _MonitorListenerFor(monitor,
		notificationListener);
	if (listener == NULL)
		return B_BAD_VALUE;

	_RemoveListener(listener);
	context->num_monitors--;

	return B_OK;
}


/*! Removes the specified monitor_listener from all lists
	and free it.
	Must be called with monitors lock hold.
*/
void
NodeMonitorService::_RemoveListener(monitor_listener *listener)
{
	uint32 flags = listener->flags;
	node_monitor *monitor = listener->monitor;

	// remove it from the listener and I/O context lists
	monitor->listeners.Remove(listener);
	list_remove_link(listener);

	if (dynamic_cast<UserNodeListener*>(listener->listener) != NULL) {
		// This is a listener we copied ourselves in UpdateUserListener(),
		// so we have to delete it here.
		delete listener->listener;
	}

	delete listener;

	if (monitor->listeners.IsEmpty())
		_RemoveMonitor(monitor, flags);
}


/*! Returns the monitor that matches the specified device/node pair.
	Must be called with monitors lock hold.
*/
node_monitor *
NodeMonitorService::_MonitorFor(dev_t device, ino_t node, bool isVolumeListener)
{
	if (isVolumeListener)
		return fVolumeMonitors.Lookup(device);

	struct monitor_hash_key key;
	key.device = device;
	key.node = node;

	return fMonitors.Lookup(&key);
}


/*! Returns the monitor that matches the specified device/node pair.
	If the monitor does not exist yet, it will be created.
	Must be called with monitors lock hold.
*/
status_t
NodeMonitorService::_GetMonitor(io_context *context, dev_t device, ino_t node,
	bool addIfNecessary, node_monitor** _monitor, bool isVolumeListener)
{
	node_monitor* monitor = _MonitorFor(device, node, isVolumeListener);
	if (monitor != NULL) {
		*_monitor = monitor;
		return B_OK;
	}
	if (!addIfNecessary)
		return B_BAD_VALUE;

	// check if this team is allowed to have more listeners
	if (context->num_monitors >= context->max_monitors) {
		// the BeBook says to return B_NO_MEMORY in this case, but
		// we should have another one.
		return B_NO_MEMORY;
	}

	// create new monitor
	monitor = new(std::nothrow) node_monitor;
	if (monitor == NULL)
		return B_NO_MEMORY;

	// initialize monitor
	monitor->device = device;
	monitor->node = node;

	status_t status;
	if (isVolumeListener)
		status = fVolumeMonitors.Insert(monitor);
	else
		status = fMonitors.Insert(monitor);
	if (status < B_OK) {
		delete monitor;
		return B_NO_MEMORY;
	}

	*_monitor = monitor;
	return B_OK;
}


/*! Returns the listener that matches the specified port/token pair.
	Must be called with monitors lock hold.
*/
monitor_listener*
NodeMonitorService::_MonitorListenerFor(node_monitor* monitor,
	NotificationListener& notificationListener)
{
	MonitorListenerList::Iterator iterator
		= monitor->listeners.GetIterator();

	while (monitor_listener* listener = iterator.Next()) {
		// does this listener match?
		if (*listener->listener == notificationListener)
			return listener;
	}

	return NULL;
}


status_t
NodeMonitorService::_AddMonitorListener(io_context *context,
	node_monitor* monitor, uint32 flags,
	NotificationListener& notificationListener)
{
	monitor_listener *listener = new(std::nothrow) monitor_listener;
	if (listener == NULL) {
		// no memory for the listener, so remove the monitor as well if needed
		if (monitor->listeners.IsEmpty())
			_RemoveMonitor(monitor, flags);

		return B_NO_MEMORY;
	}

	// initialize listener, and add it to the lists
	listener->listener = &notificationListener;
	listener->flags = flags;
	listener->monitor = monitor;

	monitor->listeners.Add(listener);
	list_add_link_to_head(&context->node_monitors, listener);

	context->num_monitors++;
	return B_OK;
}


status_t
NodeMonitorService::AddListener(io_context *context, dev_t device, ino_t node,
	uint32 flags, NotificationListener& notificationListener)
{
	TRACE(("%s(dev = %ld, node = %Ld, flags = %ld, listener = %p\n",
		__PRETTY_FUNCTION__, device, node, flags, &notificationListener));

	RecursiveLocker _(fRecursiveLock);

	node_monitor *monitor;
	status_t status = _GetMonitor(context, device, node, true, &monitor,
		(flags & B_WATCH_VOLUME) != 0);
	if (status < B_OK)
		return status;

	// add listener

	return _AddMonitorListener(context, monitor, flags, notificationListener);
}


status_t
NodeMonitorService::_UpdateListener(io_context *context, dev_t device,
	ino_t node, uint32 flags, bool addFlags,
	NotificationListener& notificationListener)
{
	TRACE(("%s(dev = %ld, node = %Ld, flags = %ld, listener = %p\n",
		__PRETTY_FUNCTION__, device, node, flags, &notificationListener));

	RecursiveLocker _(fRecursiveLock);

	node_monitor *monitor;
	status_t status = _GetMonitor(context, device, node, false, &monitor,
		(flags & B_WATCH_VOLUME) != 0);
	if (status < B_OK)
		return status;

	MonitorListenerList::Iterator iterator = monitor->listeners.GetIterator();
	while (monitor_listener* listener = iterator.Next()) {
		if (*listener->listener == notificationListener) {
			if (addFlags)
				listener->flags |= flags;
			else
				listener->flags = flags;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


/*!	\brief Given device and node ID and a node monitoring event mask, the
		   function checks whether there are listeners interested in any of
		   the events for that node and, if so, adds the respective listener
		   list to a supplied array of listener lists.

	Note, that in general not all of the listeners in an appended list will be
	interested in the events, but it is guaranteed that
	interested_monitor_listener_list::first_listener is indeed
	the first listener in the list, that is interested.

	\param device The ID of the mounted FS, the node lives in.
	\param node The ID of the node.
	\param flags The mask specifying the events occurred for the given node
		   (a combination of \c B_WATCH_* constants).
	\param interestedListeners An array of listener lists. If there are
		   interested listeners for the node, the list will be appended to
		   this array.
	\param interestedListenerCount The number of elements in the
		   \a interestedListeners array. Will be incremented, if a list is
		   appended.
*/
void
NodeMonitorService::_GetInterestedMonitorListeners(dev_t device, ino_t node,
	uint32 flags, interested_monitor_listener_list *interestedListeners,
	int32 &interestedListenerCount)
{
	// get the monitor for the node
	node_monitor *monitor = _MonitorFor(device, node, false);
	if (monitor == NULL)
		return;

	// iterate through the listeners until we find one with matching flags
	MonitorListenerList::Iterator iterator = monitor->listeners.GetIterator();
	while (monitor_listener *listener = iterator.Next()) {
		if (listener->flags & flags) {
			interested_monitor_listener_list &list
				= interestedListeners[interestedListenerCount++];
			list.iterator = iterator;
			list.flags = flags;
			return;
		}
	}
}


void
NodeMonitorService::_GetInterestedVolumeListeners(dev_t device, uint32 flags,
	interested_monitor_listener_list *interestedListeners,
	int32 &interestedListenerCount)
{
	// get the monitor for the node
	node_monitor *monitor = _MonitorFor(device, -1, true);
	if (monitor == NULL)
		return;

	// iterate through the listeners until we find one with matching flags
	MonitorListenerList::Iterator iterator = monitor->listeners.GetIterator();
	while (monitor_listener *listener = iterator.Next()) {
		if (listener->flags & flags) {
			interested_monitor_listener_list &list
				= interestedListeners[interestedListenerCount++];
			list.iterator = iterator;
			list.flags = flags;
			return;
		}
	}
}


/*!	\brief Sends a notifcation message to the given listeners.
	\param message The message to be sent.
	\param interestedListeners An array of listener lists.
	\param interestedListenerCount The number of elements in the
		   \a interestedListeners array.
	\return
	- \c B_OK, if everything went fine,
	- another error code otherwise.
*/
status_t
NodeMonitorService::_SendNotificationMessage(KMessage &message,
	interested_monitor_listener_list *interestedListeners,
	int32 interestedListenerCount)
{
	// iterate through the lists
	interested_monitor_listener_list *list = interestedListeners;
	for (int32 i = 0; i < interestedListenerCount; i++, list++) {
		// iterate through the listeners
		MonitorListenerList::Iterator iterator = list->iterator;
		do {
			monitor_listener *listener = iterator.Current();
			if (listener->flags & list->flags)
				listener->listener->EventOccurred(*this, &message);
		} while (iterator.Next() != NULL);
	}

	list = interestedListeners;
	for (int32 i = 0; i < interestedListenerCount; i++, list++) {
		// iterate through the listeners
		do {
			monitor_listener *listener = list->iterator.Current();
			if (listener->flags & list->flags)
				listener->listener->AllListenersNotified(*this);
		} while (list->iterator.Next() != NULL);
	}

	return B_OK;
}


/*!	\brief Notifies all interested listeners that an entry has been created
		   or removed.
	\param opcode \c B_ENTRY_CREATED or \c B_ENTRY_REMOVED.
	\param device The ID of the mounted FS, the entry lives/lived in.
	\param directory The entry's parent directory ID.
	\param name The entry's name.
	\param node The ID of the node the entry refers/referred to.
	\return
	- \c B_OK, if everything went fine,
	- another error code otherwise.
*/
status_t
NodeMonitorService::NotifyEntryCreatedOrRemoved(int32 opcode, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	if (!name)
		return B_BAD_VALUE;

	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the volume
	_GetInterestedVolumeListeners(device, B_WATCH_NAME,
		interestedListeners, interestedListenerCount);
	// ... for the node
	if (opcode != B_ENTRY_CREATED) {
		_GetInterestedMonitorListeners(device, node, B_WATCH_NAME,
			interestedListeners, interestedListenerCount);
	}
	// ... for the directory
	_GetInterestedMonitorListeners(device, directory, B_WATCH_DIRECTORY,
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

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


inline status_t
NodeMonitorService::NotifyEntryMoved(dev_t device, ino_t fromDirectory,
	const char *fromName, ino_t toDirectory, const char *toName,
	ino_t node)
{
	if (!fromName || !toName)
		return B_BAD_VALUE;

	// If node is a mount point, we need to resolve it to the mounted
	// volume's root node.
	dev_t nodeDevice = device;
	vfs_resolve_vnode_to_covering_vnode(device, node, &nodeDevice, &node);

	move_fd_path(device, node, fromName, toDirectory, toName);

	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[4];
	int32 interestedListenerCount = 0;
	// ... for the volume
	_GetInterestedVolumeListeners(device, B_WATCH_NAME,
		interestedListeners, interestedListenerCount);
	// ... for the node
	_GetInterestedMonitorListeners(nodeDevice, node, B_WATCH_NAME,
		interestedListeners, interestedListenerCount);
	// ... for the source directory
	_GetInterestedMonitorListeners(device, fromDirectory, B_WATCH_DIRECTORY,
		interestedListeners, interestedListenerCount);
	// ... for the target directory
	if (toDirectory != fromDirectory) {
		_GetInterestedMonitorListeners(device, toDirectory, B_WATCH_DIRECTORY,
			interestedListeners, interestedListenerCount);
	}

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

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


inline status_t
NodeMonitorService::NotifyStatChanged(dev_t device, ino_t node,
	uint32 statFields)
{
	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the volume
	_GetInterestedVolumeListeners(device, B_WATCH_STAT,
		interestedListeners, interestedListenerCount);
	// ... for the node, depending on whether its an interim update or not
	_GetInterestedMonitorListeners(device, node,
		(statFields & B_STAT_INTERIM_UPDATE) != 0
			? B_WATCH_INTERIM_STAT : B_WATCH_STAT,
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

	read_lock_fd_paths();
	fd_paths* fsPaths = lookup_fd_paths(device, node);
	if (fsPaths != NULL) {
		PathInfoList::Iterator it(&fsPaths->paths);
		for (PathInfo* info = it.Next(); info != NULL; info = it.Next()) {
			message.AddInt64("directory", info->directory);
			message.AddString("name", info->filename);
		}
	}
	read_unlock_fd_paths();

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


/*!	\brief Notifies all interested listeners that a node attribute has changed.
	\param device The ID of the mounted FS, the node lives in.
	\param node The ID of the node.
	\param attribute The attribute's name.
	\param cause Either of \c B_ATTR_CREATED, \c B_ATTR_REMOVED, or
		   \c B_ATTR_CHANGED, indicating what exactly happened to the attribute.
	\return
	- \c B_OK, if everything went fine,
	- another error code otherwise.
*/
status_t
NodeMonitorService::NotifyAttributeChanged(dev_t device, ino_t node,
	const char *attribute, int32 cause)
{
	if (!attribute)
		return B_BAD_VALUE;

	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	// ... for the volume
	_GetInterestedVolumeListeners(device, B_WATCH_ATTR,
		interestedListeners, interestedListenerCount);
	// ... for the node
	_GetInterestedMonitorListeners(device, node, B_WATCH_ATTR,
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

	read_lock_fd_paths();
	fd_paths* fsPaths = lookup_fd_paths(device, node);
	if (fsPaths != NULL) {
		PathInfoList::Iterator it(&fsPaths->paths);
		for (PathInfo* info = it.Next(); info != NULL; info = it.Next()) {
			message.AddInt64("directory", info->directory);
			message.AddString("name", info->filename);
		}
	}
	read_unlock_fd_paths();

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


inline status_t
NodeMonitorService::NotifyUnmount(dev_t device)
{
	TRACE(("unmounted device: %ld\n", device));

	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	_GetInterestedMonitorListeners(-1, -1, B_WATCH_MOUNT,
		interestedListeners, interestedListenerCount);

	if (interestedListenerCount == 0)
		return B_OK;

	// there are interested listeners: construct the message and send it
	char messageBuffer[96];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_NODE_MONITOR);
	message.AddInt32("opcode", B_DEVICE_UNMOUNTED);
	message.AddInt32("device", device);

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


inline status_t
NodeMonitorService::NotifyMount(dev_t device, dev_t parentDevice,
	ino_t parentDirectory)
{
	TRACE(("mounted device: %ld, parent %ld:%Ld\n", device, parentDevice,
		parentDirectory));

	RecursiveLocker locker(fRecursiveLock);

	// get the lists of all interested listeners
	interested_monitor_listener_list interestedListeners[3];
	int32 interestedListenerCount = 0;
	_GetInterestedMonitorListeners(-1, -1, B_WATCH_MOUNT,
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

	return _SendNotificationMessage(message, interestedListeners,
		interestedListenerCount);
}


inline status_t
NodeMonitorService::RemoveListeners(io_context *context)
{
	RecursiveLocker locker(fRecursiveLock);

	while (!list_is_empty(&context->node_monitors)) {
		// the _RemoveListener() method will also free the node_monitor
		// if it doesn't have any listeners attached anymore
		_RemoveListener(
			(monitor_listener*)list_get_first_item(&context->node_monitors));
	}

	return B_OK;
}


status_t
NodeMonitorService::AddListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	io_context *context = get_current_io_context(
		eventSpecifier->GetBool("kernel", true));

	dev_t device = eventSpecifier->GetInt32("device", -1);
	ino_t node = eventSpecifier->GetInt64("node", -1);
	uint32 flags = eventSpecifier->GetInt32("flags", 0);

	return AddListener(context, device, node, flags, listener);
}


status_t
NodeMonitorService::UpdateListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	io_context *context = get_current_io_context(
		eventSpecifier->GetBool("kernel", true));

	dev_t device = eventSpecifier->GetInt32("device", -1);
	ino_t node = eventSpecifier->GetInt64("node", -1);
	uint32 flags = eventSpecifier->GetInt32("flags", 0);
	bool addFlags = eventSpecifier->GetBool("add flags", false);

	return _UpdateListener(context, device, node, flags, addFlags, listener);
}


status_t
NodeMonitorService::RemoveListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	io_context *context = get_current_io_context(
		eventSpecifier->GetBool("kernel", true));

	dev_t device = eventSpecifier->GetInt32("device", -1);
	ino_t node = eventSpecifier->GetInt64("node", -1);

	return RemoveListener(context, device, node, listener);
}


status_t
NodeMonitorService::RemoveListener(io_context *context, dev_t device,
	ino_t node, NotificationListener& notificationListener)
{
	TRACE(("%s(dev = %ld, node = %Ld, listener = %p\n",
		__PRETTY_FUNCTION__, device, node, &notificationListener));

	RecursiveLocker _(fRecursiveLock);

	if (_RemoveListener(context, device, node, notificationListener, false)
		== B_OK)
		return B_OK;

	return _RemoveListener(context, device, node, notificationListener, true);
}


inline status_t
NodeMonitorService::RemoveUserListeners(struct io_context *context,
	port_id port, uint32 token)
{
	UserNodeListener userListener(port, token);
	monitor_listener *listener = NULL;
	int32 count = 0;

	RecursiveLocker _(fRecursiveLock);

	while ((listener = (monitor_listener*)list_get_next_item(
			&context->node_monitors, listener)) != NULL) {
		monitor_listener *removeListener;

		if (*listener->listener != userListener)
			continue;

		listener = (monitor_listener*)list_get_prev_item(
			&context->node_monitors, removeListener = listener);
			// this line sets the listener one item back, allowing us
			// to remove its successor (which is saved in "removeListener")

		_RemoveListener(removeListener);
		count++;
	}

	return count > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
NodeMonitorService::UpdateUserListener(io_context *context, dev_t device,
	ino_t node, uint32 flags, UserNodeListener& userListener)
{
	TRACE(("%s(dev = %ld, node = %Ld, flags = %ld, listener = %p\n",
		__PRETTY_FUNCTION__, device, node, flags, &userListener));

	RecursiveLocker _(fRecursiveLock);

	node_monitor *monitor;
	status_t status = _GetMonitor(context, device, node, true, &monitor,
		(flags & B_WATCH_VOLUME) != 0);
	if (status < B_OK)
		return status;

	MonitorListenerList::Iterator iterator = monitor->listeners.GetIterator();
	while (monitor_listener* listener = iterator.Next()) {
		if (*listener->listener == userListener) {
			listener->flags |= flags;
			return B_OK;
		}
	}

	UserNodeListener* copiedListener = new(std::nothrow) UserNodeListener(
		userListener);
	if (copiedListener == NULL) {
		if (monitor->listeners.IsEmpty())
			_RemoveMonitor(monitor, flags);
		return B_NO_MEMORY;
	}

	status = _AddMonitorListener(context, monitor, flags, *copiedListener);
	if (status != B_OK)
		delete copiedListener;

	return status;
}


//	#pragma mark - private kernel API


status_t
remove_node_monitors(struct io_context *context)
{
	return sNodeMonitorService.RemoveListeners(context);
}


status_t
node_monitor_init(void)
{
	new(&sNodeMonitorSender) UserMessagingMessageSender();
	new(&sNodeMonitorService) NodeMonitorService();

	if (sNodeMonitorService.InitCheck() < B_OK)
		panic("initializing node monitor failed\n");

	return B_OK;
}


status_t
notify_unmount(dev_t device)
{
	return sNodeMonitorService.NotifyUnmount(device);
}


status_t
notify_mount(dev_t device, dev_t parentDevice, ino_t parentDirectory)
{
	return sNodeMonitorService.NotifyMount(device, parentDevice,
		parentDirectory);
}


status_t
remove_node_listener(dev_t device, ino_t node, NotificationListener& listener)
{
	return sNodeMonitorService.RemoveListener(get_current_io_context(true),
		device, node, listener);
}


status_t
add_node_listener(dev_t device, ino_t node, uint32 flags,
	NotificationListener& listener)
{
	return sNodeMonitorService.AddListener(get_current_io_context(true),
		device, node, flags, listener);
}


//	#pragma mark - public kernel API


/*!	\brief Notifies all interested listeners that an entry has been created.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param directory The entry's parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_entry_created(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	return sNodeMonitorService.NotifyEntryCreatedOrRemoved(B_ENTRY_CREATED,
		device, directory, name, node);
}


/*!	\brief Notifies all interested listeners that an entry has been removed.
  	\param device The ID of the mounted FS, the entry lived in.
  	\param directory The entry's former parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry referred to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_entry_removed(dev_t device, ino_t directory, const char *name,
	ino_t node)
{
	return sNodeMonitorService.NotifyEntryCreatedOrRemoved(B_ENTRY_REMOVED,
		device, directory, name, node);
}


/*!	\brief Notifies all interested listeners that an entry has been moved.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param fromDirectory The entry's previous parent directory ID.
  	\param fromName The entry's previous name.
  	\param toDirectory The entry's new parent directory ID.
  	\param toName The entry's new name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_entry_moved(dev_t device, ino_t fromDirectory,
	const char *fromName, ino_t toDirectory, const char *toName,
	ino_t node)
{
	return sNodeMonitorService.NotifyEntryMoved(device, fromDirectory,
		fromName, toDirectory, toName, node);
}


/*!	\brief Notifies all interested listeners that a node's stat data have
  		   changed.
  	\param device The ID of the mounted FS, the node lives in.
  	\param node The ID of the node.
  	\param statFields A bitwise combination of one or more of the \c B_STAT_*
  		   constants defined in <NodeMonitor.h>, indicating what fields of the
  		   stat data have changed.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_stat_changed(dev_t device, ino_t node, uint32 statFields)
{
	return sNodeMonitorService.NotifyStatChanged(device, node, statFields);
}


/*!	\brief Notifies all interested listeners that a node attribute has changed.
  	\param device The ID of the mounted FS, the node lives in.
  	\param node The ID of the node.
  	\param attribute The attribute's name.
  	\param cause Either of \c B_ATTR_CREATED, \c B_ATTR_REMOVED, or
  		   \c B_ATTR_CHANGED, indicating what exactly happened to the attribute.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_attribute_changed(dev_t device, ino_t node, const char *attribute,
	int32 cause)
{
	return sNodeMonitorService.NotifyAttributeChanged(device, node, attribute,
		cause);
}


/*!	\brief Notifies the listener of a live query that an entry has been added
  		   to the query (for whatever reason).
  	\param port The target port of the listener.
  	\param token The BHandler token of the listener.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param directory The entry's parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_query_entry_created(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	return notify_query_entry_event(B_ENTRY_CREATED, port, token,
		device, directory, name, node);
}


/*!	\brief Notifies the listener of a live query that an entry has been removed
  		   from the query (for whatever reason).
  	\param port The target port of the listener.
  	\param token The BHandler token of the listener.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param directory The entry's parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_query_entry_removed(port_id port, int32 token, dev_t device,
	ino_t directory, const char *name, ino_t node)
{
	return notify_query_entry_event(B_ENTRY_REMOVED, port, token,
		device, directory, name, node);
}


/*!	\brief Notifies the listener of a live query that an entry has been changed
  		   and is still in the query (for whatever reason).
  	\param port The target port of the listener.
  	\param token The BHandler token of the listener.
  	\param device The ID of the mounted FS, the entry lives in.
  	\param directory The entry's parent directory ID.
  	\param name The entry's name.
  	\param node The ID of the node the entry refers to.
  	\return
  	- \c B_OK, if everything went fine,
  	- another error code otherwise.
*/
status_t
notify_query_attr_changed(port_id port, int32 token, dev_t device,
	ino_t directory, const char* name, ino_t node)
{
	return notify_query_entry_event(B_ATTR_CHANGED, port, token,
		device, directory, name, node);
}


//	#pragma mark - User syscalls


status_t
_user_stop_notifying(port_id port, uint32 token)
{
	io_context *context = get_current_io_context(false);

	return sNodeMonitorService.RemoveUserListeners(context, port, token);
}


status_t
_user_start_watching(dev_t device, ino_t node, uint32 flags, port_id port,
	uint32 token)
{
	io_context *context = get_current_io_context(false);

	UserNodeListener listener(port, token);
	return sNodeMonitorService.UpdateUserListener(context, device, node, flags,
		listener);
}


status_t
_user_stop_watching(dev_t device, ino_t node, port_id port, uint32 token)
{
	io_context *context = get_current_io_context(false);

	UserNodeListener listener(port, token);
	return sNodeMonitorService.RemoveListener(context, device, node,
		listener);
}

