//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------


#include <fcntl.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Directory.h>
#include <DiskDeviceRoster.h>
#include <Drivers.h>
#include <Entry.h>
#include <Locker.h>
#include <Path.h>

#include "RDiskDeviceList.h"
#include "Debug.h"
#include "EventMaskWatcher.h"
#include "RDiskDevice.h"
#include "RPartition.h"
#include "RSession.h"
#include "WatchingService.h"

namespace RDiskDeviceListPredicates {

// CompareIDPredicate
template <typename T>
struct CompareIDPredicate : public UnaryPredicate<T> {
	CompareIDPredicate(int32 id) : fID(id) {}
	CompareIDPredicate(const T *object) : fID(object->ID()) {}

	virtual int operator()(const T *object) const
	{
		int32 id = object->ID();
		if (fID < id)
			return 1;
		if (fID > id)
			return -1;
		return 0;
	}

private:
	int32	fID;
};

// CompareDevicePathPredicate
struct CompareDevicePathPredicate : public UnaryPredicate<RDiskDevice> {
	CompareDevicePathPredicate(const char *path) : fPath(path) {}

	virtual int operator()(const RDiskDevice *device) const
	{
		return strcmp(fPath, device->Path());
	}
	
private:
	const char	*fPath;
};

// ComparePartitionPathPredicate
struct ComparePartitionPathPredicate : public UnaryPredicate<RPartition> {
	ComparePartitionPathPredicate(const char *path) : fPath(path) {}

	virtual int operator()(const RPartition *partition) const
	{
		char path[B_FILE_NAME_LENGTH];
		partition->GetPath(path);
		return strcmp(fPath, path);
	}
	
private:
	const char	*fPath;
};

}	// namespace RDiskDeviceListPredicates

using namespace RDiskDeviceListPredicates;


// constructor
RDiskDeviceList::RDiskDeviceList(BMessenger target, BLocker &lock,
								 RVolumeList &volumeList,
								 WatchingService *watchingService)
	: MessageHandler(),
	  RVolumeListListener(),
	  fLock(lock),
	  fTarget(target),
	  fVolumeList(volumeList),
	  fWatchingService(watchingService),
	  fDevices(20, true),
	  fSessions(40, false),
	  fPartitions(80, false)
{
	fVolumeList.SetListener(this);
}

// destructor
RDiskDeviceList::~RDiskDeviceList()
{
}

// HandleMessage
void
RDiskDeviceList::HandleMessage(BMessage *message)
{
	if (Lock()) {
		switch (message->what) {
			default:
				MessageHandler::HandleMessage(message);
				break;
		}
		Unlock();
	}
}

// VolumeMounted
void
RDiskDeviceList::VolumeMounted(const RVolume *volume)
{
	PRINT(("RDiskDeviceList::VolumeMounted(%ld)\n", volume->ID()));
	Lock();
	// get the partition and set its volume
	const char *devicePath = volume->DevicePath();
	if (RPartition *partition = PartitionWithPath(devicePath)) {
		partition->SetVolume(volume);
		// send notifications
		BMessage notification(B_DEVICE_UPDATE);
		if (_InitNotificationMessage(&notification,
				B_DEVICE_PARTITION_MOUNTED, B_DEVICE_CAUSE_UNKNOWN,
				partition) == B_OK) {
			EventMaskWatcherFilter filter(B_DEVICE_REQUEST_MOUNTING);
			fWatchingService->NotifyWatchers(&notification, &filter);
		}
	}
	Unlock();
}

// VolumeUnmounted
void
RDiskDeviceList::VolumeUnmounted(const RVolume *volume)
{
	PRINT(("RDiskDeviceList::VolumeUnmounted(%ld)\n", volume->ID()));
	Lock();
	// get the partition and set its volume
	const char *devicePath = volume->DevicePath();
	if (RPartition *partition = PartitionWithPath(devicePath)) {
		partition->SetVolume(NULL);
		// send notifications
		BMessage notification(B_DEVICE_UPDATE);
		if (_InitNotificationMessage(&notification,
				B_DEVICE_PARTITION_UNMOUNTED, B_DEVICE_CAUSE_UNKNOWN,
				partition) == B_OK) {
			EventMaskWatcherFilter filter(B_DEVICE_REQUEST_MOUNTING);
			fWatchingService->NotifyWatchers(&notification, &filter);
		}
	}
	Unlock();
}

