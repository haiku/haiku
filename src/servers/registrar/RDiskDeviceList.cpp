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
#include <Drivers.h>
#include <Entry.h>
#include <Locker.h>
#include <Path.h>

#include "RDiskDeviceList.h"
#include "Debug.h"
#include "RDiskDevice.h"
#include "RPartition.h"
#include "RSession.h"

// compare_ids
/*template<typename T>
static
int
compare_ids(const T *object1, const T *object2)
{
	int32 id1 = object1->ID();
	int32 id2 = object2->ID();
	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return 1;
	return 0;
}*/

// CompareIDPredicate
template <typename T>
struct CompareIDPredicate : public UnaryPredicate<T> {
	CompareIDPredicate(int32 id) : fID(id) {}
	CompareIDPredicate(const T *object) : fID(object->ID()) {}

	virtual int operator()(const T *object) const
	{
		int32 id = object->ID();
		if (fID < id)
			return -1;
		if (fID > id)
			return 1;
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

// constructor
RDiskDeviceList::RDiskDeviceList(BMessenger target, BLocker &lock,
								 RVolumeList &volumeList)
	: MessageHandler(),
	  RVolumeListListener(),
	  fLock(lock),
	  fTarget(target),
	  fVolumeList(volumeList),
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
}

// VolumeUnmounted
void
RDiskDeviceList::VolumeUnmounted(const RVolume *volume)
{
	PRINT(("RDiskDeviceList::VolumeUnmounted(%ld)\n", volume->ID()));
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
		device->SetDeviceList(NULL);
		fDevices.RemoveItemAt(index);
		DeviceRemoved(device);
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
RDiskDeviceList::DeviceWithID(int32 id) const
{
	bool inList = false;
	int32 index = fDevices.FindBinaryInsertionIndex(
		CompareIDPredicate<RDiskDevice>(id), &inList);
	return (inList ? DeviceAt(index) : NULL);
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

// Rescan
status_t
RDiskDeviceList::Rescan()
{
FUNCTION_START();
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
			if (!DeviceAt(i)->Touched())
				RemoveDevice(i);
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
RDiskDeviceList::DeviceAdded(RDiskDevice *device)
{
}

// DeviceRemoved
void
RDiskDeviceList::DeviceRemoved(RDiskDevice *device)
{
}

// SessionAdded
void
RDiskDeviceList::SessionAdded(RSession *session)
{
}

// SessionRemoved
void
RDiskDeviceList::SessionRemoved(RSession *session)
{
}

// PartitionAdded
void
RDiskDeviceList::PartitionAdded(RPartition *partition)
{
}

// PartitionRemoved
void
RDiskDeviceList::PartitionRemoved(RPartition *partition)
{
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
//BEntry dirEntry;
//BPath dirPath;
//dir.GetEntry(&dirEntry);
//dirEntry.GetPath(&dirPath);
//PRINT(("RDiskDeviceList::_Scan(`%s')\n", dirPath.Path()));
	status_t error = B_OK;
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		struct stat st;
		if (entry.GetStat(&st) == B_OK) {
//PRINT(("st.st_mode: 0x%x\n", st.st_mode & S_IFMT));
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
		error = device->Update();
	} else {
		// not found: check whether it is really a disk device and add it to
		// the list
		int fd = open(path, O_RDONLY);
		if (fd >= 0) {
			bool closeFile = true;
			device_geometry geometry;
			status_t mediaStatus = B_OK;
			partition_info partitionInfo;
			if (ioctl(fd, B_GET_MEDIA_STATUS, &mediaStatus) == 0
				&& ioctl(fd, B_GET_GEOMETRY, &geometry) == 0
				&& ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo) < 0) {
//				int32 blockSize = geometry.bytes_per_sector;
//				off_t deviceSize = (off_t)blockSize
//					* geometry.sectors_per_track * geometry.cylinder_count
//					* geometry.head_count;
				RDiskDevice *device = new(nothrow) RDiskDevice;
				if (device) {
					error = device->SetTo(path, fd, &geometry, mediaStatus);
					if (error == B_OK) {
						closeFile = false;
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
	return error;
}

