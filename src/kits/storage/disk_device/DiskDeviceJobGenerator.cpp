/*
 * Copyright 2003-2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DiskDeviceJobGenerator.h"

#include <new>

#include <stdlib.h>
#include <string.h>

#include <DiskDevice.h>
#include <MutablePartition.h>

#include <ddm_userland_interface_defs.h>

#include "DiskDeviceJob.h"
#include "DiskDeviceJobQueue.h"
#include "PartitionDelegate.h"
#include "PartitionReference.h"

#include "CreateChildJob.h"
#include "DeleteChildJob.h"
#include "DefragmentJob.h"
#include "InitializeJob.h"
#include "MoveJob.h"
#include "RepairJob.h"
#include "ResizeJob.h"
#include "SetStringJob.h"
#include "UninitializeJob.h"


#undef TRACE
#define TRACE(x...)
//#define TRACE(x...)	printf(x)


using std::nothrow;


// compare_string
/*!	\brief \c NULL aware strcmp().

	\c NULL is considered the least of all strings. \c NULL equals \c NULL.

	\param str1 First string.
	\param str2 Second string.
	\return A value less than 0, if \a str1 is less than \a str2,
			0, if they are equal, or a value greater than 0, if
			\a str1 is greater \a str2.
*/
static inline int
compare_string(const char* str1, const char* str2)
{
	if (str1 == NULL) {
		if (str2 == NULL)
			return 0;
		return 1;
	} else if (str2 == NULL)
		return -1;

	return strcmp(str1, str2);
}


// MoveInfo
struct DiskDeviceJobGenerator::MoveInfo {
	BPartition*	partition;
	off_t		position;
	off_t		target_position;
	off_t		size;
};


// PartitionRefInfo
struct DiskDeviceJobGenerator::PartitionRefInfo {
	PartitionRefInfo()
		: partition(NULL),
		  reference(NULL)
	{
	}

	~PartitionRefInfo()
	{
		if (reference)
			reference->ReleaseReference();
	}

	BPartition*			partition;
	PartitionReference*	reference;
};


// constructor
DiskDeviceJobGenerator::DiskDeviceJobGenerator(BDiskDevice* device,
		DiskDeviceJobQueue* jobQueue)
	: fDevice(device),
	  fJobQueue(jobQueue),
	  fMoveInfos(NULL),
	  fPartitionRefs(NULL),
	  fContentsToMove(NULL),
	  fContentsToMoveCount(0)
{
	// Make sure the arrays are big enough (worst case: all old partitions have
	// been deleted and new ones been created).
	fPartitionCount = fDevice->CountDescendants()
		+ fDevice->_CountDescendants();

	fMoveInfos = new(nothrow) MoveInfo[fPartitionCount];
	fPartitionRefs = new(nothrow) PartitionRefInfo[fPartitionCount];
	fContentsToMove = new(nothrow) PartitionReference*[fPartitionCount];
}


// destructor
DiskDeviceJobGenerator::~DiskDeviceJobGenerator()
{
	delete[] fMoveInfos;
	delete[] fPartitionRefs;
	delete[] fContentsToMove;
}


// GenerateJobs
status_t
DiskDeviceJobGenerator::GenerateJobs()
{
	// check parameters
	if (!fDevice || !fJobQueue)
		return B_BAD_VALUE;

	if (!fMoveInfos || !fPartitionRefs || !fContentsToMove)
		return B_NO_MEMORY;

	// 1) Generate jobs for all physical partitions that don't have an
	// associated shadow partition, i.e. those that shall be deleted.
	// 2) Generate uninitialize jobs for all partition whose initialization
	// changes, also those that shall be initialized with a disk system.
	// This simplifies moving and resizing.
	status_t error = _GenerateCleanupJobs(fDevice);
	if (error != B_OK) {
		TRACE("DiskDeviceJobGenerator::GenerateJobs(): _GenerateCleanupJobs() "
			"failed\n");
		return error;
	}

	// Generate jobs that move and resize the remaining physical partitions
	// to their final position/size.
	error = _GeneratePlacementJobs(fDevice);
	if (error != B_OK) {
		TRACE("DiskDeviceJobGenerator::GenerateJobs(): "
			"_GeneratePlacementJobs() failed\n");
		return error;
	}

	// Generate the remaining jobs in one run: initialization, creation of
	// partitions, and changing of name, content name, type, parameters, and
	// content parameters.
	error = _GenerateRemainingJobs(NULL, fDevice);
	if (error != B_OK) {
		TRACE("DiskDeviceJobGenerator::GenerateJobs(): "
			"_GenerateRemainingJobs() failed\n");
		return error;
	}

	TRACE("DiskDeviceJobGenerator::GenerateJobs(): succeeded\n");

	return B_OK;
}


