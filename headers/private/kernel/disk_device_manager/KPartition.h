// KPartition.h

#ifndef _K_DISK_DEVICE_PARTITION_H
#define _K_DISK_DEVICE_PARTITION_H

#include "disk_device_manager.h"
#include "List.h"

// partition flags
// TODO: move to another header (must be accessible from userland API impl.)
enum {
	B_PARTITION_IS_DEVICE		= 0x01,
	B_PARTITION_MOUNTABLE		= 0x02,
	B_PARTITION_PARTITIONABLE	= 0x04,
	B_PARTITION_READ_ONLY		= 0x08,
	B_PARTITION_MOUNTED			= 0x10,	// needed?
	B_PARTITION_BUSY			= 0x20,
	B_PARTITION_DESCENDANT_BUSY	= 0x40,
};

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskSystem;

class KPartition {
public:
	KPartition(partition_id id = -1);
	virtual ~KPartition();

	// Reference counting. As long as there's at least one referrer, the
	// object won't be deleted.
	// manager must be locked (Unregister() locks itself)
	void Register();
	void Unregister();
	int32 CountReferences() const;

	void MarkObsolete();
		// called by the manager only
	bool IsObsolete() const;

	virtual bool PrepareForRemoval();
	virtual bool PrepareForDeletion();

	status_t Open(int flags, int *fd);
	virtual status_t PublishDevice();
	virtual status_t UnpublishDevice();

	void SetBusy(bool busy);
	bool IsBusy() const;
		// == jobs which may affect this partition are scheduled/in progress
	void SetDescendantBusy(bool busy);
	bool IsDescendantBusy() const;
		// == jobs which may affect a descendant of this partition are
		// scheduled/in progress; IsBusy() => IsDescendantBusy()
		// In the userland API, both can probably mapped to one flag.

	void SetOffset(off_t offset);
	off_t Offset() const;		// 0 for devices

	void SetSize(off_t size);
	off_t Size() const;

	void SetBlockSize(uint32 blockSize);
	uint32 BlockSize() const;

	void SetIndex(int32 index);
	int32 Index() const;		// 0 for devices

	void SetStatus(uint32 status);
	uint32 Status() const;
	
	void SetFlags(uint32 flags);	// comprises the ones below
	uint32 Flags() const;
	bool IsMountable() const;
	bool IsPartitionable() const;
	bool IsReadOnly() const;
	bool IsMounted() const;

	bool IsDevice() const;

	status_t SetName(const char *name);
	const char *Name() const;

	status_t SetContentName(const char *name);
	const char *ContentName() const;

	status_t SetType(const char *type);
	const char *Type() const;

	status_t SetContentType(const char *type);
	const char *ContentType() const;

	// access to C style partition data
	partition_data *PartitionData();
	const partition_data *PartitionData() const;

	virtual void SetID(partition_id id);
	partition_id ID() const;

	int32 ChangeCounter() const;
	
	virtual status_t GetPath(char *path) const;
		// no setter (see BDiskDevice) -- built on the fly

	void SetVolumeID(dev_t volumeID);
	dev_t VolumeID() const;

	status_t Mount(uint32 mountFlags, const char *parameters);
	status_t Unmount();

	// Parameters

	status_t SetParameters(const char *parameters);
	const char *Parameters() const;

	status_t SetContentParameters(const char *parameters);
	const char *ContentParameters() const;

	// Hierarchy

	void SetDevice(KDiskDevice *device);
	KDiskDevice *Device() const;

	void SetParent(KPartition *parent);
	KPartition *Parent() const;

	status_t AddChild(KPartition *partition, int32 index = -1);
	status_t CreateChild(partition_id id, int32 index,
						 KPartition **child = NULL);
	bool RemoveChild(int32 index);
	bool RemoveChild(KPartition *child);
	bool RemoveAllChildren();
	KPartition *ChildAt(int32 index) const;
	int32 CountChildren() const;

	// Shadow Partition

	virtual KPartition *CreateShadowPartition();	// creates a complete tree
	void DeleteShadowPartition();					// deletes ...
	KPartition *ShadowPartition() const;
	bool IsShadowPartition() const;

	// DiskSystem

	void SetDiskSystem(KDiskSystem *diskSystem);
	KDiskSystem *DiskSystem() const;
	KDiskSystem *ParentDiskSystem() const;
		// When setting a disk system, it must already be loaded.
		// The partition will load it too, hence it won't be unloaded before
		// it is unset here.

	void SetCookie(void *cookie);
	void *Cookie() const;

	void SetContentCookie(void *cookie);
	void *ContentCookie() const;

	virtual void Dump(bool deep, int32 level);

private:
	void _UpdateChildIndices(int32 index);
	static int32 _NextID();

protected:
	partition_data		fPartitionData;
	List<KPartition*>	fChildren;
	KDiskDevice			*fDevice;
	KPartition			*fParent;
	KDiskSystem			*fDiskSystem;
	int32				fReferenceCount;
	bool				fObsolete;
	static int32		fNextID;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartition;

#endif	// _K_DISK_DEVICE_PARTITION_H
