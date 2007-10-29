/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include "MoveJob.h"

#include "DiskDeviceUtils.h"
#include "PartitionReference.h"


using std::nothrow;


// constructor
MoveJob::MoveJob(PartitionReference* partition, PartitionReference* child)
	: DiskDeviceJob(partition, child),
	  fContents(NULL),
	  fContentsCount(0)
{
}


// destructor
MoveJob::~MoveJob()
{
	if (fContents) {
		for (int32 i = 0; i < fContentsCount; i++)
			fContents[i]->RemoveReference();
		delete[] fContents;
	}
}


// Init
status_t
MoveJob::Init(off_t offset, PartitionReference** contents, int32 contentsCount)
{
	fContents = new(nothrow) PartitionReference*[contentsCount];
	if (!fContents)
		return B_NO_MEMORY;

	fContentsCount = contentsCount;
	for (int32 i = 0; i < contentsCount; i++) {
		fContents[i] = contents[i];
		fContents[i]->AddReference();
	}

	fOffset = offset;

	return B_OK;
}


// Do
status_t
MoveJob::Do()
{
// Implement!
	return B_BAD_VALUE;
}

