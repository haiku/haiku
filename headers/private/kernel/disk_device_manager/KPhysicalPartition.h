// KPartition.h

#ifndef _K_DISK_DEVICE_PHYSICAL_PARTITION_H
#define _K_DISK_DEVICE_PHYSICAL_PARTITION_H

#include <KPartition.h>

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskSystem;
class KShadowPartition;

/// \brief Class representing an existing partition.
class KPhysicalPartition : public KPartition {
public:
	KPhysicalPartition(partition_id id = -1);
	virtual ~KPhysicalPartition();

	virtual bool PrepareForRemoval();

	virtual status_t Open(int flags, int *fd);
	virtual status_t PublishDevice();
	virtual status_t UnpublishDevice();

	virtual status_t Mount(uint32 mountFlags, const char *parameters);
	virtual status_t Unmount();

	// Hierarchy

	virtual status_t CreateChild(partition_id id, int32 index,
								 KPartition **child = NULL);

	// Shadow Partition

	virtual status_t CreateShadowPartition();	// creates a complete tree
	virtual void UnsetShadowPartition(bool doDelete);
	virtual KShadowPartition *ShadowPartition() const;
	virtual bool IsShadowPartition() const;
	virtual KPhysicalPartition *PhysicalPartition() const;

	// DiskSystem

	virtual void Dump(bool deep, int32 level);

protected:
	KShadowPartition	*fShadowPartition;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPhysicalPartition;

#endif	// _K_DISK_DEVICE_PHYSICAL_PARTITION_H
