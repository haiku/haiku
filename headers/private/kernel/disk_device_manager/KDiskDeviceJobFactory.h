// KDiskDeviceJobFactory.h

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

	KDiskDeviceJob *CreateDefragmentJob(partition_id partitionID);
	KDiskDeviceJob *CreateRepairJob(partition_id partitionID, bool checkOnly);
	KDiskDeviceJob *CreateResizeJob(partition_id parentID,
									partition_id partitionID, off_t size);
	KDiskDeviceJob *CreateMoveJob(partition_id parentID,
								  partition_id partitionID, off_t offset,
								  const partition_id *contentsToMove,
								  int32 contentsToMoveCount);
	KDiskDeviceJob *CreateSetNameJob(partition_id parentID,
									 partition_id partitionID,
									 const char *name);
	KDiskDeviceJob *CreateSetContentNameJob(partition_id partitionID,
											const char *name);
	KDiskDeviceJob *CreateSetTypeJob(partition_id parentID,
									 partition_id partitionID,
									 const char *type);
	KDiskDeviceJob *CreateSetParametersJob(partition_id parentID,
										   partition_id partitionID,
										   const char *parameters);
	KDiskDeviceJob *CreateSetContentParametersJob(partition_id partitionID,
												  const char *parameters);
	KDiskDeviceJob *CreateInitializeJob(partition_id partitionID,
										disk_system_id diskSystemID,
										const char *name,
										const char *parameters);
	KDiskDeviceJob *CreateUninitializeJob(partition_id partitionID);
	KDiskDeviceJob *CreateCreateChildJob(partition_id partitionID,
										 partition_id childID, off_t offset,
										 off_t size, const char *type,
										 const char *parameters);
	KDiskDeviceJob *CreateDeleteChildJob(partition_id parentID,
										 partition_id partitionID);

	KDiskDeviceJob *CreateScanPartitionJob(partition_id partitionID);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJobFactory;

#endif	// _K_DISK_DEVICE_JOB_FACTORY_H
