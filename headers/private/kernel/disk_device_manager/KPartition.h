// KPartition.h

#ifndef _K_DISK_DEVICE_PARTITION_H
#define _K_DISK_DEVICE_PARTITION_H

#include <Vector.h>

#include "disk_device_manager.h"

struct user_partition_data;

namespace BPrivate {
namespace DiskDevice {

class UserDataWriter;

class KDiskDevice;
class KDiskSystem;
class KPartitionVisitor;
class KPhysicalPartition;
class KShadowPartition;

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

	virtual status_t Open(int flags, int *fd);
	virtual status_t PublishDevice();
	virtual status_t UnpublishDevice();

	void SetBusy(bool busy);
	bool IsBusy() const;
		// == jobs which may affect this partition are scheduled/in progress
	void SetDescendantBusy(bool busy);
	bool IsDescendantBusy() const;
		// == jobs which may affect a descendant of this partition are
		// scheduled/in progress; IsBusy() => IsDescendantBusy()
		// In the userland API, both can probably be mapped to one flag.

	void SetOffset(off_t offset);
	off_t Offset() const;		// 0 for devices

	void SetSize(off_t size);
	off_t Size() const;

	void SetContentSize(off_t size);
	off_t ContentSize() const;

	void SetBlockSize(uint32 blockSize);
	uint32 BlockSize() const;

	void SetIndex(int32 index);
	int32 Index() const;		// 0 for devices

	void SetStatus(uint32 status);
	uint32 Status() const;
	
	void SetFlags(uint32 flags);	// comprises the ones below
	void AddFlags(uint32 flags);
	void ClearFlags(uint32 flags);
	uint32 Flags() const;
	bool ContainsFileSystem() const;
	bool ContainsPartitioningSystem() const;
	bool IsReadOnly() const;
	bool IsMounted() const;

	bool IsDevice() const;

	status_t SetName(const char *name);
	const char *Name() const;

	status_t SetContentName(const char *name);
	const char *ContentName() const;

	status_t SetType(const char *type);
	const char *Type() const;

	const char *ContentType() const;
		// ContentType() == DiskSystem()->NamePretty()

	// access to C style partition data
	partition_data *PartitionData();
	const partition_data *PartitionData() const;

	virtual void SetID(partition_id id);
	partition_id ID() const;

	virtual status_t GetPath(char *path) const;
		// no setter (see BDiskDevice) -- built on the fly

	void SetVolumeID(dev_t volumeID);
	dev_t VolumeID() const;

	void SetMountCookie(void *cookie);
	void *MountCookie() const;

	virtual status_t Mount(uint32 mountFlags, const char *parameters);
	virtual status_t Unmount();

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
	virtual status_t CreateChild(partition_id id, int32 index,
								 KPartition **child = NULL) = 0;
	bool RemoveChild(int32 index);
	bool RemoveChild(KPartition *child);
	bool RemoveAllChildren();
	KPartition *ChildAt(int32 index) const;
	int32 CountChildren() const;
	int32 CountDescendants() const;

	KPartition *VisitEachDescendant(KPartitionVisitor *visitor);

	// Shadow Partition

	virtual status_t CreateShadowPartition();	// creates a complete tree
	virtual void UnsetShadowPartition(bool doDelete);
	virtual KShadowPartition *ShadowPartition() const = 0;
	virtual bool IsShadowPartition() const = 0;
	virtual KPhysicalPartition *PhysicalPartition() const = 0;

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

	// Change Tracking

	void Changed(uint32 flags, uint32 clearFlags = 0);
	void SetChangeFlags(uint32 flags);
	uint32 ChangeFlags() const;
	int32 ChangeCounter() const;
	void UninitializeContents(bool logChanges = true);

	void SetAlgorithmData(uint32 data);
	uint32 AlgorithmData() const;
		// temporary storage freely usable by algorithms

	virtual void WriteUserData(UserDataWriter &writer,
							   user_partition_data *data);

	virtual void Dump(bool deep, int32 level);

private:
	void _UpdateChildIndices(int32 index);
	static int32 _NextID();

protected:
	typedef Vector<KPartition*> PartitionVector;

	partition_data		fPartitionData;
	PartitionVector		fChildren;
	KDiskDevice			*fDevice;
	KPartition			*fParent;
	KDiskSystem			*fDiskSystem;
	uint32				fChangeFlags;
	int32				fChangeCounter;
	uint32				fAlgorithmData;
	int32				fReferenceCount;
	bool				fObsolete;
	static int32		fNextID;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartition;

#endif	// _K_DISK_DEVICE_PARTITION_H
