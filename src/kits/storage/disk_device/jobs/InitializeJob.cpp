/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "InitializeJob.h"

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
InitializeJob::InitializeJob(PartitionReference* partition)
	: DiskDeviceJob(partition),
	  fDiskSystem(NULL),
	  fName(NULL),
	  fParameters(NULL)
{
}


// destructor
InitializeJob::~InitializeJob()
{
	free(fDiskSystem);
	free(fName);
	free(fParameters);
}


// Init
status_t
InitializeJob::Init(const char* diskSystem, const char* name,
	const char* parameters)
{
	SET_STRING_RETURN_ON_ERROR(fDiskSystem, diskSystem);
	SET_STRING_RETURN_ON_ERROR(fName, name);
	SET_STRING_RETURN_ON_ERROR(fParameters, parameters);

	return B_OK;
}


// Do
status_t
InitializeJob::Do()
{
// Implement!
	return B_BAD_VALUE;
}

