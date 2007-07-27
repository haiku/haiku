/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <util/kernel_cpp.h>

#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobFactory.h"

#include "KCreateChildJob.h"
#include "KDefragmentJob.h"
#include "KDeleteChildJob.h"
#include "KInitializeJob.h"
#include "KMoveJob.h"
#include "KRepairJob.h"
#include "KResizeJob.h"
#include "KScanPartitionJob.h"
#include "KSetNameJob.h"
#include "KSetParametersJob.h"
#include "KSetTypeJob.h"
#include "KUninitializeJob.h"


using namespace std;


KDiskDeviceJobFactory::KDiskDeviceJobFactory()
{
}


KDiskDeviceJobFactory::~KDiskDeviceJobFactory()
{
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateDefragmentJob(partition_id partitionID)
{
	return new(nothrow) KDefragmentJob(partitionID);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateRepairJob(partition_id partitionID,
	bool checkOnly)
{
	return new(nothrow) KRepairJob(partitionID, checkOnly);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateResizeJob(partition_id parentID,
	partition_id partitionID, off_t size)
{
	return new(nothrow) KResizeJob(parentID, partitionID, size);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateMoveJob(partition_id parentID,
	partition_id partitionID, off_t offset, const partition_id *contentsToMove,
	int32 contentsToMoveCount)
{
	// TODO: this is wierd, what in hell are contentsToMove etc?
	return new(nothrow) KMoveJob(parentID, partitionID, offset);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetNameJob(partition_id parentID,
	partition_id partitionID, const char *name)
{
	return new(nothrow) KSetNameJob(parentID, partitionID, name, 0);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentNameJob(partition_id partitionID,
	const char *name)
{
	return new(nothrow) KSetNameJob(0, partitionID, 0, name);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetTypeJob(partition_id parentID,
	partition_id partitionID, const char *type)
{
	return new(nothrow) KSetTypeJob(parentID, partitionID, type);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetParametersJob(partition_id parentID,
	partition_id partitionID, const char *parameters)
{
	return new(nothrow) KSetParametersJob(parentID, partitionID, parameters, 0);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentParametersJob(partition_id partitionID,
	const char *parameters)
{
	return new(nothrow) KSetParametersJob(0, partitionID, 0, parameters);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateInitializeJob(partition_id partitionID,
	disk_system_id diskSystemID, const char *name, const char *parameters)
{
	return new(nothrow) KInitializeJob(partitionID, diskSystemID, name,
		parameters);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateUninitializeJob(partition_id partitionID)
{
	return new(nothrow) KUninitializeJob(partitionID);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateCreateChildJob(partition_id partitionID,
	partition_id childID, off_t offset, off_t size, const char *type,
	const char *parameters)
{
	return new(nothrow) KCreateChildJob(partitionID, childID, offset, size,
		type, parameters);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateDeleteChildJob(partition_id parentID,
	partition_id partitionID)
{
	return new(nothrow) KDeleteChildJob(parentID, partitionID);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateScanPartitionJob(partition_id partitionID)
{
	return new(nothrow) KScanPartitionJob(partitionID);
}

