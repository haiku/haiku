// KInitializeJob.cpp

#include <stdio.h>

#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>

#include "KUninitializeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KUninitializeJob::KUninitializeJob(partition_id partitionID)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_UNINITIALIZE, partitionID)
{
	SetDescription("uninitializing partition");
}

// destructor
KUninitializeJob::~KUninitializeJob()
{
}

// Do
status_t
KUninitializeJob::Do()
{
DBG(OUT("KUninitializeJob::Do(%ld)\n", PartitionID()));
	// get the partition
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KPartition *partition = manager->WriteLockPartition(PartitionID())) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// the partition's descendants must not be mounted
		struct IsMountedVisitor : KPartitionVisitor {
			virtual bool VisitPre(KPartition *partition)
			{
				return partition->IsMounted();
			}
		} isMountedVisitor;
		if (partition->VisitEachDescendant(&isMountedVisitor)) {
			SetErrorMessage("Can't uninitialize mounted partition.");
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
			SetErrorMessage("Can't uninitialize non-busy partition!");
			return B_ERROR;
		}
		// things look good: uninitialize the thing
		status_t error = partition->UninitializeContents();
		if (error != B_OK)
			SetErrorMessage("Failed to uninitialize partition contents!");
		return error;
	} else {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}
	// cannot come here
	return B_ERROR;
}

