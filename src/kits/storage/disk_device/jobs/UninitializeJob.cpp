/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UninitializeJob.h"

#include <syscalls.h>

#include "PartitionReference.h"


// constructor
UninitializeJob::UninitializeJob(PartitionReference* partition,
		PartitionReference* parent)
	: DiskDeviceJob(parent, partition)
{
}


// destructor
UninitializeJob::~UninitializeJob()
{
}


// Do
status_t
UninitializeJob::Do()
{
	bool haveParent = fPartition != NULL;
	int32 changeCounter = fChild->ChangeCounter();
	int32 parentChangeCounter = haveParent ? fPartition->ChangeCounter() : 0;
	partition_id parentID = haveParent ? fPartition->PartitionID() : -1;

	status_t error = _kern_uninitialize_partition(fChild->PartitionID(),
		&changeCounter, parentID, &parentChangeCounter);

	if (error != B_OK)
		return error;

	fChild->SetChangeCounter(changeCounter);
	if (haveParent)
		fPartition->SetChangeCounter(parentChangeCounter);

	return B_OK;
}

