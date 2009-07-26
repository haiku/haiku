/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CreateChildJob.h"

#include <syscalls.h>

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


// constructor
CreateChildJob::CreateChildJob(PartitionReference* partition,
		PartitionReference* child)
	:
	DiskDeviceJob(partition, child),
	fOffset(0),
	fSize(0),
	fType(NULL),
	fName(NULL),
	fParameters(NULL)
{
}


// destructor
CreateChildJob::~CreateChildJob()
{
	free(fType);
	free(fName);
	free(fParameters);
}


// Init
status_t
CreateChildJob::Init(off_t offset, off_t size, const char* type,
	const char* name, const char* parameters)
{
	fOffset = offset;
	fSize = size;

	SET_STRING_RETURN_ON_ERROR(fType, type);
	SET_STRING_RETURN_ON_ERROR(fName, name);
	SET_STRING_RETURN_ON_ERROR(fParameters, parameters);

	return B_OK;
}


// Do
status_t
CreateChildJob::Do()
{
	int32 changeCounter = fPartition->ChangeCounter();
	partition_id childID;
	int32 childChangeCounter;
	status_t error = _kern_create_child_partition(fPartition->PartitionID(),
		&changeCounter, fOffset, fSize, fType, fName, fParameters, &childID,
		&childChangeCounter);
	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);
	fChild->SetTo(childID, childChangeCounter);

	return B_OK;
}