// MountPointMoved
void
RDiskDeviceList::MountPointMoved(const RVolume *volume,
								 const entry_ref *oldRoot,
								 const entry_ref *newRoot)
{
	PRINT(("RDiskDeviceList::MountPointMoved(%ld)\n", volume->ID()));
	Lock();
	// get the partition
	const char *devicePath = volume->DevicePath();
	if (RPartition *partition = PartitionWithPath(devicePath)) {
		// send notifications
		BMessage notification(B_DEVICE_UPDATE);
		if (_InitNotificationMessage(&notification,
				B_DEVICE_MOUNT_POINT_MOVED, B_DEVICE_CAUSE_UNKNOWN,
				partition) == B_OK
			&& notification.AddRef("old_directory", oldRoot) == B_OK
			&& notification.AddRef("new_directory", newRoot) == B_OK) {
			EventMaskWatcherFilter filter(B_DEVICE_REQUEST_MOUNT_POINT);
			fWatchingService->NotifyWatchers(&notification, &filter);
		}
	}
	Unlock();
}

// DeviceAppeared
void
RDiskDeviceList::DeviceAppeared(const char *devicePath)
{
	status_t error = (devicePath ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int fd = open(devicePath, O_RDONLY);
		if (fd >= 0) {
			bool closeFile = true;
			device_geometry geometry;
			status_t mediaStatus = B_OK;
			partition_info partitionInfo;
			if (ioctl(fd, B_GET_MEDIA_STATUS, &mediaStatus) == 0
				&& ioctl(fd, B_GET_GEOMETRY, &geometry) == 0
				&& ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo) < 0) {
PRINT(("RDiskDeviceList::DeviceAppeared(`%s')\n", devicePath));
				RDiskDevice *device = new(nothrow) RDiskDevice;
				if (device) {
					error = device->SetTo(devicePath, fd, &geometry,
										  mediaStatus);
					if (error == B_OK) {
						closeFile = false;
						device->SetTouched(true);
						AddDevice(device);
					} else
						delete device;
				} else
					error = B_NO_MEMORY;
			}
			if (closeFile)
				close(fd);
		}
	}
}

// DeviceDisappeared
void
RDiskDeviceList::DeviceDisappeared(const char *devicePath)
{
PRINT(("RDiskDeviceList::DeviceDisappeared(`%s')\n", devicePath));
	if (devicePath) {
		if (RDiskDevice *device = DeviceWithPath(devicePath))
			RemoveDevice(device);
	}
}

// AddDevice
bool
RDiskDeviceList::AddDevice(RDiskDevice *device)
{
	bool success = false;
	if (device) {
		fDevices.BinaryInsert(device, CompareIDPredicate<RDiskDevice>(device));
		device->SetDeviceList(this);
		DeviceAdded(device);
	}
	return success;
}

// RemoveDevice
bool
RDiskDeviceList::RemoveDevice(int32 index)
{
	RDiskDevice *device = DeviceAt(index);
	if (device) {
		DeviceRemoved(device);
		device->SetDeviceList(NULL);
		fDevices.RemoveItemAt(index);
		delete device;
	}
	return (device != NULL);
}

// RemoveDevice
bool
RDiskDeviceList::RemoveDevice(RDiskDevice *device)
{
	bool success = false;
	if (device) {
		int32 index = fDevices.FindBinaryInsertionIndex(
			CompareIDPredicate<RDiskDevice>(device), &success);
		if (success)
			success = RemoveDevice(index);
	}
	return success;
}

