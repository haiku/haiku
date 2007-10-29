/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DeleteChildJob.h"

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
// Implement!
	return B_BAD_VALUE;
}

