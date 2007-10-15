/*
 * Copyright 2003-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_LIST_H
#define _DISK_DEVICE_LIST_H


#include <DiskDeviceDefs.h>
#include <DiskDeviceVisitor.h>
#include <Handler.h>
#include <ObjectList.h>

class BDiskDevice;
class BDiskDeviceRoster;
class BLocker;
class BPartition;
class BSession;


class BDiskDeviceList : public BHandler {
public:
	BDiskDeviceList(bool useOwnLocker = true);
	virtual ~BDiskDeviceList();

	virtual void MessageReceived(BMessage *message);
	virtual void SetNextHandler(BHandler *handler);

	status_t Fetch();
	void Unset();

	bool Lock();
	void Unlock();

	int32 CountDevices() const;
	BDiskDevice *DeviceAt(int32 index) const;

	BDiskDevice *VisitEachDevice(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachPartition(BDiskDeviceVisitor *visitor);

	BPartition *VisitEachMountedPartition(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachMountablePartition(BDiskDeviceVisitor *visitor);

	BDiskDevice *DeviceWithID(partition_id id) const;
	BPartition *PartitionWithID(partition_id id) const;

	virtual void MountPointMoved(BPartition *partition);
	virtual void PartitionMounted(BPartition *partition);
	virtual void PartitionUnmounted(BPartition *partition);
	virtual void PartitionInitialized(BPartition *partition);
	virtual void PartitionResized(BPartition *partition);
	virtual void PartitionMoved(BPartition *partition);
	virtual void PartitionCreated(BPartition *partition);
	virtual void PartitionDeleted(BPartition *partition,
		partition_id partitionID);
	virtual void PartitionDefragmented(BPartition *partition);
	virtual void PartitionRepaired(BPartition *partition);
	virtual void PartitionChanged(BPartition *partition, uint32 event);
	virtual void MediaChanged(BDiskDevice *device);
	virtual void DeviceAdded(BDiskDevice *device);
	virtual void DeviceRemoved(BDiskDevice *device);

private:
	status_t _StartWatching();
	void _StopWatching();

	void _MountPointMoved(BMessage *message);
	void _PartitionMounted(BMessage *message);
	void _PartitionUnmounted(BMessage *message);
	void _PartitionInitialized(BMessage *message);
	void _PartitionResized(BMessage *message);
	void _PartitionMoved(BMessage *message);
	void _PartitionCreated(BMessage *message);
	void _PartitionDeleted(BMessage *message);
	void _PartitionDefragmented(BMessage *message);
	void _PartitionRepaired(BMessage *message);
	void _MediaChanged(BMessage *message);
	void _DeviceAdded(BMessage *message);
	void _DeviceRemoved(BMessage *message);

	BDiskDevice *_FindDevice(BMessage *message);
	BPartition *_FindPartition(BMessage *message);

	BDiskDevice *_UpdateDevice(BMessage *message);

private:
	BLocker						*fLocker;
	BObjectList<BDiskDevice>	fDevices;
	bool						fSubscribed;
};

#endif	// _DISK_DEVICE_LIST_H
