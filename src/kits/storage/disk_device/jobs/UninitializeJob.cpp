/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UninitializeJob.h"

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
// Implement!
	return B_BAD_VALUE;
}

