// KPartition.h

#ifndef _K_DISK_DEVICE_SHADOW_PARTITION_H
#define _K_DISK_DEVICE_SHADOW_PARTITION_H

#include "KPartition.h"

namespace BPrivate {
namespace DiskDevice {

class KPhysicalPartition;

class KShadowPartition : public KPartition {
public:
	KShadowPartition(KPhysicalPartition *physicalPartition);
	virtual ~KShadowPartition();

	// Hierarchy

	virtual status_t CreateChild(partition_id id, int32 index,
								 KPartition **child = NULL);

	// Shadow Partition

	virtual KShadowPartition *ShadowPartition() const;
	virtual bool IsShadowPartition() const;
	void UnsetPhysicalPartition();
	virtual KPhysicalPartition *PhysicalPartition() const;

	void SyncWithPhysicalPartition();

	virtual void Dump(bool deep, int32 level);

protected:
	KPhysicalPartition	*fPhysicalPartition;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KShadowPartition;

#endif	// _K_DISK_DEVICE_SHADOW_PARTITION_H