// DeviceWithID
RDiskDevice *
RDiskDeviceList::DeviceWithID(int32 id, bool exact) const
{
	bool inList = false;
	int32 index = fDevices.FindBinaryInsertionIndex(
		CompareIDPredicate<RDiskDevice>(id), &inList);
	return (inList || !exact ? DeviceAt(index) : NULL);
}

// SessionWithID
RSession *
RDiskDeviceList::SessionWithID(int32 id) const
{
	bool inList = false;
	int32 index = fSessions.FindBinaryInsertionIndex(
		CompareIDPredicate<RSession>(id), &inList);
	return (inList ? SessionAt(index) : NULL);
}

// PartitionWithID
RPartition *
RDiskDeviceList::PartitionWithID(int32 id) const
{
	bool inList = false;
	int32 index = fPartitions.FindBinaryInsertionIndex(
		CompareIDPredicate<RPartition>(id), &inList);
	return (inList ? PartitionAt(index) : NULL);
}

// DeviceWithPath
RDiskDevice *
RDiskDeviceList::DeviceWithPath(const char *path) const
{
	const RDiskDevice *device
		= fDevices.FindIf(CompareDevicePathPredicate(path));
	return const_cast<RDiskDevice*>(device);
}

// PartitionWithPath
RPartition *
RDiskDeviceList::PartitionWithPath(const char *path) const
{
	const RPartition *partition
		= fPartitions.FindIf(ComparePartitionPathPredicate(path));
	return const_cast<RPartition*>(partition);
}

// Rescan
status_t
RDiskDeviceList::Rescan()
{
FUNCTION_START();
// TODO: This method should be reworked, as soon as we react on events only
// and don't need to poll anymore. This method will then do an initial
// scan only.
	status_t error = B_OK;
	if (Lock()) {
		// marked all devices untouched
		for (int32 i = 0; RDiskDevice *device = DeviceAt(i); i++)
			device->SetTouched(false);
		// scan the "/dev/disk" directory for devices
		BDirectory dir;
		if (dir.SetTo("/dev/disk") == B_OK) {
			error = _Scan(dir);
		}
		// remove all untouched devices
		for (int32 i = CountDevices() - 1; i >= 0; i--) {
			RDiskDevice *device = DeviceAt(i);
			if (!device->Touched())
				DeviceDisappeared(device->Path());
		}
		Unlock();
	} else
		error = B_ERROR;
FUNCTION_END();
	return error;
}

// Lock
bool
RDiskDeviceList::Lock()
{
	return fLock.Lock();
}

// Unlock
void
RDiskDeviceList::Unlock()
{
	fLock.Unlock();
}

// DeviceAdded
void
RDiskDeviceList::DeviceAdded(RDiskDevice *device, uint32 cause)
{
	// propagate to sessions
	for (int32 i = 0; RSession *session = device->SessionAt(i); i++)
		SessionAdded(session, B_DEVICE_CAUSE_PARENT_CHANGED);
	// notifications
	// ...
}

// DeviceRemoved
void
RDiskDeviceList::DeviceRemoved(RDiskDevice *device, uint32 cause)
{
	// propagate to sessions
	for (int32 i = 0; RSession *session = device->SessionAt(i); i++)
		SessionRemoved(session, B_DEVICE_CAUSE_PARENT_CHANGED);
	// notifications
	// ...
}

// SessionAdded
void
RDiskDeviceList::SessionAdded(RSession *session, uint32 cause)
{
	// add the session to our list
	fSessions.BinaryInsert(session, CompareIDPredicate<RSession>(session));
	// propagate to partitions
	for (int32 i = 0; RPartition *partition = session->PartitionAt(i); i++)
		PartitionAdded(partition, B_DEVICE_CAUSE_PARENT_CHANGED);
	// notifications
	// ...
}

