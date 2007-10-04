/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <stdio.h>
#include <string.h>

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
	if (!partition) {
		SetErrorMessage("Couldn't find partition.");
		return B_ENTRY_NOT_FOUND;
	}

	PartitionRegistrar registrar(partition, true);
	PartitionRegistrar deviceRegistrar(partition->Device(), true);

	DeviceWriteLocker locker(partition->Device(), true);

	// basic checks

	// all descendants should be marked busy/descendant busy
	if (IsPartitionNotBusy(partition)) {
		SetErrorMessage("Can't initialize non-busy partition!");
		return B_ERROR;
	}

	if (partition->DiskSystem()) {
		SetErrorMessage("Partition is already initialized.");
		return B_BAD_VALUE;
	}

	// get the disk system
	KDiskSystem *diskSystemToInit = manager->LoadDiskSystem(fDiskSystemID);
	if (!diskSystemToInit) {
		SetErrorMessage("Given DiskSystemID doesn't correspond to any "
			"known DiskSystem.");
		return B_BAD_VALUE;
	}
	DiskSystemLoader loader(diskSystemToInit, true);

	// check the parameters
	char name[B_DISK_DEVICE_NAME_LENGTH];
	if (fName)
		strlcpy(name, fName, sizeof(name));

	status_t error = validate_initialize_partition(partition,
		partition->ChangeCounter(), diskSystemToInit->Name(),
		fName ? name : NULL, fParameters, false);

	if (error != B_OK) {
		SetErrorMessage("Validation of initializing partition failed.");
		return error;
	}

	if (fName != NULL && strcmp(fName, name) != 0) {
		SetErrorMessage("Requested name not valid.");
		return B_BAD_VALUE;
	}

	// everything seems OK -> let's do the job
	locker.Unlock();

	error = diskSystemToInit->Initialize(partition, fName, fParameters, this);
	if (error != B_OK) {
		SetErrorMessage("Initialization of partition failed!");
		return error;
	}

	locker.Lock();

	if (partition->DiskSystem()) {
		// The disk system might have triggered a rescan after initializing, in
		// which case the disk system is already set. Just check, if it is the
		// right one.
		if (partition->DiskSystem() != diskSystemToInit) {
			SetErrorMessage("Partition has wrong disk system after "
				"initialization.");
			return B_ERROR;
		}
	} else
		partition->SetDiskSystem(diskSystemToInit);

	return B_OK;
}
