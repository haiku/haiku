/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ResizeJob.h"

#include <syscalls.h>

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
ResizeJob::ResizeJob(PartitionReference* partition, PartitionReference* child,
		off_t size, off_t contentSize)
	:
	DiskDeviceJob(partition, child),
	fSize(size),
	fContentSize(contentSize)
{
}


// destructor
ResizeJob::~ResizeJob()
{
}


// Do
status_t
ResizeJob::Do()
{
	int32 changeCounter = fPartition->ChangeCounter();
	int32 childChangeCounter = fChild->ChangeCounter();
	status_t error = _kern_resize_partition(fPartition->PartitionID(),
		&changeCounter, fChild->PartitionID(), &childChangeCounter, fSize,
		fContentSize);
	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);
	fChild->SetChangeCounter(childChangeCounter);

	return B_OK;
}

