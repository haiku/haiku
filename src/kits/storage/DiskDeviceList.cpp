//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new.h>

#include <DiskDevice.h>
#include <DiskDeviceList.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceRoster.h>
#include <Locker.h>
#include <Looper.h>
#include <Partition.h>
#include <Session.h>

// constructor
/*!	\brief Creates an empty BDiskDeviceList object.
*/
BDiskDeviceList::BDiskDeviceList(bool useOwnLocker)
	: fLocker(NULL),
	  fDevices(20, true),
	  fSubscribed(false)
{
	if (useOwnLocker)
		fLocker = new(nothrow) BLocker;
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
BDiskDeviceList::~BDiskDeviceList()
{
	if (fLocker)
		delete fLocker;
}

// MessageReceived
/*!	\brief Implemented to handle notification messages.
*/
void
BDiskDeviceList::MessageReceived(BMessage *message)
{
	bool locked = Lock();
	switch (message->what) {
		case B_DEVICE_UPDATE:
		{
			uint32 event;
			if (message->FindInt32("event", (int32*)&event) == B_OK) {
				switch (event) {
					case B_DEVICE_MOUNT_POINT_MOVED:
						_MountPointMoved(message);
						break;
					case B_DEVICE_PARTITION_MOUNTED:
						_PartitionMounted(message);
						break;
					case B_DEVICE_PARTITION_UNMOUNTED:
						_PartitionUnmounted(message);
						break;
					case B_DEVICE_PARTITION_CHANGED:
						_PartitionChanged(message);
						break;
					case B_DEVICE_PARTITION_ADDED:
						_PartitionAdded(message);
						break;
					case B_DEVICE_PARTITION_REMOVED:
						_PartitionRemoved(message);
						break;
					case B_DEVICE_SESSION_ADDED:
						_SessionAdded(message);
						break;
					case B_DEVICE_SESSION_REMOVED:
						_SessionRemoved(message);
						break;
					case B_DEVICE_MEDIA_CHANGED:
						_MediaChanged(message);
						break;
					case B_DEVICE_ADDED:
						_DeviceAdded(message);
						break;
					case B_DEVICE_REMOVED:
						_DeviceRemoved(message);
						break;
				}
			}
		}
		default:
			BHandler::MessageReceived(message);
	}
	if (locked)
		Unlock();
}

// SetNextHandler
/*!	\brief Implemented to unsubscribe from notification services when going
		   to be detached from looper.
*/
void
BDiskDeviceList::SetNextHandler(BHandler *handler)
{
	if (!handler && fSubscribed) {
		bool locked = Lock();
		BDiskDeviceRoster().StopWatching(BMessenger(this));
		fSubscribed = false;
		if (locked)
			Unlock();
	}
}

// Fetch
/*!	\brief Empties the list and refills it according to the current state.

	Furthermore, if added to a looper, the list subscribes to notification
	services needed to keep the list up-to-date.

	If an error occurs, the list Unset()s itself.

	The object doesn't need to be locked, when this method is invoked. The
	method does itself try to lock the list, but doesn't fail, if that
	doesn't succeed. That way an object can be used without locking in a
	single threaded environment.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceList::Fetch()
{
	Unset();
	bool locked = Lock();
	// get the devices
	status_t error = B_OK;
	BDiskDeviceRoster roster;
	do {
		if (BDiskDevice *device = new(nothrow) BDiskDevice) {
			status_t status = roster.GetNextDevice(device);
			if (status == B_OK)
				fDevices.AddItem(device);
			else if (status == B_ENTRY_NOT_FOUND)
				break;
			else
				error = status;
		} else
			error = B_NO_MEMORY;
	} while (error == B_OK);
	// register for notifications
	if (error == B_OK && Looper()) {
		error = roster.StartWatching(BMessenger(this));
		fSubscribed = (error == B_OK);
	}
	// cleanup on error
	if (error != B_OK)
		Unset();
	if (locked)
		Unlock();
	return error;
}

// Unset
/*!	\brief Empties the list and unsubscribes from all notification services.

	The object doesn't need to be locked, when this method is invoked. The
	method does itself try to lock the list, but doesn't fail, if that
	doesn't succeed. That way an object can be used without locking in a
	single threaded environment.
*/
void
BDiskDeviceList::Unset()
{
	bool locked = Lock();
	// unsubscribe from notification services
	if (fSubscribed) {
		BDiskDeviceRoster().StopWatching(BMessenger(this));
		fSubscribed = false;
	}
	// empty the list
	fDevices.MakeEmpty();
	if (locked)
		Unlock();
}

// Lock
/*!	\brief Locks the list.

	If on construction it had been specified, that the list shall use an
	own BLocker, then this locker is locked, otherwise LockLooper() is
	invoked.

	\return \c true, if the list could be locked successfully, \c false
			otherwise.
*/
bool
BDiskDeviceList::Lock()
{
	if (fLocker)
		return fLocker->Lock();
	return LockLooper();
}

// Unlock
/*!	\brief Unlocks the list.

	If on construction it had been specified, that the list shall use an
	own BLocker, then this locker is unlocked, otherwise UnlockLooper() is
	invoked.
*/
void
BDiskDeviceList::Unlock()
{
	if (fLocker)
		return fLocker->Unlock();
	return UnlockLooper();
}

// CountDevices
/*!	\brief Returns the number of devices in the list.

	The list must be locked.

	\return The number of devices in the list.
*/
int32
BDiskDeviceList::CountDevices() const
{
	return fDevices.CountItems();
}

// DeviceAt
/*!	\brief Retrieves a device by index.

	The list must be locked.

	\param index The list index of the device to be returned.
	\return The device with index \a index, or \c NULL, if the list is not
			locked or \a index is out of range.
*/
BDiskDevice *
BDiskDeviceList::DeviceAt(int32 index) const
{
	return fDevices.ItemAt(index);
}

// VisitEachDevice
/*!	\brief Iterates through the all devices in the list.

	The supplied visitor's Visit(BDiskDevice*) is invoked for each device.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective device.

	The list must be locked.

	\param visitor The visitor.
	\return The respective device, if the iteration was terminated early,
			\c NULL otherwise.
*/
BDiskDevice *
BDiskDeviceList::VisitEachDevice(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		for (int32 i = 0; BDiskDevice *device = DeviceAt(i); i++) {
			if (visitor->Visit(device))
				return device;
		}
	}
	return NULL;
}

