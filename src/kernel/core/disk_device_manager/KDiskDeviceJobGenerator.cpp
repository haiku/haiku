// KDiskDeviceJobGenerator.cpp

#include "KDiskDevice.h"
#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobFactory.h"
#include "KDiskDeviceJobQueue.h"
#include "KDiskDeviceJobGenerator.h"
#include "KDiskDeviceUtils.h"
#include "KShadowPartition.h"

// move_info
struct KDiskDeviceJobGenerator::move_info {
	KPartition	*partition;
	off_t		position;
	off_t		target_position;
	off_t		size;
};

// constructor
KDiskDeviceJobGenerator::KDiskDeviceJobGenerator(
	KDiskDeviceJobFactory *jobFactory, KDiskDevice *device,
	KDiskDeviceJobQueue *jobQueue)
	: fJobFactory(jobFactory),
	  fDevice(device),
	  fJobQueue(jobQueue),
	  fMoveInfos(NULL),
	  fMoveInfoCount(0),
	  fPartitionIDs(NULL),
	  fPartitionIDCount(0)
{
	if (fDevice && !fJobQueue) {
		fJobQueue = new(nothrow) KDiskDeviceJobQueue(fDevice);
		if (fDevice->ShadowPartition()) {
			fMoveInfoCount = max(fDevice->CountDescendants(),
				fDevice->ShadowPartition()->CountDescendants());
		}
		fMoveInfos = new(nothrow) move_info[fMoveInfoCount];
		fPartitionIDs = new(nothrow) partition_id[fMoveInfoCount];
	}
}

// destructor
KDiskDeviceJobGenerator::~KDiskDeviceJobGenerator()
{
	delete[] fMoveInfos;
	delete[] fPartitionIDs;
}

// JobFactory
KDiskDeviceJobFactory *
KDiskDeviceJobGenerator::JobFactory() const
{
	return fJobFactory;
}

// Device
KDiskDevice *
KDiskDeviceJobGenerator::Device() const
{
	return fDevice;
}

// JobQueue
KDiskDeviceJobQueue *
KDiskDeviceJobGenerator::JobQueue() const
{
	return fJobQueue;
}

// GenerateJobs
status_t
KDiskDeviceJobGenerator::GenerateJobs()
{
	// check parameters
	if (!fJobFactory || !fDevice || !fJobQueue || !fMoveInfos || !fPartitionIDs
		|| fJobQueue->Device() != fDevice || !fDevice->ShadowPartition()) {
		return B_BAD_VALUE;
	}
	// Generate jobs for all physical partitions that don't have an associated
	// shadow partition, i.e. those that shall be deleted.
	status_t error = _GenerateDeleteChildJobs(fDevice);
	if (error != B_OK)
		return error;
	// Generate uninitialize jobs for all partition whose initialization
	// changes, also those that shall be initialized with a disk system.
	// This simplifies moving and resizing.
	error = _GenerateUninitializeJobs(fDevice);
	if (error != B_OK)
		return error;
	// Generate jobs that move and resize the remaining physical partitions
	// to their final position/size.
	error = _GeneratePlacementJobs(fDevice);
	if (error != B_OK)
		return error;
	return B_ERROR;
}

// _AddJob
status_t
KDiskDeviceJobGenerator::_AddJob(KDiskDeviceJob *job)
{
	if (!job)
		return B_NO_MEMORY;
	if (!fJobQueue->AddJob(job)) {
		delete job;
		return B_NO_MEMORY;
	}
	return B_OK;
}


// _GenerateDeleteChildJobs
status_t
KDiskDeviceJobGenerator::_GenerateDeleteChildJobs(KPartition *partition)
{
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		if (child->ShadowPartition()) {
			// recurse
			status_t error = _GenerateDeleteChildJobs(child);
			if (error != B_OK)
				return error;
		} else {
			// create job and add it to the queue
			status_t error = _AddJob(fJobFactory->CreateDeleteChildJob(
				partition->ID(), child->ID()));
			if (error != B_OK)
				return error;
		}
	}
	return B_OK;
}

// _GenerateUninitializeJobs
status_t
KDiskDeviceJobGenerator::_GenerateUninitializeJobs(KPartition *partition)
{
	if (KPartition *shadow = partition->ShadowPartition()) {
		if (shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION) {
			// partition changes initialization
			status_t error = _AddJob(fJobFactory->CreateUninitializeJob(
				partition->ID()));
			if (error != B_OK)
				return error;
		} else {
			// recurse
			for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
				status_t error = _GenerateUninitializeJobs(child);
				if (error != B_OK)
					return error;
			}
		}
	}
	return B_OK;
}

