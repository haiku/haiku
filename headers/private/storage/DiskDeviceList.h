//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_LIST_H
#define _DISK_DEVICE_LIST_H

#include <DiskDeviceVisitor.h>
#include <Handler.h>
#include <ObjectList.h>

class BDiskDevice;
class BPartition;
class BSession;

class BDiskDeviceList : public BHandler {
public:
	BDiskDeviceList(bool useOwnLocker);
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
	BSession *VisitEachSession(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachPartition(BDiskDeviceVisitor *visitor);
	bool Traverse(BDiskDeviceVisitor *visitor);

	BPartition *VisitEachMountedPartition(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachMountablePartition(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachInitializablePartition(BDiskDeviceVisitor *visitor);

	BDiskDevice *DeviceWithID(int32 id) const;
	BSession *SessionWithID(int32 id) const;
	BPartition *PartitionWithID(int32 id) const;

	virtual void MountPointMoved(BPartition *partition);
	virtual void PartitionMounted(BPartition *partition);
	virtual void PartitionUnmounted(BPartition *partition);
	virtual void PartitionChanged(BPartition *partition);
	virtual void PartitionAdded(BPartition *partition);
	virtual void PartitionRemoved(BPartition *partition);
	virtual void SessionAdded(BSession *session);
	virtual void SessionRemoved(BSession *session);
	virtual void MediaChanged(BDiskDevice *device);
	virtual void DeviceAdded(BDiskDevice *device);
	virtual void DeviceRemoved(BDiskDevice *device);

private:
	void _MountPointMoved(BMessage *message);
	void _PartitionMounted(BMessage *message);
	void _PartitionUnmounted(BMessage *message);
	void _PartitionChanged(BMessage *message);
	void _PartitionAdded(BMessage *message);
	void _PartitionRemoved(BMessage *message);
	void _SessionAdded(BMessage *message);
	void _SessionRemoved(BMessage *message);
	void _MediaChanged(BMessage *message);
	void _DeviceAdded(BMessage *message);
	void _DeviceRemoved(BMessage *message);

	BDiskDevice *_FindDevice(BMessage *message);
	BSession *_FindSession(BMessage *message);
	BPartition *_FindPartition(BMessage *message);

	BDiskDevice *_UpdateDevice(BMessage *message);

private:
	BLocker						*fLocker;
	BObjectList<BDiskDevice>	fDevices;
	bool						fSubscribed;
};

#endif	// _DISK_DEVICE_LIST_H