// VisitEachSession
/*!	\brief Iterates through the all devices' sessions.

	The supplied visitor's Visit(BSession*) is invoked for each session.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective session.

	The list must be locked.

	\param visitor The visitor.
	\return The respective session, if the iteration was terminated early,
			\c NULL otherwise.
*/
BSession *
BDiskDeviceList::VisitEachSession(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		for (int32 i = 0; BDiskDevice *device = DeviceAt(i); i++) {
			if (BSession *session = device->VisitEachSession(visitor))
				return session;
		}
	}
	return NULL;
}

// VisitEachPartition
/*!	\brief Iterates through the all devices' partitions.

	The supplied visitor's Visit(BPartition*) is invoked for each partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective partition.

	The list must be locked.

	\param visitor The visitor.
	\return The respective partition, if the iteration was terminated early,
			\c NULL otherwise.
*/
BPartition *
BDiskDeviceList::VisitEachPartition(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		for (int32 i = 0; BDiskDevice *device = DeviceAt(i); i++) {
			if (BPartition *partition = device->VisitEachPartition(visitor))
				return partition;
		}
	}
	return NULL;
}

// Traverse
/*!	\brief Pre-order traverses the trees of the spanned by the BDiskDevices
		   and their subobjects.

	The supplied visitor's Visit() is invoked for each device, for each
	session and for each partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true as well.

	\param visitor The visitor.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceList::Traverse(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		for (int32 i = 0; BDiskDevice *device = DeviceAt(i); i++) {
			if (device->Traverse(visitor))
				return true;
		}
	}
	return false;
}

// VisitEachMountedPartition
/*!	\brief Iterates through the all devices' partitions that are mounted.

	The supplied visitor's Visit(BPartition*) is invoked for each mounted
	partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective partition.

	The list must be locked.

	\param visitor The visitor.
	\return The respective partition, if the iteration was terminated early,
			\c NULL otherwise.
*/
BPartition *
BDiskDeviceList::VisitEachMountedPartition(BDiskDeviceVisitor *visitor)
{
	BPartition *partition = NULL;
	if (visitor) {
		struct MountedPartitionFilter : public PartitionFilter {
			virtual bool Filter(BPartition *partition)
				{ return partition->IsMounted(); }
		} filter;
		PartitionFilterVisitor filterVisitor(visitor, &filter);
		partition = VisitEachPartition(&filterVisitor);
	}
	return partition;
}

// VisitEachMountablePartition
/*!	\brief Iterates through the all devices' partitions that are mountable.

	The supplied visitor's Visit(BPartition*) is invoked for each mountable
	partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective partition.

	The list must be locked.

	\param visitor The visitor.
	\return The respective partition, if the iteration was terminated early,
			\c NULL otherwise.
*/
BPartition *
BDiskDeviceList::VisitEachMountablePartition(BDiskDeviceVisitor *visitor)
{
	BPartition *partition = NULL;
	if (visitor) {
		struct MountablePartitionFilter : public PartitionFilter {
			virtual bool Filter(BPartition *partition)
				{ return partition->ContainsFileSystem(); }
		} filter;
		PartitionFilterVisitor filterVisitor(visitor, &filter);
		partition = VisitEachPartition(&filterVisitor);
	}
	return partition;
}

