/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DiskDeviceJob.h"

#include "PartitionReference.h"


// constructor
DiskDeviceJob::DiskDeviceJob(PartitionReference* partition,
		PartitionReference* child)
	: fPartition(partition),
	  fChild(child)
{
	if (fPartition)
		fPartition->AddReference();

	if (fChild)
		fChild->AddReference();
}


// destructor
DiskDeviceJob::~DiskDeviceJob()
{
	if (fPartition)
		fPartition->RemoveReference();

	if (fChild)
		fChild->RemoveReference();
}
