/*
 * Copyright 2003-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_ROSTER_H
#define _DISK_DEVICE_ROSTER_H

#include <image.h>

#include <DiskDeviceDefs.h>
#include <Messenger.h>
#include <SupportDefs.h>

class BDirectory;
class BDiskDevice;
class BDiskDeviceVisiting;
class BDiskDeviceVisitor;
class BDiskScannerPartitionAddOn;
class BDiskSystem;
class BPartition;
class BVolume;

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

			status_t		GetNextDevice(BDiskDevice* device);
			status_t		RewindDevices();

			status_t		GetNextDiskSystem(BDiskSystem* system);
			status_t		RewindDiskSystems();

			status_t		GetDiskSystem(BDiskSystem* system, const char* name);

			partition_id	RegisterFileDevice(const char* filename);
				// publishes: /dev/disk/virtual/files/<disk device ID>/raw
			status_t		UnregisterFileDevice(const char* filename);
			status_t		UnregisterFileDevice(partition_id device);

			bool			VisitEachDevice(BDiskDeviceVisitor* visitor,
								BDiskDevice* device = NULL);
			bool			VisitEachPartition(BDiskDeviceVisitor* visitor,
								BDiskDevice* device = NULL,
								BPartition** _partition = NULL);

			bool			VisitEachMountedPartition(
								BDiskDeviceVisitor* visitor,
								BDiskDevice* device = NULL,
								BPartition** _partition = NULL);
			bool			VisitEachMountablePartition(
								BDiskDeviceVisitor* visitor,
								BDiskDevice* device = NULL,
								BPartition** _partition = NULL);

			status_t		FindPartitionByVolume(const BVolume& volume,
								BDiskDevice* device,
								BPartition** _partition);
			status_t		FindPartitionByMountPoint(const char* mountPoint,
								BDiskDevice* device,
								BPartition** _partition);

			status_t		GetDeviceWithID(partition_id id,
								BDiskDevice* device) const;
			status_t		GetPartitionWithID(partition_id id,
								BDiskDevice* device,
								BPartition** _partition) const;

			status_t		GetDeviceForPath(const char* filename,
								BDiskDevice* device);
			status_t		GetPartitionForPath(const char* filename,
								BDiskDevice* device, BPartition** _partition);
			status_t		GetFileDeviceForPath(const char* filename,
								BDiskDevice* device);

			status_t		StartWatching(BMessenger target,
								uint32 eventMask = B_DEVICE_REQUEST_ALL);
			status_t		StopWatching(BMessenger target);

private:
#if 0
	status_t _GetObjectWithID(const char *fieldName, partition_id id,
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
#endif	// 0
private:
	int32					fDeviceCookie;
	int32					fDiskSystemCookie;
	int32					fJobCookie;
//	BDirectory	*fPartitionAddOnDir;
//	BDirectory	*fFSAddOnDir;
//	int32		fPartitionAddOnDirIndex;
//	int32		fFSAddOnDirIndex;
};

#endif	// _DISK_DEVICE_ROSTER_H
