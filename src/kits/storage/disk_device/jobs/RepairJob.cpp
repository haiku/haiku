/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "RepairJob.h"

#include "PartitionReference.h"


// constructor
RepairJob::RepairJob(PartitionReference* partition, bool checkOnly)
	: DiskDeviceJob(partition),
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
// Implement!
	return B_BAD_VALUE;
}

