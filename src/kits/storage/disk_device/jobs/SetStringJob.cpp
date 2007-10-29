/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SetStringJob.h"

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
SetStringJob::SetStringJob(PartitionReference* partition,
		PartitionReference* child)
	: DiskDeviceJob(partition, child),
	  fString(NULL)
{
}


// destructor
SetStringJob::~SetStringJob()
{
	free(fString);
}


// Init
status_t
SetStringJob::Init(const char* string, uint32 jobType)
{
	switch (jobType) {
		case B_DISK_DEVICE_JOB_SET_NAME:
		case B_DISK_DEVICE_JOB_SET_CONTENT_NAME:
		case B_DISK_DEVICE_JOB_SET_TYPE:
		case B_DISK_DEVICE_JOB_SET_PARAMETERS:
		case B_DISK_DEVICE_JOB_SET_CONTENT_PARAMETERS:
			break;
		default:
			return B_BAD_VALUE;
	}

	fJobType = jobType;
	SET_STRING_RETURN_ON_ERROR(fString, string);

	return B_OK;
}


// Do
status_t
SetStringJob::Do()
{
// Implement!
	return B_BAD_VALUE;
}

