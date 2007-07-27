/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include "KDeleteChildJob.h"

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


/*!
	Creates the job

	\param parent device of the deleted partition
	\param partition partition supposed to be removed
*/
KDeleteChildJob::KDeleteChildJob(partition_id parent, partition_id partition)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_DELETE, partition, parent)
{
	SetDescription("deleting child of the partition");
}
		

KDeleteChildJob::~KDeleteChildJob()
{
}


status_t
KDeleteChildJob::Do()
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	KPartition* partition = manager->WriteLockPartition(PartitionID());

	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		DeviceWriteLocker locker(partition->Device(), true);

		if (!partition->ParentDiskSystem()) {
			SetErrorMessage("Partition has no parent disk system!");
			return B_BAD_VALUE;
		}

		// all descendants should be marked busy/descendant busy
		if (IsPartitionNotBusy( partition)) {
			SetErrorMessage("Can't delete child of non-busy partition!");
			return B_ERROR;
		}

		status_t validationResult = validate_delete_child_partition(
			partition, partition->ChangeCounter(), false);

		if (validationResult != B_OK) {
			SetErrorMessage("Validation of deleting child failed!");
			return validationResult;
		}

		// everything OK, let's do the job!
		KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
		DiskSystemLoader loader(parentDiskSystem );

		locker.Unlock();

		status_t deleteResult = parentDiskSystem->DeleteChild(partition, this);

		if (deleteResult != B_OK) {
			SetErrorMessage("Deleting child failed!");
			return deleteResult;
		}

		return B_OK;

	} else {
		SetErrorMessage("Couldn't find partition!");
		return B_ENTRY_NOT_FOUND;
	}
}