// VisitEachInitializablePartition
/*!	\brief Iterates through the all devices' partitions that are initializable.

	The supplied visitor's Visit(BPartition*) is invoked for each
	initializable partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns the respective partition.

	The list must be locked.

	\param visitor The visitor.
	\return The respective partition, if the iteration was terminated early,
			\c NULL otherwise.
*/
BPartition *
BDiskDeviceList::VisitEachInitializablePartition(BDiskDeviceVisitor *visitor)
{
	BPartition *partition = NULL;
	if (visitor) {
		struct InitializablePartitionFilter : public PartitionFilter {
			virtual bool Filter(BPartition *partition)
				{ return !partition->IsHidden(); }
		} filter;
		PartitionFilterVisitor filterVisitor(visitor, &filter);
		partition = VisitEachPartition(&filterVisitor);
	}
	return partition;
}

// DeviceWithID
/*!	\brief Retrieves a device by ID.

	The list must be locked.

	\param id The ID of the device to be returned.
	\return The device with ID \a id, or \c NULL, if the list is not
			locked or no device with ID \a id is in the list.
*/
BDiskDevice *
BDiskDeviceList::DeviceWithID(int32 id) const
{
	IDFinderVisitor visitor(id);
	return const_cast<BDiskDeviceList*>(this)->VisitEachDevice(&visitor);
}

// SessionWithID
/*!	\brief Retrieves a session by ID.

	The list must be locked.

	\param id The ID of the session to be returned.
	\return The session with ID \a id, or \c NULL, if the list is not
			locked or no session with ID \a id is in the list.
*/
BSession *
BDiskDeviceList::SessionWithID(int32 id) const
{
	IDFinderVisitor visitor(id);
	return const_cast<BDiskDeviceList*>(this)->VisitEachSession(&visitor);
}

// PartitionWithID
/*!	\brief Retrieves a partition by ID.

	The list must be locked.

	\param id The ID of the partition to be returned.
	\return The partition with ID \a id, or \c NULL, if the list is not
			locked or no partition with ID \a id is in the list.
*/
BPartition *
BDiskDeviceList::PartitionWithID(int32 id) const
{
	IDFinderVisitor visitor(id);
	return const_cast<BDiskDeviceList*>(this)->VisitEachPartition(&visitor);
}

// MountPointMoved
/*!	\brief Invoked, when the mount point of a partition has been moved.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::MountPointMoved(BPartition *partition)
{
}

// PartitionMounted
/*!	\brief Invoked, when a partition has been mounted.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionMounted(BPartition *partition)
{
}

// PartitionUnmounted
/*!	\brief Invoked, when a partition has been unmounted.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionUnmounted(BPartition *partition)
{
}

// PartitionChanged
/*!	\brief Invoked, when data concerning a partition have changed.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionChanged(BPartition *partition)
{
}

// PartitionAdded
/*!	\brief Invoked, when a partition has been added.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionAdded(BPartition *partition)
{
}

// PartitionRemoved
/*!	\brief Invoked, when a partition has been removed.

	The supplied object is already removed from the list and is going to be
	deleted after the hook returns.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionRemoved(BPartition *partition)
{
}

// SessionAdded
/*!	\brief Invoked, when a session has been added.

	The list is locked, when this method is invoked.

	\param session The concerned session.
*/
void
BDiskDeviceList::SessionAdded(BSession *session)
{
}

// SessionRemoved
/*!	\brief Invoked, when a session has been removed.

	The supplied object is already removed from the list and is going to be
	deleted after the hook returns.

	The list is locked, when this method is invoked.

	\param session The concerned session.
*/
void
BDiskDeviceList::SessionRemoved(BSession *session)
{
}

// MediaChanged
/*!	\brief Invoked, when the media of a device has been changed.

	The list is locked, when this method is invoked.

	\param device The concerned device.
*/
void
BDiskDeviceList::MediaChanged(BDiskDevice *device)
{
}

// DeviceAdded
/*!	\brief Invoked, when a device has been added.

	The list is locked, when this method is invoked.

	\param device The concerned device.
*/
void
BDiskDeviceList::DeviceAdded(BDiskDevice *device)
{
}

// DeviceRemoved
/*!	\brief Invoked, when a device has been removed.

	The supplied object is already removed from the list and is going to be
	deleted after the hook returns.

	The list is locked, when this method is invoked.

	\param device The concerned device.
*/
void
BDiskDeviceList::DeviceRemoved(BDiskDevice *device)
{
}