// _AddJob
status_t
DiskDeviceJobGenerator::_AddJob(DiskDeviceJob* job)
{
	if (!job)
		return B_NO_MEMORY;

	status_t error = fJobQueue->AddJob(job);
	if (error != B_OK)
		delete job;

	return error;
}


// _GenerateCleanupJobs
status_t
DiskDeviceJobGenerator::_GenerateCleanupJobs(BPartition* partition)
{
// TODO: Depending on how this shall be handled, we might want to unmount
// all descendants of a partition to be uninitialized or removed.
	if (BMutablePartition* shadow = _GetMutablePartition(partition)) {
		if ((shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION)
			&& partition->fPartitionData->content_type) {
			// partition changes initialization
			status_t error = _GenerateUninitializeJob(partition);
			if (error != B_OK)
				return error;
		} else {
			// recurse
			for (int32 i = 0; BPartition* child = partition->_ChildAt(i); i++) {
				status_t error = _GenerateCleanupJobs(child);
				if (error != B_OK)
					return error;
			}
		}
	} else if (BPartition* parent = partition->Parent()) {
		// create job and add it to the queue
		status_t error = _GenerateDeleteChildJob(parent, partition);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}


// _GeneratePlacementJobs
status_t
DiskDeviceJobGenerator::_GeneratePlacementJobs(BPartition* partition)
{
	if (BMutablePartition* shadow = _GetMutablePartition(partition)) {
		// Don't resize/move partitions that have an unrecognized contents.
		// They must have been uninitialized before.
		if (shadow->Status() == B_PARTITION_UNRECOGNIZED
			&& (shadow->Size() != partition->Size()
				|| shadow->Offset() != partition->Offset())) {
			return B_ERROR;
		}

		if (shadow->Size() > partition->Size()) {
			// size grows: resize first
			status_t error = _GenerateResizeJob(partition);
			if (error != B_OK)
				return error;
		}

		// place the children
		status_t error = _GenerateChildPlacementJobs(partition);
		if (error != B_OK)
			return error;

		if (shadow->Size() < partition->Size()) {
			// size shrinks: resize now
			status_t error = _GenerateResizeJob(partition);
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


// _GenerateChildPlacementJobs
status_t
DiskDeviceJobGenerator::_GenerateChildPlacementJobs(BPartition* partition)
{
	BMutablePartition* shadow = _GetMutablePartition(partition);

	// nothing to do, if the partition contains no partitioning system or
	// shall be re-initialized
	if (!shadow->ContentType()
		|| (shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION)) {
		return B_OK;
	}

	// first resize all children that shall shrink and place their descendants
	int32 childCount = 0;
	int32 moveForth = 0;
	int32 moveBack = 0;

	for (int32 i = 0; BPartition* child = partition->_ChildAt(i); i++) {
		if (BMutablePartition* childShadow = _GetMutablePartition(child)) {
			// add a MoveInfo for the child
			MoveInfo& info = fMoveInfos[childCount];
			childCount++;
			info.partition = child;
			info.position = child->Offset();
			info.target_position = childShadow->Offset();
			info.size = child->Size();

			if (info.position < info.target_position)
				moveForth++;
			else if (info.position > info.target_position)
				moveBack++;

			// resize the child, if it shall shrink
			if (childShadow->Size() < child->Size()) {
				status_t error = _GeneratePlacementJobs(child);
				if (error != B_OK)
					return error;
				info.size = childShadow->Size();
			}
		}
	}

	// sort the move infos
	if (childCount > 0 && moveForth + moveBack > 0) {
		qsort(fMoveInfos, childCount, sizeof(MoveInfo),
			_CompareMoveInfoPosition);
	}

	// move the children to their final positions
	while (moveForth + moveBack > 0) {
		int32 moved = 0;
		if (moveForth < moveBack) {
			// move children back
			for (int32 i = 0; i < childCount; i++) {
				MoveInfo& info = fMoveInfos[i];
				if (info.position > info.target_position) {
					if (i == 0
						|| info.target_position >= fMoveInfos[i - 1].position
							+ fMoveInfos[i - 1].size) {
						// check OK -- the partition wouldn't be moved before
						// the end of the preceding one
						status_t error = _GenerateMoveJob(info.partition);
						if (error != B_OK)
							return error;
						info.position = info.target_position;
						moved++;
						moveBack--;
					}
				}
			}
		} else {
			// move children forth
			for (int32 i = childCount - 1; i >= 0; i--) {
				MoveInfo &info = fMoveInfos[i];
				if (info.position > info.target_position) {
					if (i == childCount - 1
						|| info.target_position + info.size
							<= fMoveInfos[i - 1].position) {
						// check OK -- the partition wouldn't be moved before
						// the end of the preceding one
						status_t error = _GenerateMoveJob(info.partition);
						if (error != B_OK)
							return error;
						info.position = info.target_position;
						moved++;
						moveForth--;
					}
				}
			}
		}

		// terminate, if no partition could be moved
		if (moved == 0)
			return B_ERROR;
	}

	// now resize all children that shall grow/keep their size and place
	// their descendants
	for (int32 i = 0; BPartition* child = partition->_ChildAt(i); i++) {
		if (BMutablePartition* childShadow = _GetMutablePartition(child)) {
			if (childShadow->Size() >= child->Size()) {
				status_t error = _GeneratePlacementJobs(child);
				if (error != B_OK)
					return error;
			}
		}
	}

	return B_OK;
}


// _GenerateRemainingJobs
status_t
DiskDeviceJobGenerator::_GenerateRemainingJobs(BPartition* parent,
	BPartition* partition)
{
	user_partition_data* partitionData = partition->fPartitionData;

	uint32 changeFlags
		= partition->fDelegate->MutablePartition()->ChangeFlags();

	// create the partition, if not existing yet
	if (!partitionData) {
		if (!parent)
			return B_BAD_VALUE;

		status_t error = _GenerateCreateChildJob(parent, partition);
		if (error != B_OK)
			return error;
	} else {
		// partition already exists: set non-content properties

		// name
		if ((changeFlags & B_PARTITION_CHANGED_NAME)
			|| compare_string(partition->Name(), partitionData->name)) {
			if (!parent)
				return B_BAD_VALUE;

			status_t error = _GenerateSetNameJob(parent, partition);
			if (error != B_OK)
				return error;
		}

		// type
		if ((changeFlags & B_PARTITION_CHANGED_TYPE)
			|| compare_string(partition->Type(), partitionData->type)) {
			if (!parent)
				return B_BAD_VALUE;

			status_t error = _GenerateSetTypeJob(parent, partition);
			if (error != B_OK)
				return error;
		}

		// parameters
		if ((changeFlags & B_PARTITION_CHANGED_PARAMETERS)
			|| compare_string(partition->Parameters(),
				partitionData->parameters)) {
			if (!parent)
				return B_BAD_VALUE;

			status_t error = _GenerateSetParametersJob(parent, partition);
			if (error != B_OK)
				return error;
		}
	}

	if (partition->ContentType()) {
		// initialize the partition, if required
		if (changeFlags & B_PARTITION_CHANGED_INITIALIZATION) {
			status_t error = _GenerateInitializeJob(partition);
			if (error != B_OK)
				return error;
		} else {
			// partition not (re-)initialized, set content properties

			// content name
			if ((changeFlags & B_PARTITION_CHANGED_NAME)
				|| compare_string(partition->ContentName(),
					partitionData->content_name)) {
				status_t error = _GenerateSetContentNameJob(partition);
				if (error != B_OK)
					return error;
			}

			// content parameters
			if ((changeFlags & B_PARTITION_CHANGED_PARAMETERS)
				|| compare_string(partition->ContentParameters(),
					partitionData->content_parameters)) {
				status_t error = _GenerateSetContentParametersJob(partition);
				if (error != B_OK)
					return error;
			}

			// defragment
			if (changeFlags & B_PARTITION_CHANGED_DEFRAGMENTATION) {
				status_t error = _GenerateDefragmentJob(partition);
				if (error != B_OK)
					return error;
			}

			// check / repair
			bool repair = (changeFlags & B_PARTITION_CHANGED_REPAIR);
			if ((changeFlags & B_PARTITION_CHANGED_CHECK)
				|| repair) {
				status_t error = _GenerateRepairJob(partition, repair);
				if (error != B_OK)
					return error;
			}
		}
	}

	// recurse
	for (int32 i = 0; BPartition* child = partition->ChildAt(i); i++) {
		status_t error = _GenerateRemainingJobs(partition, child);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// _GetMutablePartition
BMutablePartition*
DiskDeviceJobGenerator::_GetMutablePartition(BPartition* partition)
{
	if (!partition)
		return NULL;

	return partition->fDelegate
		? partition->fDelegate->MutablePartition() : NULL;
}


// _GenerateInitializeJob
status_t
DiskDeviceJobGenerator::_GenerateInitializeJob(BPartition* partition)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	InitializeJob* job = new(nothrow) InitializeJob(reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->ContentType(),
		partition->ContentName(), partition->ContentParameters());
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateUninitializeJob
status_t
DiskDeviceJobGenerator::_GenerateUninitializeJob(BPartition* partition)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	return _AddJob(new(nothrow) UninitializeJob(reference));
}


// _GenerateSetContentNameJob
status_t
DiskDeviceJobGenerator::_GenerateSetContentNameJob(BPartition* partition)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	SetStringJob* job = new(nothrow) SetStringJob(reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->ContentName(),
		B_DISK_DEVICE_JOB_SET_CONTENT_NAME);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateSetContentParametersJob
status_t
DiskDeviceJobGenerator::_GenerateSetContentParametersJob(BPartition* partition)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	SetStringJob* job = new(nothrow) SetStringJob(reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->ContentParameters(),
		B_DISK_DEVICE_JOB_SET_CONTENT_PARAMETERS);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateDefragmentJob
status_t
DiskDeviceJobGenerator::_GenerateDefragmentJob(BPartition* partition)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	return _AddJob(new(nothrow) DefragmentJob(reference));
}


// _GenerateRepairJob
status_t
DiskDeviceJobGenerator::_GenerateRepairJob(BPartition* partition, bool repair)
{
	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	return _AddJob(new(nothrow) RepairJob(reference, repair));
}


// _GenerateCreateChildJob
status_t
DiskDeviceJobGenerator::_GenerateCreateChildJob(BPartition* parent,
	BPartition* partition)
{
	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	CreateChildJob* job = new(nothrow) CreateChildJob(parentReference,
		reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->Offset(), partition->Size(), partition->Type(),
		partition->Name(), partition->Parameters());
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateDeleteChildJob
status_t
DiskDeviceJobGenerator::_GenerateDeleteChildJob(BPartition* parent,
	BPartition* partition)
{
	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	return _AddJob(new(nothrow) DeleteChildJob(parentReference, reference));
}


// _GenerateResizeJob
status_t
DiskDeviceJobGenerator::_GenerateResizeJob(BPartition* partition)
{
	BPartition* parent = partition->Parent();
	if (!parent)
		return B_BAD_VALUE;

	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	return _AddJob(new(nothrow) ResizeJob(parentReference, reference,
		partition->Size(), partition->ContentSize()));
}


// _GenerateMoveJob
status_t
DiskDeviceJobGenerator::_GenerateMoveJob(BPartition* partition)
{
	BPartition* parent = partition->Parent();
	if (!parent)
		return B_BAD_VALUE;

	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	// collect all descendants whose contents need to be moved
	fContentsToMoveCount = 0;
	error = _CollectContentsToMove(partition);
	if (error != B_OK)
		return B_OK;

	// create and init the job
	MoveJob* job = new(nothrow) MoveJob(parentReference, reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->Offset(), fContentsToMove,
		fContentsToMoveCount);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateSetNameJob
status_t
DiskDeviceJobGenerator::_GenerateSetNameJob(BPartition* parent,
	BPartition* partition)
{
	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	SetStringJob* job = new(nothrow) SetStringJob(parentReference, reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->Name(), B_DISK_DEVICE_JOB_SET_NAME);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateSetTypeJob
status_t
DiskDeviceJobGenerator::_GenerateSetTypeJob(BPartition* parent,
	BPartition* partition)
{
	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	SetStringJob* job = new(nothrow) SetStringJob(parentReference, reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->Type(), B_DISK_DEVICE_JOB_SET_TYPE);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _GenerateSetParametersJob
status_t
DiskDeviceJobGenerator::_GenerateSetParametersJob(BPartition* parent,
	BPartition* partition)
{
	PartitionReference* parentReference;
	status_t error = _GetPartitionReference(parent, parentReference);
	if (error != B_OK)
		return error;

	PartitionReference* reference;
	error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	SetStringJob* job = new(nothrow) SetStringJob(parentReference, reference);
	if (!job)
		return B_NO_MEMORY;

	error = job->Init(partition->Parameters(),
		B_DISK_DEVICE_JOB_SET_PARAMETERS);
	if (error != B_OK) {
		delete job;
		return error;
	}

	return _AddJob(job);
}


// _CollectContentsToMove
status_t
DiskDeviceJobGenerator::_CollectContentsToMove(BPartition* partition)
{
	BMutablePartition* shadow = _GetMutablePartition(partition);
	if (shadow->Status() == B_PARTITION_UNRECOGNIZED)
		return B_ERROR;

	// if the partition has contents, push its ID
	if (shadow->ContentType()
		&& !(shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION)) {
		status_t error = _PushContentsToMove(partition);
		if (error != B_OK)
			return error;
	}

	// recurse
	for (int32 i = 0; BPartition* child = partition->ChildAt(i); i++) {
		status_t error = _CollectContentsToMove(child);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}


// _PushContentsToMove
status_t
DiskDeviceJobGenerator::_PushContentsToMove(BPartition* partition)
{
	if (fContentsToMoveCount >= fPartitionCount)
		return B_ERROR;

	PartitionReference* reference;
	status_t error = _GetPartitionReference(partition, reference);
	if (error != B_OK)
		return error;

	fContentsToMove[fContentsToMoveCount++] = reference;

	return B_OK;
}


// _GetPartitionReference
status_t
DiskDeviceJobGenerator::_GetPartitionReference(BPartition* partition,
	PartitionReference*& reference)
{
	if (!partition)
		return B_BAD_VALUE;

	for (int32 i = 0; i < fPartitionCount; i++) {
		PartitionRefInfo& info = fPartitionRefs[i];

		if (info.partition == partition) {
			reference = info.reference;
			return B_OK;
		}

		if (info.partition == NULL) {
			// create partition reference
			info.reference = new(nothrow) PartitionReference();
			if (!info.reference)
				return B_NO_MEMORY;

			// set partition ID and change counter
			user_partition_data* partitionData = partition->fPartitionData;
			if (partitionData) {
				info.reference->SetPartitionID(partitionData->id);
				info.reference->SetChangeCounter(partitionData->change_counter);
			}

			info.partition = partition;
			reference = info.reference;
			return B_OK;
		}
	}

	// Out of slots -- that can't happen.
	return B_ERROR;
}


// _CompareMoveInfoOffset
int
DiskDeviceJobGenerator::_CompareMoveInfoPosition(const void* _a, const void* _b)
{
	const MoveInfo* a = static_cast<const MoveInfo*>(_a);
	const MoveInfo* b = static_cast<const MoveInfo*>(_b);
	if (a->position < b->position)
		return -1;
	if (a->position > b->position)
		return 1;
	return 0;
}
