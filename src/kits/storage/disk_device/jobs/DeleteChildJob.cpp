/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DeleteChildJob.h"

#include <syscalls.h>

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
DeleteChildJob::DeleteChildJob(PartitionReference* partition,
		PartitionReference* child)
	: DiskDeviceJob(partition, child)
{
}


// destructor
DeleteChildJob::~DeleteChildJob()
{
}


// Do
status_t
DeleteChildJob::Do()
{
	int32 changeCounter = fPartition->ChangeCounter();
	status_t error = _kern_delete_child_partition(fPartition->PartitionID(),
		&changeCounter, fChild->PartitionID(), fChild->ChangeCounter());
	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);
	fChild->SetTo(-1, 0);

	return B_OK;
}

