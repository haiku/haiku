// KDiskDeviceJob.h

#ifndef _K_DISK_DEVICE_JOB_FACTORY_H
#define _K_DISK_DEVICE_JOB_FACTORY_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDeviceJob;

class KDiskDeviceJobFactory {
public:
	KDiskDeviceJobFactory();
	~KDiskDeviceJobFactory();

	KDiskDeviceJob *CreateCreateChildJob(partition_id partition,
										 partition_id child, off_t offset,
										 off_t size, const char *parameters);
	KDiskDeviceJob *CreateDefragmentJob(partition_id partition);
	KDiskDeviceJob *CreateDeleteChildJob(partition_id parent,
										 partition_id partition);
	KDiskDeviceJob *CreateScanPartitionJob(partition_id partition);
	KDiskDeviceJob *CreateInitializeJob(partition_id partition,
										disk_system_id diskSystemID,
										const char *parameters);
	KDiskDeviceJob *CreateMoveJob(partition_id parent, partition_id partition,
								  off_t offset);
	KDiskDeviceJob *CreateRepairJob(partition_id partition);
	KDiskDeviceJob *CreateResizeJob(partition_id parent,
									partition_id partition, off_t size);
	KDiskDeviceJob *CreateSetParametersJob(partition_id parent,
										   partition_id partition,
										   const char *parameters,
										   const char *contentParameters);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJobQueue;

#endif	// _K_DISK_DEVICE_JOB_FACTORY_H
