/*
 * Copyright 2004-2006, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <util/kernel_cpp.h>

#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobFactory.h"
#include "KResizeJob.h"
#include "KScanPartitionJob.h"
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
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateRepairJob(partition_id partitionID,
	bool checkOnly)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateResizeJob(partition_id parentID,
	partition_id partitionID, off_t size)
{
	return new(nothrow) KResizeJob(parentID, partitionID, size);
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateMoveJob(partition_id parentID, partition_id partitionID,
	off_t offset, const partition_id *contentsToMove, int32 contentsToMoveCount)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetNameJob(partition_id parentID,
	partition_id partitionID, const char *name)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentNameJob(partition_id partitionID,
	const char *name)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetTypeJob(partition_id parentID,
	partition_id partitionID, const char *type)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetParametersJob(partition_id parentID,
	partition_id partitionID, const char *parameters)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateSetContentParametersJob(partition_id partitionID,
	const char *parameters)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateInitializeJob(partition_id partitionID,
	disk_system_id diskSystemID, const char *name, const char *parameters)
{
	// not implemented
	return NULL;
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
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateDeleteChildJob(partition_id parentID,
	partition_id partitionID)
{
	// not implemented
	return NULL;
}


KDiskDeviceJob *
KDiskDeviceJobFactory::CreateScanPartitionJob(partition_id partitionID)
{
	return new(nothrow) KScanPartitionJob(partitionID);
}

