//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_LIST_H
#define DISK_DEVICE_LIST_H

#include <Locker.h>
#include <ObjectList.h>

class BDirectory;
class BEntry;

class RDiskDevice;
class RPartition;
class RSession;

class RDiskDeviceList {
public:
	RDiskDeviceList();
	~RDiskDeviceList();

	bool AddDevice(RDiskDevice *device);
	bool RemoveDevice(int32 index);
	bool RemoveDevice(RDiskDevice *device);
	int32 CountDevices() const { return fDevices.CountItems(); }
	RDiskDevice *DeviceAt(int32 index) const { return fDevices.ItemAt(index); }

	int32 CountSessions() const { return fSessions.CountItems(); }
	RSession *SessionAt(int32 index) const { return fSessions.ItemAt(index); }

	int32 CountPartitions() const { return fPartitions.CountItems(); }
	RPartition *PartitionAt(int32 index) const
		{ return fPartitions.ItemAt(index); }

	RDiskDevice *DeviceWithID(int32 id) const;
	RSession *SessionWithID(int32 id) const;
	RPartition *PartitionWithID(int32 id) const;

	RDiskDevice *DeviceWithPath(const char *path) const;

	status_t Rescan();

	bool Lock();
	void Unlock();

	void DeviceAdded(RDiskDevice *device);
	void DeviceRemoved(RDiskDevice *device);
	void SessionAdded(RSession *session);
	void SessionRemoved(RSession *session);
	void PartitionAdded(RPartition *partition);
	void PartitionRemoved(RPartition *partition);

	void Dump() const;

private:
	status_t _Scan(BDirectory &dir);
	status_t _ScanDevice(const char *path);

private:
	BLocker						fLock;
	BObjectList<RDiskDevice>	fDevices;		// sorted by ID
	BObjectList<RSession>		fSessions;		//
	BObjectList<RPartition>		fPartitions;	//
};

#endif	// DISK_DEVICE_LIST_H