// _GeneratePlacementJobs
status_t
KDiskDeviceJobGenerator::_GeneratePlacementJobs(KPartition *partition)
{
	if (KPartition *shadow = partition->ShadowPartition()) {
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

// _GenerateResizeJob
status_t
KDiskDeviceJobGenerator::_GenerateResizeJob(KPartition *partition)
{
	KPartition *shadow = partition->ShadowPartition();
	if (!shadow)
		return B_BAD_VALUE;
	if (!partition->Parent())
		return B_ERROR;
	bool hasContents = shadow->DiskSystem()
		&& !(shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION);
	return _AddJob(fJobFactory->CreateResizeJob(partition->Parent()->ID(),
												partition->ID(),
												shadow->Size(), hasContents));
}

// _GenerateChildPlacementJobs
status_t
KDiskDeviceJobGenerator::_GenerateChildPlacementJobs(KPartition *partition)
{
	KPartition *shadow = partition->ShadowPartition();
	// nothing to do, if the partition contains no partitioning system or
	// shall be re-initialized
	if (!shadow->DiskSystem()
		|| !(shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION)) {
		return B_OK;
	}
	// first resize all children that shall shrink and place their descendants
	int32 childCount = 0;
	int32 moveForth = 0;
	int32 moveBack = 0;
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		if (KPartition *childShadow = child->ShadowPartition()) {
			// add a move_info for the child
			move_info &info = fMoveInfos[childCount];
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
		qsort(fMoveInfos, childCount, sizeof(move_info),
			  _CompareMoveInfoPosition);
	}
	// move the children to their final positions
	while (moveForth + moveBack > 0) {
		int32 moved = 0;
		if (moveForth < moveBack) {
			// move children back
			for (int32 i = 0; i < childCount; i++) {
				move_info &info = fMoveInfos[i];
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
				move_info &info = fMoveInfos[i];
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
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		if (KPartition *childShadow = child->ShadowPartition()) {
			if (childShadow->Size() >= child->Size()) {
				status_t error = _GeneratePlacementJobs(child);
				if (error != B_OK)
					return error;
			}
		}
	}
	return B_ERROR;
}

// _GenerateMoveJob
status_t
KDiskDeviceJobGenerator::_GenerateMoveJob(KPartition *partition)
{
	KPartition *shadow = partition->ShadowPartition();
	if (!shadow)
		return B_BAD_VALUE;
	if (!partition->Parent())
		return B_ERROR;
	// collect all descendants whose contents need to be moved
	fPartitionIDCount = 0;;
	status_t error = _CollectContentsToMove(partition);
	if (error != B_OK)
		return B_OK;
	// add the job
	return _AddJob(fJobFactory->CreateMoveJob(partition->Parent()->ID(),
											  partition->ID(),
											  shadow->Offset(), fPartitionIDs,
											  fPartitionIDCount));
}

// _CollectContentsToMove
status_t
KDiskDeviceJobGenerator::_CollectContentsToMove(KPartition *partition)
{
	KPartition *shadow = partition->ShadowPartition();
	if (!shadow)
		return B_OK;
	if (shadow->Status() == B_PARTITION_UNRECOGNIZED)
		return B_ERROR;
	// if the partition has contents, push its ID
	if (shadow->DiskSystem()
		&& !(shadow->ChangeFlags() & B_PARTITION_CHANGED_INITIALIZATION)) {
		_PushPartitionID(partition->ID());
	}
	// recurse
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		status_t error = _CollectContentsToMove(child);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}

// _PushPartitionID
void
KDiskDeviceJobGenerator::_PushPartitionID(partition_id id)
{
	fPartitionIDs[fPartitionIDCount++] = id;
}

// _CompareMoveInfoOffset
int
KDiskDeviceJobGenerator::_CompareMoveInfoPosition(const void *_a,
												  const void *_b)
{
	const move_info *a = static_cast<const move_info *>(_a);
	const move_info *b = static_cast<const move_info *>(_b);
	if (a->position < b->position)
		return -1;
	if (a->position > b->position)
		return 1;
	return 0;
}