// SessionRemoved
void
RDiskDeviceList::SessionRemoved(RSession *session, uint32 cause)
{
	// remove the session from our list
	bool success = false;
	int32 index = fSessions.FindBinaryInsertionIndex(
		CompareIDPredicate<RSession>(session), &success);
	if (success)
		fSessions.RemoveItemAt(index);
	// propagate to partitions
	for (int32 i = 0; RPartition *partition = session->PartitionAt(i); i++)
		PartitionRemoved(partition, B_DEVICE_CAUSE_PARENT_CHANGED);
	// notifications
	// ...
}

// PartitionAdded
void
RDiskDeviceList::PartitionAdded(RPartition *partition, uint32 cause)
{
	// add the partition to our list
	fPartitions.BinaryInsert(partition,
							 CompareIDPredicate<RPartition>(partition));
	// get the corresponding volume, if partition is mounted
	char path[B_FILE_NAME_LENGTH];
	partition->GetPath(path);
	if (const RVolume *volume = fVolumeList.VolumeForDevicePath(path))
		partition->SetVolume(volume);
	// notifications
	// ...
}

// PartitionRemoved
void
RDiskDeviceList::PartitionRemoved(RPartition *partition, uint32 cause)
{
	// remove the partition from our list
	bool success = false;
	int32 index = fPartitions.FindBinaryInsertionIndex(
		CompareIDPredicate<RPartition>(partition), &success);
	if (success)
		fPartitions.RemoveItemAt(index);
	// notifications
	// ...
}

// Dump
void
RDiskDeviceList::Dump() const
{
	for (int32 i = 0; RDiskDevice *device = DeviceAt(i); i++)
		device->Dump();
}

// _Scan
status_t
RDiskDeviceList::_Scan(BDirectory &dir)
{
	status_t error = B_OK;
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		struct stat st;
		if (entry.GetStat(&st) == B_OK) {
			if (S_ISDIR(st.st_mode)) {
				BDirectory subdir;
				if (subdir.SetTo(&entry) == B_OK)
					error = _Scan(subdir);
			} else if (!S_ISLNK(st.st_mode)) {
				BPath path;
				if (entry.GetPath(&path) == B_OK) {
					_ScanDevice(path.Path());
				}
			}
		}
	}
	return error;
}

// _ScanDevice
status_t
RDiskDeviceList::_ScanDevice(const char *path)
{
	status_t error = B_OK;
	// search the list for a device with that path
	if (RDiskDevice *device = DeviceWithPath(path)) {
		// found: just update it
		device->SetTouched(true);
		error = device->Update();
	} else {
		// not found: check whether it is really a disk device and add it to
		// the list
		// The event hook does the work anyway:
		DeviceAppeared(path);
	}
	return error;
}

// _InitNotificationMessage
status_t
RDiskDeviceList::_InitNotificationMessage(BMessage *message, uint32 event,
										  uint32 cause)
{
	status_t error = (message ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		message->what = B_DEVICE_UPDATE;
		error = message->AddInt32("event", event);
	}
	if (error == B_OK)
		error = message->AddInt32("cause", cause);
	return error;
}

// _InitNotificationMessage
status_t
RDiskDeviceList::_InitNotificationMessage(BMessage *message, uint32 event,
										  uint32 cause,
										  const RDiskDevice *device)
{
	status_t error = (device ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = _InitNotificationMessage(message, event, cause);
	if (error == B_OK)
		error = message->AddInt32("device_id", device->ID());
	return error;
}

// _InitNotificationMessage
status_t
RDiskDeviceList::_InitNotificationMessage(BMessage *message, uint32 event,
										  uint32 cause,
										  const RSession *session)
{
	status_t error = (session ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = _InitNotificationMessage(message, event, cause,
										 session->Device());
	}
	if (error == B_OK)
		error = message->AddInt32("session_id", session->ID());
	return error;
}

// _InitNotificationMessage
status_t
RDiskDeviceList::_InitNotificationMessage(BMessage *message, uint32 event,
										  uint32 cause,
										  const RPartition *partition)
{
	status_t error = (partition ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = _InitNotificationMessage(message, event, cause,
										 partition->Session());
	}
	if (error == B_OK)
		error = message->AddInt32("partition_id", partition->ID());
	return error;
}

