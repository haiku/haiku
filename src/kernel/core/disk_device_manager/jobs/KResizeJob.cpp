// KResizeJob.cpp

#include <stdio.h>

#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>

#include "ddm_operation_validation.h"
#include "KResizeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KResizeJob::KResizeJob(partition_id parentID, partition_id partitionID,
					   off_t size)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_RESIZE, partitionID, parentID),
	  fSize(size)
{
	SetDescription("resizing partition");
}

// destructor
KResizeJob::~KResizeJob()
{
}

// Do
status_t
KResizeJob::Do()
{
	DBG(OUT("KResizeJob::Do(%ld)\n", PartitionID()));
	// get the partition
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// basic checks
		if (!partition->ParentDiskSystem()) {
			SetErrorMessage("Partition has no parent disk system!");
			return B_BAD_VALUE;
		}
		// if size remains the same, then nothing's to do
		if (partition->Size() == fSize)
			return B_OK;
		// check new size
		off_t size = fSize;
		off_t contentSize = fSize;
		status_t error = validate_resize_partition(partition,
			partition->ChangeCounter(), &size, &contentSize, false);
		if (error != B_OK) {
			SetErrorMessage("Validation of the new partition size failed.");
			return error;
		}
		if (size != fSize) {
			SetErrorMessage("Requested size is not valid.");
			return B_ERROR;
		}
		// all descendants should be marked busy/descendant busy
		struct IsNotBusyVisitor : KPartitionVisitor {
			virtual bool VisitPre(KPartition *partition)
			{
				return !(partition->IsBusy() || partition->IsDescendantBusy());
			}
		} isNotBusyVisitor;
		if (partition->VisitEachDescendant(&isNotBusyVisitor)) {
			SetErrorMessage("Can't resize non-busy partition!");
			return B_ERROR;
		}
		// things look good: get all infos needed for resizing
		KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
		KDiskSystem *childDiskSystem = partition->DiskSystem();
		DiskSystemLoader loader1(parentDiskSystem);
		DiskSystemLoader loader2(childDiskSystem);
		off_t oldSize = partition->Size();
		off_t oldContentSize = partition->ContentSize();
		// unlock the device and resize the beast
		locker.Unset();
		// if growing, resize partition first
		if (oldSize < fSize) {
			status_t error = parentDiskSystem->ResizeChild(partition, fSize,
														   this);
			if (error != B_OK)
				return error;
		}
		// resize contents
		if (childDiskSystem && oldContentSize != contentSize) {
			status_t error = childDiskSystem->Resize(partition, contentSize,
													 this);
			if (error != B_OK)
				return error;
		}
		// if shrinking, resize partition last
		if (oldSize > fSize) {
			status_t error = parentDiskSystem->ResizeChild(partition, fSize,
														   this);
			if (error != B_OK)
				return error;
		}
	} else {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}
	// cannot come here
	return B_ERROR;
}

