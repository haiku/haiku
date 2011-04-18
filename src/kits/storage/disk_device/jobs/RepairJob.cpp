/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "RepairJob.h"

#include <syscalls.h>

#include "PartitionReference.h"


// constructor
RepairJob::RepairJob(PartitionReference* partition, bool checkOnly)
	:
	DiskDeviceJob(partition),
	fCheckOnly(checkOnly)
{
}


// destructor
RepairJob::~RepairJob()
{
}


// Do
status_t
RepairJob::Do()
{
	int32 changeCounter = fPartition->ChangeCounter();
	status_t error = _kern_repair_partition(fPartition->PartitionID(),
		&changeCounter, fCheckOnly);
	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);

	return B_OK;
}