// _MountPointMoved
/*!	\brief Handles a "mount point moved" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_MountPointMoved(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BPartition *partition = _FindPartition(message))
			MountPointMoved(partition);
	}
}

// _PartitionMounted
/*!	\brief Handles a "partition mounted" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionMounted(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BPartition *partition = _FindPartition(message))
			PartitionMounted(partition);
	}
}

// _PartitionUnmounted
/*!	\brief Handles a "partition unmounted" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionUnmounted(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BPartition *partition = _FindPartition(message))
			PartitionUnmounted(partition);
	}
}

// _PartitionChanged
/*!	\brief Handles a "partition changed" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionChanged(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BPartition *partition = _FindPartition(message))
			PartitionChanged(partition);
	}
}

// _PartitionAdded
/*!	\brief Handles a "partition added" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionAdded(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BPartition *partition = _FindPartition(message))
			PartitionAdded(partition);
	}
}

// _PartitionRemoved
/*!	\brief Handles a "partition removed" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionRemoved(BMessage *message)
{
	if (BPartition *partition = _FindPartition(message)) {
		partition->Session()->fPartitions.RemoveItem(partition, false);
		if (_UpdateDevice(message) && !_FindPartition(message))
			PartitionRemoved(partition);
		delete partition;
	}
}

// _SessionAdded
/*!	\brief Handles a "session added" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_SessionAdded(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message)) {
		if (BSession *session = _FindSession(message))
			SessionAdded(session);
	}
}

// _SessionRemoved
/*!	\brief Handles a "session removed" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_SessionRemoved(BMessage *message)
{
	if (BSession *session = _FindSession(message)) {
		session->Device()->fSessions.RemoveItem(session, false);
		if (_UpdateDevice(message) && !_FindSession(message))
			SessionRemoved(session);
		delete session;
	}
}

// _MediaChanged
/*!	\brief Handles a "media changed" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_MediaChanged(BMessage *message)
{
	if (BDiskDevice *device = _UpdateDevice(message))
		MediaChanged(device);
}

// _DeviceAdded
/*!	\brief Handles a "device added" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_DeviceAdded(BMessage *message)
{
	int32 id;
	if (message->FindInt32("device_id", &id) == B_OK && !DeviceWithID(id)) {
		BDiskDevice *device = new(nothrow) BDiskDevice;
		if (BDiskDeviceRoster().GetDeviceWithID(id, device) == B_OK) {
			fDevices.AddItem(device);
			DeviceAdded(device);
		} else
			delete device;
	}
}

// _DeviceRemoved
/*!	\brief Handles a "device removed" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_DeviceRemoved(BMessage *message)
{
	if (BDiskDevice *device = _FindDevice(message)) {
		fDevices.RemoveItem(device, false);
		DeviceRemoved(device);
		delete device;
	}
}

// _FindDevice
/*!	\brief Returns the device for the ID contained in a motification message.
	\param message The notification message.
	\return The device with the ID, or \c NULL, if the ID or the device could
			not be found.
*/
BDiskDevice *
BDiskDeviceList::_FindDevice(BMessage *message)
{
	BDiskDevice *device = NULL;
	int32 id;
	if (message->FindInt32("device_id", &id) == B_OK)
		device = DeviceWithID(id);
	return device;
}

// _FindSession
/*!	\brief Returns the session for the ID contained in a motification message.
	\param message The notification message.
	\return The session with the ID, or \c NULL, if the ID or the session could
			not be found.
*/
BSession *
BDiskDeviceList::_FindSession(BMessage *message)
{
	BSession *session = NULL;
	int32 id;
	if (message->FindInt32("session_id", &id) == B_OK)
		session = SessionWithID(id);
	return session;
}

// _FindPartition
/*!	\brief Returns the partition for the ID contained in a motification
		   message.
	\param message The notification message.
	\return The partition with the ID, or \c NULL, if the ID or the partition
			could not be found.*/
BPartition *
BDiskDeviceList::_FindPartition(BMessage *message)
{
	BPartition *partition = NULL;
	int32 id;
	if (message->FindInt32("partition_id", &id) == B_OK)
		partition = PartitionWithID(id);
	return partition;
}

// _UpdateDevice
/*!	\brief Finds the device for the ID contained in a motification message
		   and updates it.
	\param message The notification message.
	\return The device with the ID, or \c NULL, if the ID or the device could
			not be found.
*/
BDiskDevice *
BDiskDeviceList::_UpdateDevice(BMessage *message)
{
	BDiskDevice *device = _FindDevice(message);
	if (device) {
		if (device->Update() != B_OK) {
			fDevices.RemoveItem(device);
			device = NULL;
		}
	}
	return device;
}

