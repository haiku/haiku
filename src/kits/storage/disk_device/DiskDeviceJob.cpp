/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DiskDeviceJob.h"

#include "PartitionReference.h"


// constructor
DiskDeviceJob::DiskDeviceJob(PartitionReference* partition,
		PartitionReference* child)
	:
	fPartition(partition),
	fChild(child)
{
	if (fPartition)
		fPartition->AcquireReference();

	if (fChild)
		fChild->AcquireReference();
}


// destructor
DiskDeviceJob::~DiskDeviceJob()
{
	if (fPartition)
		fPartition->ReleaseReference();

	if (fChild)
		fChild->ReleaseReference();
}
