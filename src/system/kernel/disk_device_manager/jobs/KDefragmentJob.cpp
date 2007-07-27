/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include "KDefragmentJob.h"


#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>
#include <string.h>

#include "ddm_operation_validation.h"


/**
	Creates the job

	\param partition which device should we defragment
*/
KDefragmentJob::KDefragmentJob(partition_id partition)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_DEFRAGMENT, partition, partition)
{
	SetDescription("defragmenting partition");
}


KDefragmentJob::~KDefragmentJob()
{
}


status_t
KDefragmentJob::Do()
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	KPartition* partition = manager->WriteLockPartition(PartitionID());

	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		DeviceWriteLocker locker(partition->Device(), true);

		// some basic checks

		if (!partition->DiskSystem()) {
			SetErrorMessage("Partition has no disk system!");
			return B_BAD_VALUE;
		}

		// all descendants should be marked busy/descendant busy
		if (IsPartitionNotBusy(partition) ) {
			SetErrorMessage("Can't defragment non-busy partition!");
			return B_ERROR;
		}

		bool whileMounted;
		// OK, seems alright, let's validate the job
		status_t validationResult = validate_defragment_partition(
			partition, partition->ChangeCounter(), &whileMounted, false);

		if (validationResult != B_OK) {
			SetErrorMessage("Validation of defragmenting partition failed.");
			return validationResult;
		}

		if (!whileMounted && partition->IsMounted()) {
			SetErrorMessage("This partition cannot be defragmented while "
				"mounted." );
			return B_ERROR;
		}

		// everything OK, let's do the job!

		KDiskSystem *diskSystem = partition->DiskSystem();
		DiskSystemLoader loader(diskSystem);

		locker.Unlock();

		status_t defragResult = diskSystem->Defragment(partition, this);

		if (defragResult != B_OK) {
			SetErrorMessage("Defragmenting partition failed!");
			return defragResult;
		}

		return B_OK;

	} else {
		SetErrorMessage("Couldn't find partition!");
		return B_ENTRY_NOT_FOUND;
	}
}
