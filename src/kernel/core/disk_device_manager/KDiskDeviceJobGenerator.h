// KDiskDeviceJobGenerator.h

#ifndef _K_DISK_DEVICE_JOB_GENERATOR_H
#define _K_DISK_DEVICE_JOB_GENERATOR_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;
class KDiskDeviceJobFactory;
class KDiskDeviceJobQueue;
class KPartition;
class KPhysicalPartition;
class KShadowPartition;

class KDiskDeviceJobGenerator {
public:
	KDiskDeviceJobGenerator(KDiskDeviceJobFactory *jobFactory,
							KDiskDevice *device,
							KDiskDeviceJobQueue *jobQueue = NULL);
	~KDiskDeviceJobGenerator();

	KDiskDeviceJobFactory *JobFactory() const;
	KDiskDevice *Device() const;
	KDiskDeviceJobQueue *JobQueue() const;

	KDiskDeviceJobQueue *DetachJobQueue();

	status_t GenerateJobs();

private:
	struct move_info;

	status_t _AddJob(KDiskDeviceJob *job);

	status_t _GenerateCleanupJobs(KPartition *partition);
	status_t _GeneratePlacementJobs(KPartition *partition);
	status_t _GenerateResizeJob(KPartition *partition);
	status_t _GenerateChildPlacementJobs(KPartition *partition);
	status_t _GenerateMoveJob(KPartition *partition);
	status_t _CollectContentsToMove(KPartition *partition);
	status_t _GenerateRemainingJobs(KShadowPartition *parent,
									KShadowPartition *partition);

	void _PushPartitionID(partition_id id);
	static int _CompareMoveInfoPosition(const void *_a, const void *_b);

	KDiskDeviceJobFactory	*fJobFactory;
	KDiskDevice				*fDevice;
	KDiskDeviceJobQueue		*fJobQueue;
	move_info				*fMoveInfos;
	int32					fMoveInfoCount;
	partition_id			*fPartitionIDs;
	int32					fPartitionIDCount;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJobGenerator;

#endif	// _K_DISK_DEVICE_JOB_GENERATOR_H
