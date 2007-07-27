/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <KPartition.h>
#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>

#include <KPartitionVisitor.h>

#include <string.h>

#include "KSetTypeJob.h"
#include "ddm_operation_validation.h"


/**
	Creates the job.

	\param partitionID the partition whose type should be set
	\param type the new type for the partition
*/
KSetTypeJob::KSetTypeJob(partition_id parentID, partition_id partitionID,
		const char *type)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_SET_TYPE, partitionID, parentID),
	  fType(!type ? NULL : strdup(type))
{
	SetDescription("setting partition type");
}


KSetTypeJob::~KSetTypeJob()
{
}


status_t
KSetTypeJob::Do()
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	KPartition* partition = manager->WriteLockPartition(PartitionID());

	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		// TODO is lock necessary?
		DeviceWriteLocker locker(partition->Device(), true);

		// basic checks
		if (!partition->ParentDiskSystem()) {
			SetErrorMessage("Partition has no parent disk system!");
			return B_BAD_VALUE;
		}

		if (!fType) {
			SetErrorMessage("No type to set!");
			return B_BAD_VALUE;
		}

// TODO: The parameters are already validated and should not be changed again!
		status_t validationResult = validate_set_partition_type(
			partition, partition->ChangeCounter(), fType, false);
		if (validationResult != B_OK) {
			SetErrorMessage("Validation of setting partition type failed!");
			return validationResult;
		}

		// everything OK, let's do the job
		KDiskSystem* parentDiskSystem = partition->ParentDiskSystem();
		DiskSystemLoader loader(parentDiskSystem);

		status_t setTypeResult = parentDiskSystem->SetType(partition, fType,
			this);

		if (setTypeResult != B_OK) {
			SetErrorMessage("Setting partition type failed!");
			return setTypeResult;
		}

		return B_OK;

	} else {
		SetErrorMessage("Couldn't find partition!");
		return B_ENTRY_NOT_FOUND;
	}
}
