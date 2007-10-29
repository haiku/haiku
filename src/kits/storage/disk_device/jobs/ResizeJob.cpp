/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ResizeJob.h"

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
ResizeJob::ResizeJob(PartitionReference* partition, PartitionReference* child,
		off_t size, off_t contentSize)
	: DiskDeviceJob(partition, child),
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
// Implement!
	return B_BAD_VALUE;
}

