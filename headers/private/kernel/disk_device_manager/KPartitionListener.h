// KPartitionListener.h

#ifndef _K_DISK_DEVICE_PARTITION_LISTENER_H
#define _K_DISK_DEVICE_PARTITION_LISTENER_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskSystem;
class KPartition;

class KPartitionListener {
public:
	KPartitionListener();
	virtual ~KPartitionListener();

	virtual void OffsetChanged(KPartition *partition, off_t offset);
	virtual void SizeChanged(KPartition *partition, off_t size);
	virtual void ContentSizeChanged(KPartition *partition, off_t size);
	virtual void BlockSizeChanged(KPartition *partition, uint32 blockSize);
	virtual void IndexChanged(KPartition *partition, int32 index);
	virtual void StatusChanged(KPartition *partition, uint32 status);
	virtual void FlagsChanged(KPartition *partition, uint32 flags);
	virtual void NameChanged(KPartition *partition, const char *name);
	virtual void ContentNameChanged(KPartition *partition, const char *name);
	virtual void TypeChanged(KPartition *partition, const char *type);
	virtual void IDChanged(KPartition *partition, partition_id id);
	virtual void VolumeIDChanged(KPartition *partition, dev_t volumeID);
	virtual void MountCookieChanged(KPartition *partition, void *cookie);
	virtual void ParametersChanged(KPartition *partition,
								   const char *parameters);
	virtual void ContentParametersChanged(KPartition *partition,
										  const char *parameters);
	virtual void ChildAdded(KPartition *partition, KPartition *child,
							int32 index);
	virtual void ChildRemoved(KPartition *partition, KPartition *child,
							  int32 index);
	virtual void DiskSystemChanged(KPartition *partition,
								   KDiskSystem *diskSystem);
	virtual void CookieChanged(KPartition *partition, void *cookie);
	virtual void ContentCookieChanged(KPartition *partition, void *cookie);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartitionListener;

#endif	// _K_DISK_DEVICE_PARTITION_LISTENER_H
