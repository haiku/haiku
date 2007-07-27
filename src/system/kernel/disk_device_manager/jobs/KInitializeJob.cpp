/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <stdio.h>

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
#include "KInitializeJob.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


/*!
	Creates the job.

	\param partition the partition to initialize
	\param diskSystemID which disk system the partition should be initialized with
	\param parameters additional parameters for the operation
*/
KInitializeJob::KInitializeJob(partition_id partition,
	disk_system_id diskSystemID, const char *name, const char *parameters)
	: KDiskDeviceJob(B_DISK_DEVICE_JOB_INITIALIZE, partition, partition),
	  fDiskSystemID(diskSystemID),
	  fName(!name ? NULL : strdup(name)),
	  fParameters(!parameters ? NULL : strdup(parameters))
{
	SetDescription("initializing the partition with given disk system");
}


KInitializeJob::~KInitializeJob()
{
	free(fName);
	free(fParameters);
}


status_t
KInitializeJob::Do()
{
DBG(OUT("KInitializeJob::Do(%ld)\n", PartitionID()));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->WriteLockPartition(PartitionID());
	if (partition) {
		PartitionRegistrar registrar(partition, true);
		PartitionRegistrar deviceRegistrar(partition->Device(), true);

		DeviceWriteLocker locker(partition->Device(), true);
		// basic checks

// 		if (!partition->ParentDiskSystem()) {
// 			SetErrorMessage("Partition has no parent disk system!");
// 			return B_BAD_VALUE;
// 		}

		// all descendants should be marked busy/descendant busy
		if (IsPartitionNotBusy(partition)) {
			SetErrorMessage("Can't initialize non-busy partition!");
			return B_ERROR;
		}

/*		if (!fParameters) {
			//no parameters for the operation
			SetErrorMessage( "No parameters for partition initialization." );
			return B_ERROR;
		}*/

		// TODO shouldn't we load the disk system AFTER the validation?
		KDiskSystem *diskSystemToInit = manager->LoadDiskSystem(fDiskSystemID);
		if (!diskSystemToInit) {
			SetErrorMessage("Given DiskSystemID doesn't correspond to any "
				"known DiskSystem.");
			return B_BAD_VALUE;
		}
		DiskSystemLoader loader2(diskSystemToInit);

// TODO: The parameters are already validated and should not be changed again!
		status_t validationResult = validate_initialize_partition(partition,
			partition->ChangeCounter(), diskSystemToInit->Name(), fName,
			fParameters, false);

		if (validationResult != B_OK) {
			SetErrorMessage("Validation of initializing partition failed!");
			return validationResult;
		}

		// everything seems OK -> let's do the job
		locker.Unlock();

		status_t initResult = diskSystemToInit->Initialize(partition, fName,
			fParameters, this);

		if (initResult != B_OK) {
			SetErrorMessage("Initialization of partition failed!");
			return initResult;
		}

		partition->SetDiskSystem(diskSystemToInit);

		return B_OK;

	} else {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}
}
