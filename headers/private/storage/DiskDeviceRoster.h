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
	// Basic masks
	B_DEVICE_REQUEST_MOUNT_POINT			= 0x0001,	// mount point changes
	B_DEVICE_REQUEST_MOUNTING				= 0x0002,	// mounting/unmounting
	B_DEVICE_REQUEST_PARTITION				= 0x0004,	// partition changes 
	B_DEVICE_REQUEST_DEVICE					= 0x0008,	// device changes (media changes)
	B_DEVICE_REQUEST_DEVICE_LIST			= 0x0010,	// device additions/removals
	B_DEVICE_REQUEST_JOB_LIST				= 0x0020, 	// job addition/initiation/cancellation/completion
	B_DEVICE_REQUEST_JOB_SIMPLE_PROGRESS	= 0x0040, 	// simple job progress (i.e. "% complete" only)
	B_DEVICE_REQUEST_JOB_EXTRA_PROGRESS		= 0x0080, 	// extra info on job progress (no "% complete" info)

	// Combination masks
	B_DEVICE_REQUEST_JOB_COMPLETE_PROGRESS	= 0x00C0, 	// complete job progress info
	B_DEVICE_REQUEST_ALL					= 0xffff,	// all events
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
	B_DEVICE_JOB_SCHEDULED,				// job added
	B_DEVICE_JOB_INITIATED,				// job initiated
	B_DEVICE_JOB_CANCELED,				// job canceled
	B_DEVICE_JOB_FINISHED,				// job finished
	B_DEVICE_JOB_SIMPLE_PROGRESS,		// job percent complete progress
	B_DEVICE_JOB_EXTRA_PROGRESS,		// extended job progress info
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

	// Active jobs are those that are scheduled or in-progress
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
									 
	status_t GetDeviceWithID(uint32 id, BDiskDevice *device) const;
	status_t GetPartitionWithID(uint32 id, BDiskDevice *device,
								BPartition **partition) const;

	status_t StartWatching(BMessenger target,
						   uint32 eventMask = B_DEVICE_REQUEST_ALL);
	status_t StartWatchingJob(BDiskDeviceJob *job, BMessenger target,
	                          uint32 eventMask = B_DEVICE_REQUEST_JOB_COMPLETE_PROGRESS);
	status_t StopWatching(BMessenger target);

private:
	status_t _GetObjectWithID(const char *fieldName, uint32 id,
							  BDiskDevice *device) const;

	// TODO: Introduce iterators instead of these functions.

	static status_t _GetNextAddOn(BDirectory **directory, uint32 *index,
								  const char *subdir,
								  BPrivate::AddOnImage *image);
	static status_t _GetNextAddOn(BDirectory *directory,
								  BPrivate::AddOnImage *image);
	static status_t _GetNextAddOnDir(BPath *path, uint32 *index,
									 const char *subdir);
	static status_t _GetNextAddOnDir(BDirectory **directory, uint32 *index,
									 const char *subdir);

	static status_t _LoadPartitionAddOn(const char *partitioningSystem,
										BPrivate::AddOnImage *image,
										BDiskScannerPartitionAddOn **addOn);

private:
	BMessenger	fManager;
	uint32		fCookie;
	BDirectory	*fPartitionAddOnDir;
	BDirectory	*fFSAddOnDir;
	uint32		fPartitionAddOnDirIndex;
	uint32		fFSAddOnDirIndex;
};

#endif	// _DISK_DEVICE_ROSTER_H
