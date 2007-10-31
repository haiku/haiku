/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UninitializeJob.h"

#include <syscalls.h>

#include "PartitionReference.h"


// constructor
UninitializeJob::UninitializeJob(PartitionReference* partition)
	: DiskDeviceJob(partition)
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
	int32 changeCounter = fPartition->ChangeCounter();
	status_t error = _kern_uninitialize_partition(fPartition->PartitionID(),
		&changeCounter);

	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);

	return B_OK;
}

