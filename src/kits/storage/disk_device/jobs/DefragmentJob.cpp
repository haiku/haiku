/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DefragmentJob.h"

#include "PartitionReference.h"


// constructor
DefragmentJob::DefragmentJob(PartitionReference* partition)
	: DiskDeviceJob(partition)
{
}


// destructor
DefragmentJob::~DefragmentJob()
{
}


// Do
status_t
DefragmentJob::Do()
{
// Implement!
	return B_BAD_VALUE;
}

