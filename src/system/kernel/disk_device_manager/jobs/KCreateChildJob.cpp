/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include "KCreateChildJob.h"

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
	Creates the job.

	\param partition whose children should we create
	\param child new child ID
	\param offset where the child should start
	\param size size of the new child
	\param parameters additional parameters for the operation
*/
KCreateChildJob::KCreateChildJob(partition_id partition, partition_id child,
		off_t offset, off_t size, const char *type, const char *parameters)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_CREATE, partition, partition),
	  fChildID(child),
	  fOffset(offset),
	  fSize(size),
	  fType (!type ? NULL : strdup(type)),
	  fParameters(!parameters ? NULL : strdup(parameters))
{
	SetDescription( "creating child of the partition" );
}
				

KCreateChildJob::~KCreateChildJob()
{
	free(fType);
	free(fParameters);
}


/**
 * Do the actual creation
 * 
 * \note in time of calling the \c KDiskSystem function for creating child, the partition is NOT locked 
 */
status_t
KCreateChildJob::Do()
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	KPartition* partition = manager->WriteLockPartition(PartitionID());

	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		DeviceWriteLocker locker(partition->Device(), true);

		if (!partition->DiskSystem()) {
			SetErrorMessage("Partition has no disk system!");
			return B_BAD_VALUE;
		}

		// all descendants should be marked busy/descendant busy
		if (IsPartitionNotBusy(partition)) {
			SetErrorMessage("Can't create child of non-busy partition!");
			return B_ERROR;
		}

		KPartition* childPartition = manager->WriteLockPartition(fChildID);
		if (!childPartition) {
			// TODO
		}

		off_t newOffset = fOffset;
		off_t newSize = fSize;
		status_t validationResult = validate_create_child_partition(
			partition, partition->ChangeCounter(),
			&newOffset, &fSize, fType,
			fParameters, NULL, false);

		if (validationResult != B_OK) {
			SetErrorMessage("Validating of creating new child failed!");
			return validationResult;
		}

		if (newOffset != fOffset) {
			SetErrorMessage("Requested offset is not valid.");
			return B_ERROR;
		}

		if (newSize != fSize) {
			SetErrorMessage("Requested size is not valid");
		}

		KDiskSystem *diskSystem = partition->DiskSystem();
		DiskSystemLoader loader(diskSystem);
		KPartition *newChild = 0;

		locker.Unlock(); 

		status_t createResult = diskSystem->CreateChild(partition, newOffset,
			newSize, fType, fParameters, this, &newChild, fChildID);

		if (createResult != B_OK) {
			SetErrorMessage("Creating new partition child failed!");
			return createResult;
		}

		return B_OK;
	}
	else {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}
}
