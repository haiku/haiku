//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_ROSTER_H
#define _DISK_DEVICE_ROSTER_H

#include <image.h>

#include <DiskDeviceVisitor.h>
#include <Messenger.h>
#include <SupportDefs.h>

class BDirectory;
class BDiskDevice;
class BDiskDeviceJob;
class BDiskScannerPartitionAddOn;
class BDiskSystem;
class BPartition;
class BSession;

namespace BPrivate {
	class AddOnImage;
}

// watchable events
enum {
	B_DEVICE_REQUEST_MOUNT_POINT	= 0x01,	// mount point changes
	B_DEVICE_REQUEST_MOUNTING		= 0x02,	// mounting/unmounting
	B_DEVICE_REQUEST_PARTITION		= 0x04,	// partition changes 
	B_DEVICE_REQUEST_DEVICE			= 0x10,	// device changes (media changes)
	B_DEVICE_REQUEST_DEVICE_LIST	= 0x20,	// device additions/removals
	B_DEVICE_REQUEST_JOBS			= 0x40, // job addition/initiation/completion
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
	B_DEVICE_PARTITION_INITIALIZED,		// partition initialized
	B_DEVICE_PARTITION_RESIZED,			// partition resized
	B_DEVICE_PARTITION_MOVED,			// partition moved
	B_DEVICE_PARTITION_CREATED,			// partition created
	B_DEVICE_PARTITION_DELETED,			// partition deleted
	B_DEVICE_PARTITION_DEFRAGMENTED,	// partition defragmented
	B_DEVICE_PARTITION_REPAIRED,		// partition repaired
	B_DEVICE_MEDIA_CHANGED,				// media changed
	B_DEVICE_ADDED,						// device added
	B_DEVICE_REMOVED,					// device removed
	B_DEVICE_JOB_ADDED,					// job added
	B_DEVICE_JOB_INITIATED,				// job initiated
	B_DEVICE_JOB_FINISHED,				// job finished
};

// notification message "cause" field values
enum {
	// helpful causes
	B_DEVICE_CAUSE_MEDIA_CHANGED,
	B_DEVICE_CAUSE_FORMATTED,		// is this still applicable?
	B_DEVICE_CAUSE_PARTITIONED,		// is this still applicable?
	B_DEVICE_CAUSE_INITIALIZED,		// is this still applicable?
	// unknown cause
	B_DEVICE_CAUSE_UNKNOWN,
	// for internal use only (e.g.: partition added, because device added)
	B_DEVICE_CAUSE_PARENT_CHANGED,
};

class BDiskDeviceRoster {
public:
	BDiskDeviceRoster();
	~BDiskDeviceRoster();
	
	status_t GetNextDevice(BDiskDevice *device);
	status_t RewindDevices();
	
	status_t GetNextDiskSystem(BDiskSystem *system);
	status_t RewindDiskSystems();

	status_t GetNextActiveJob(BDiskDeviceJob *job);
	status_t RewindActiveJobs();

	bool VisitEachDevice(BDiskDeviceVisitor *visitor,
						 BDiskDevice *device = NULL);
	bool VisitEachPartition(BDiskDeviceVisitor *visitor,
							BDiskDevice *device = NULL,
							BPartition **partition = NULL);
	bool VisitAll(BDiskDeviceVisitor *visitor);

	bool VisitEachMountedPartition(BDiskDeviceVisitor *visitor,
								   BDiskDevice *device = NULL,
								   BPartition **partition = NULL);
	bool VisitEachMountablePartition(BDiskDeviceVisitor *visitor,
									 BDiskDevice *device = NULL,
									 BPartition **partition = NULL);
	bool VisitEachInitializablePartition(BDiskDeviceVisitor *visitor,
										 BDiskDevice *device = NULL,
										 BPartition **partition = NULL);
	bool VisitEachPartitionablePartition(BDiskDeviceVisitor *visitor,
									 BDiskDevice *device = NULL,
									 BPartition **partition = NULL);
									 
	// Do we want visit functions for disk systems and/or jobs?
	
	status_t GetDeviceWithID(int32 id, BDiskDevice *device) const;
	status_t GetPartitionWithID(int32 id, BDiskDevice *device,
								BPartition **partition) const;

	status_t StartWatching(BMessenger target,
						   uint32 eventMask = B_DEVICE_REQUEST_ALL);
	status_t StopWatching(BMessenger target);

private:
	status_t _GetObjectWithID(const char *fieldName, int32 id,
							  BDiskDevice *device) const;

	// TODO: Introduce iterators instead of these functions.

	static status_t _GetNextAddOn(BDirectory **directory, int32 *index,
								  const char *subdir,
								  BPrivate::AddOnImage *image);
	static status_t _GetNextAddOn(BDirectory *directory,
								  BPrivate::AddOnImage *image);
	static status_t _GetNextAddOnDir(BPath *path, int32 *index,
									 const char *subdir);
	static status_t _GetNextAddOnDir(BDirectory **directory, int32 *index,
									 const char *subdir);

	static status_t _LoadPartitionAddOn(const char *partitioningSystem,
										BPrivate::AddOnImage *image,
										BDiskScannerPartitionAddOn **addOn);

private:
	BMessenger	fManager;
	int32		fCookie;
	BDirectory	*fPartitionAddOnDir;
	BDirectory	*fFSAddOnDir;
	int32		fPartitionAddOnDirIndex;
	int32		fFSAddOnDirIndex;
};

#endif	// _DISK_DEVICE_ROSTER_H
