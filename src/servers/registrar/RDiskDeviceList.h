//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_LIST_H
#define DISK_DEVICE_LIST_H

#include <Messenger.h>
#include <ObjectList.h>

#include "MessageHandler.h"
#include "RVolumeList.h"

class BDirectory;
class BEntry;
class BLocker;

class RDiskDevice;
class RPartition;
class RSession;
class WatchingService;

// device list modification causes
// TODO: to be moved to <DiskDeviceRoster.h>
enum {
	// helpful causes
	B_DEVICE_CAUSE_MEDIA_CHANGED,
	B_DEVICE_CAUSE_FORMATTED,
	B_DEVICE_CAUSE_PARTITIONED,
	B_DEVICE_CAUSE_INITIALIZED,
	// unknown cause
	B_DEVICE_CAUSE_UNKNOWN,
	// for internal use only (e.g.: partition added, because device added)
	B_DEVICE_CAUSE_PARENT_CHANGED,
};

class RDiskDeviceList : public MessageHandler, public RVolumeListListener {
public:
	RDiskDeviceList(BMessenger target, BLocker &lock, RVolumeList &volumeList,
					WatchingService *watchingService);
	~RDiskDeviceList();

	virtual void HandleMessage(BMessage *message);

	virtual void VolumeMounted(const RVolume *volume);
	virtual void VolumeUnmounted(const RVolume *volume);
	virtual void MountPointMoved(const RVolume *volume,
								 const entry_ref *oldRoot,
								 const entry_ref *newRoot);

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

	RDiskDevice *DeviceWithID(int32 id, bool exact = true) const;
	RSession *SessionWithID(int32 id) const;
	RPartition *PartitionWithID(int32 id) const;

	RDiskDevice *DeviceWithPath(const char *path) const;
	RPartition *PartitionWithPath(const char *path) const;

	status_t Rescan();

	bool Lock();
	void Unlock();

	void DeviceAdded(RDiskDevice *device,
					 uint32 cause = B_DEVICE_CAUSE_UNKNOWN);
	void DeviceRemoved(RDiskDevice *device,
					   uint32 cause = B_DEVICE_CAUSE_UNKNOWN);
	void SessionAdded(RSession *session,
					  uint32 cause = B_DEVICE_CAUSE_UNKNOWN);
	void SessionRemoved(RSession *session,
						uint32 cause = B_DEVICE_CAUSE_UNKNOWN);
	void PartitionAdded(RPartition *partition,
						uint32 cause = B_DEVICE_CAUSE_UNKNOWN);
	void PartitionRemoved(RPartition *partition,
						  uint32 cause = B_DEVICE_CAUSE_UNKNOWN);

	void Dump() const;

private:
	status_t _Scan(BDirectory &dir);
	status_t _ScanDevice(const char *path);

	status_t _InitNotificationMessage(BMessage *message, uint32 event,
									  uint32 cause);
	status_t _InitNotificationMessage(BMessage *message, uint32 event,
									  uint32 cause, const RDiskDevice *device);
	status_t _InitNotificationMessage(BMessage *message, uint32 event,
									  uint32 cause, const RSession *session);
	status_t _InitNotificationMessage(BMessage *message, uint32 event,
									  uint32 cause,
									  const RPartition *partition);

private:
	mutable BLocker				&fLock;
	BMessenger					fTarget;
	RVolumeList					&fVolumeList;
	WatchingService				*fWatchingService;
	BObjectList<RDiskDevice>	fDevices;		// sorted by ID
	BObjectList<RSession>		fSessions;		//
	BObjectList<RPartition>		fPartitions;	//
};

#endif	// DISK_DEVICE_LIST_H
