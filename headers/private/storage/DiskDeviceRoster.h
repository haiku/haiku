// DiskDeviceRoster.h

#ifndef _DISK_DEVICE_ROSTER_H
#define _DISK_DEVICE_ROSTER_H

#include <DiskDeviceVisitor.h>
#include <SupportDefs.h>

class BDiskDevice;
class BPartition;
class BSession;

// watchable events
enum {
	B_DEVICE_REQUEST_MOUNT_POINT	= 0x01,	// mount point changes
	B_DEVICE_REQUEST_MOUNTING		= 0x02,	// mounting/unmounting
	B_DEVICE_REQUEST_PARTITION		= 0x04,	// partition changes (initial.)
	B_DEVICE_REQUEST_SESSION		= 0x08,	// session changes (partitioning)
	B_DEVICE_REQUEST_DEVICE			= 0x10,	// device changes (media changes)
	B_DEVICE_REQUEST_DEVICE_LIST	= 0x20,	// device additions/removals
	B_DEVICE_REQUEST_ALL			= 0xff,	// all events
};

// notification message "what" field
// TODO: move to app/AppDefs.h
enum {
	B_DEVICE_UPDATE	= 'DUPD'
};

// notification message "event" field values
enum {
	B_DEVICE_MOUNT_POINT_MOVED,			// mount point moved/renamed
	B_DEVICE_PARTITION_MOUNTED,			// partition mounted
	B_DEVICE_PARTITION_UNMOUNTED,		// partition unmounted
	B_DEVICE_PARTITION_CHANGED,			// partition changed, e.g. initialized
										// or resized
	B_DEVICE_PARTITION_ADDED,			// partition added
	B_DEVICE_PARTITION_REMOVED,			// partition removed
	B_DEVICE_MEDIA_CHANGED,				// media changed
	B_DEVICE_ADDED,						// device added
	B_DEVICE_REMOVED					// device removed
};

class BDiskDeviceRoster {
public:
	BDiskDeviceRoster();
	~BDiskDeviceRoster();

	status_t GetNextDevice(BDiskDevice *device);
	status_t Rewind();

	bool VisitEachDevice(BDiskDeviceVisitor *visitor,
						 BDiskDevice *device = NULL);
		// return true if terminated early

	bool VisitEachPartition(BDiskDeviceVisitor *visitor,
							BDiskDevice *device = NULL,
							BPartition **partition = NULL);
		// return true if terminated early
	bool Traverse(BDiskDeviceVisitor *visitor);
		// return true if terminated early

	bool VisitEachMountedPartition(BDiskDeviceVisitor *visitor,
								   BDiskDevice *device = NULL,
								   BPartition **partition = NULL);
		// return true if terminated early
	bool VisitEachMountablePartition(BDiskDeviceVisitor *visitor,
									 BDiskDevice *device = NULL,
									 BPartition **partition = NULL);
		// return true if terminated early
	bool VisitEachInitializablePartition(BDiskDeviceVisitor *visitor,
										 BDiskDevice *device = NULL,
										 BPartition **partition = NULL);
		// return true if terminated early

	BDiskDevice *DeviceWithID(int32 id) const;
	BSession *SessionWithID(int32 id, BDiskDevice *device) const;
		// inits device to the device containing the session
	BPartition *PartitionWithID(int32 id, BDiskDevice *device) const;
		// inits device to the device containing the partition

	status_t StartWatching(BMessenger target,
						   uint32 eventMask = B_DEVICE_REQUEST_ALL);
	status_t StopWatching(BMessenger target);

private:
	int32	fDeviceID;	// iteration state
};

#endif	// _DISK_DEVICE_ROSTER_H
