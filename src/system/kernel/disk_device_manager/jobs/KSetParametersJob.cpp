/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <KernelExport.h>
#include <DiskDeviceDefs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionVisitor.h>

#include <string.h>


#include "KSetParametersJob.h"
#include "ddm_operation_validation.h"


/**
	Creates the job.

	\param partition the partition whose params (or content params) should be set
	\param parameters the new parameters for the partition
	\param contentParameters the new parameters for partition's content
*/
KSetParametersJob::KSetParametersJob(partition_id parent,
		partition_id partition, const char *parameters,
		const char *contentParameters)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_SET_TYPE, partition, parent), 
	  fParams(!parameters ? NULL : strdup(parameters)),
	  fContentParams(!contentParameters ? NULL : strdup(contentParameters))
{
	SetDescription("setting partition parameters&content parameters");
}


KSetParametersJob::~KSetParametersJob()
{
	free(fParams);
	free(fContentParams);
}


status_t
KSetParametersJob::Do()
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		if (!fParams && !fContentParams) {	
			SetErrorMessage("No parameter to set.");
			return B_BAD_VALUE;
		}
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		// TODO is lock necessary?
		DeviceWriteLocker locker(partition->Device(), true);

		// all descendants should be marked busy/descendant busy
		if (IsPartitionNotBusy(partition)) {
			SetErrorMessage("Can't set parameters for non-busy partition!");
			return B_ERROR;
		}

		// TODO unlock?

		if (fParams) {
			// basic checks

			if (!partition->ParentDiskSystem()) {
				SetErrorMessage("Partition has no parent disk system!");
				return B_BAD_VALUE;
			}
					
			// TODO maybe give copy of parameters?
			// we have some parameters to set
// TODO: The parameters are already validated and should not be changed again!
			status_t validationResult = validate_set_partition_parameters(
				partition, partition->ChangeCounter(), fParams, false);

			KDiskSystem *parentDiskSystem = partition->ParentDiskSystem();
			DiskSystemLoader loader(parentDiskSystem);

			if (validationResult != B_OK) {
				SetErrorMessage("Validation of setting partition parameters "
					"failed.");
				return validationResult;
			}

			locker.Unlock();
			status_t setParametersResult = parentDiskSystem->SetParameters(
				partition, fParams, this);

			if (setParametersResult != B_OK) {
				SetErrorMessage( "Setting partition parameters failed." );
			}			
		}

		if (fContentParams) {
			// basic checks

			if (!partition->DiskSystem()) {
				SetErrorMessage("Partition has no disk system!");
				return B_BAD_VALUE;
			}

// TODO: The parameters are already validated and should not be changed again!
			status_t validationResult
				= validate_set_partition_content_parameters(partition,
					partition->ChangeCounter(), fContentParams, false);

			if (validationResult != B_OK) {
				SetErrorMessage("Validation of setting partition content "
					"parameters failed.");
				return validationResult;
			}

			KDiskSystem *diskSystem = partition->DiskSystem();
			DiskSystemLoader loader(diskSystem);

			locker.Unlock();
			status_t result = diskSystem->SetContentParameters(partition,
				fContentParams, this);

			if (result != B_OK) {
				SetErrorMessage("Setting partition content parameters failed.");
			}
		}
// TODO: Check return values!

		return B_OK;

	} else {
		SetErrorMessage( "Couldn't find partition" );
		return B_ENTRY_NOT_FOUND;
	}
}
