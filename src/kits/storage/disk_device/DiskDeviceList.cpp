/*
 * Copyright 2003-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */

#include <DiskDeviceList.h>

#include <AutoLocker.h>
#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceRoster.h>
#include <Locker.h>
#include <Looper.h>
#include <Partition.h>

#include <new>
using namespace std;

// constructor
/*!	\brief Creates an empty BDiskDeviceList object.
*/
BDiskDeviceList::BDiskDeviceList(bool useOwnLocker)
	: fLocker(NULL),
	  fDevices(20, true),
	  fSubscribed(false)
{
	if (useOwnLocker)
		fLocker = new(nothrow) BLocker("BDiskDeviceList_fLocker");
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
BDiskDeviceList::~BDiskDeviceList()
{
	delete fLocker;
}

// MessageReceived
/*!	\brief Implemented to handle notification messages.
*/
void
BDiskDeviceList::MessageReceived(BMessage *message)
{
	AutoLocker<BDiskDeviceList> _(this);
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
					case B_DEVICE_PARTITION_INITIALIZED:
						_PartitionInitialized(message);
						break;
					case B_DEVICE_PARTITION_RESIZED:
						_PartitionResized(message);
						break;
					case B_DEVICE_PARTITION_MOVED:
						_PartitionMoved(message);
						break;
					case B_DEVICE_PARTITION_CREATED:
						_PartitionCreated(message);
						break;
					case B_DEVICE_PARTITION_DELETED:
						_PartitionDeleted(message);
						break;
					case B_DEVICE_PARTITION_DEFRAGMENTED:
						_PartitionDefragmented(message);
						break;
					case B_DEVICE_PARTITION_REPAIRED:
						_PartitionRepaired(message);
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
}

// SetNextHandler
/*!	\brief Implemented to unsubscribe from notification services when going
		   to be detached from looper.
*/
void
BDiskDeviceList::SetNextHandler(BHandler *handler)
{
	if (!handler) {
		AutoLocker<BDiskDeviceList> _(this);
		if (fSubscribed) 
			_StopWatching();
	}
	BHandler::SetNextHandler(handler);
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
	AutoLocker<BDiskDeviceList> _(this);
	// register for notifications
	status_t error = B_OK;
	if (Looper())
		error = _StartWatching();
	// get the devices
	BDiskDeviceRoster roster;
	while (error == B_OK) {
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
	}
	// cleanup on error
	if (error != B_OK)
		Unset();
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
	AutoLocker<BDiskDeviceList> _(this);
	// unsubscribe from notification services
	_StopWatching();
	// empty the list
	fDevices.MakeEmpty();
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
			if (BPartition *partition = device->VisitEachDescendant(visitor))
				return partition;
		}
	}
	return NULL;
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
			virtual ~MountedPartitionFilter() {};
			virtual bool Filter(BPartition *partition, int32 level)
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
			virtual ~MountablePartitionFilter() {};
			virtual bool Filter(BPartition *partition, int32 level)
				{ return partition->ContainsFileSystem(); }
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
	PartitionChanged(partition, B_DEVICE_MOUNT_POINT_MOVED);
}

// PartitionMounted
/*!	\brief Invoked, when a partition has been mounted.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionMounted(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_MOUNTED);
}

// PartitionUnmounted
/*!	\brief Invoked, when a partition has been unmounted.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionUnmounted(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_UNMOUNTED);
}

// PartitionInitialized
/*!	\brief Invoked, when a partition has been initialized.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionInitialized(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_INITIALIZED);
}

// PartitionResized
/*!	\brief Invoked, when a partition has been resized.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionResized(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_RESIZED);
}

// PartitionMoved
/*!	\brief Invoked, when a partition has been moved.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionMoved(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_MOVED);
}

// PartitionCreated
/*!	\brief Invoked, when a partition has been created.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionCreated(BPartition *partition)
{
}

// PartitionDeleted
/*!	\brief Invoked, when a partition has been deleted.

	The method is called twice for a deleted partition. The first time
	before the BDiskDevice the partition belongs to has been updated. The
	\a partition parameter will point to a still valid BPartition object.
	On the second invocation the device object will have been updated and
	the partition object will have been deleted -- \a partition will be
	\c NULL then.

	The list is locked, when this method is invoked.

	\param partition The concerned partition. Only non- \c NULL on the first
		   invocation.
	\param partitionID The ID of the concerned partition.
*/
void
BDiskDeviceList::PartitionDeleted(BPartition *partition,
	partition_id partitionID)
{
}

// PartitionDefragmented
/*!	\brief Invoked, when a partition has been defragmented.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionDefragmented(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_DEFRAGMENTED);
}

// PartitionRepaired
/*!	\brief Invoked, when a partition has been repaired.

	The list is locked, when this method is invoked.

	\param partition The concerned partition.
*/
void
BDiskDeviceList::PartitionRepaired(BPartition *partition)
{
	PartitionChanged(partition, B_DEVICE_PARTITION_REPAIRED);
}

// PartitionChanged
/*!	\brief Catch-all method invoked by the \c Partition*() hooks, save by
		   PartitionCreated() and PartitionDeleted().

	If you're interested only in the fact, that something about the partition
	changed, you can just override this hook instead of the ones telling you
	exactly what happened.

	\param partition The concerned partition.
	\param event The event that occurred, if you are interested in it after all.
*/
void
BDiskDeviceList::PartitionChanged(BPartition *partition, uint32 event)
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

// _StartWatching
/*!	\brief Starts watching for disk device notifications.

	The object must be locked (if possible at all), when this method is
	invoked.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceList::_StartWatching()
{
	if (!Looper() || fSubscribed)
		return B_BAD_VALUE;
		
	status_t error = BDiskDeviceRoster().StartWatching(BMessenger(this));
	fSubscribed = (error == B_OK);
	return error;
}

// _StopWatching
/*!	\brief Stop watching for disk device notifications.

	The object must be locked (if possible at all), when this method is
	invoked.
*/
void
BDiskDeviceList::_StopWatching()
{
	if (fSubscribed) {
		BDiskDeviceRoster().StopWatching(BMessenger(this));
		fSubscribed = false;
	}
}

// _MountPointMoved
/*!	\brief Handles a "mount point moved" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_MountPointMoved(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
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
	if (_UpdateDevice(message) != NULL) {
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
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionUnmounted(partition);
	}
}

// _PartitionInitialized
/*!	\brief Handles a "partition initialized" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionInitialized(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionInitialized(partition);
	}
}

// _PartitionResized
/*!	\brief Handles a "partition resized" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionResized(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionResized(partition);
	}
}

// _PartitionMoved
/*!	\brief Handles a "partition moved" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionMoved(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionMoved(partition);
	}
}

// _PartitionCreated
/*!	\brief Handles a "partition created" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionCreated(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionCreated(partition);
	}
}

// _PartitionDeleted
/*!	\brief Handles a "partition deleted" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionDeleted(BMessage *message)
{
	if (BPartition *partition = _FindPartition(message)) {
		partition_id id = partition->ID();
		PartitionDeleted(partition, id);
		if (_UpdateDevice(message))
			PartitionDeleted(NULL, id);
	}
}

// _PartitionDefragmented
/*!	\brief Handles a "partition defragmented" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionDefragmented(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionDefragmented(partition);
	}
}

// _PartitionRepaired
/*!	\brief Handles a "partition repaired" message.
	\param message The respective notification message.
*/
void
BDiskDeviceList::_PartitionRepaired(BMessage *message)
{
	if (_UpdateDevice(message) != NULL) {
		if (BPartition *partition = _FindPartition(message))
			PartitionRepaired(partition);
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

