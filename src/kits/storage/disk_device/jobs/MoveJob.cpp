/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "MoveJob.h"

#include <new>

#include <AutoDeleter.h>

#include <syscalls.h>

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
			fContents[i]->ReleaseReference();
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
		fContents[i]->AcquireReference();
	}

	fOffset = offset;

	return B_OK;
}


// Do
status_t
MoveJob::Do()
{
	int32 changeCounter = fPartition->ChangeCounter();
	int32 childChangeCounter = fChild->ChangeCounter();

	partition_id* descendantIDs = new(nothrow) partition_id[fContentsCount];
	int32* descendantChangeCounters = new(nothrow) int32[fContentsCount];
	ArrayDeleter<partition_id> _(descendantIDs);
	ArrayDeleter<int32> _2(descendantChangeCounters);

	if (!descendantIDs || !descendantChangeCounters)
		return B_NO_MEMORY;

	for (int32 i = 0; i < fContentsCount; i++) {
		descendantIDs[i] = fContents[i]->PartitionID();
		descendantChangeCounters[i] = fContents[i]->ChangeCounter();
	}

	status_t error = _kern_move_partition(fPartition->PartitionID(),
		&changeCounter, fChild->PartitionID(), &childChangeCounter, fOffset,
		descendantIDs, descendantChangeCounters, fContentsCount);

	if (error != B_OK)
		return error;

	fPartition->SetChangeCounter(changeCounter);
	fChild->SetChangeCounter(childChangeCounter);

	for (int32 i = 0; i < fContentsCount; i++)
		fContents[i]->SetChangeCounter(descendantChangeCounters[i]);

	return B_OK;
}

