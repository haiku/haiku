/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_DISK_DEVICE_PARTITION_H
#define _K_DISK_DEVICE_PARTITION_H

#include <disk_device_manager.h>
#include <Vector.h>

struct user_partition_data;

namespace BPrivate {
namespace DiskDevice {

class UserDataWriter;

class KDiskDevice;
class KDiskSystem;
class KPartitionListener;
class KPartitionVisitor;
class KPath;
class KPhysicalPartition;

//!	\brief Class representing a single partition.
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
	virtual status_t RepublishDevice();
	bool IsPublished() const;

	void SetBusy(bool busy);
	bool IsBusy() const;
	bool IsBusy(bool includeDescendants);
	bool CheckAndMarkBusy(bool includeDescendants);
	void MarkBusy(bool includeDescendants);
	void UnmarkBusy(bool includeDescendants);

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
	bool IsUninitialized() const;

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

	virtual	status_t GetFileName(char* buffer, size_t size) const;
	virtual status_t GetPath(KPath *path) const;
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
	status_t CreateChild(partition_id id, int32 index, off_t offset, off_t size,
		KPartition **child = NULL);
	bool RemoveChild(int32 index);
	bool RemoveChild(KPartition *child);
	bool RemoveAllChildren();
	KPartition *ChildAt(int32 index) const;
	int32 CountChildren() const;
	int32 CountDescendants() const;

	KPartition *VisitEachDescendant(KPartitionVisitor *visitor);

	// DiskSystem

	void SetDiskSystem(KDiskSystem *diskSystem, float priority = 0.0f);
	KDiskSystem *DiskSystem() const;
	float DiskSystemPriority() const;
	KDiskSystem *ParentDiskSystem() const;
		// When setting a disk system, it must already be loaded.
		// The partition will load it too, hence it won't be unloaded before
		// it is unset here.

	void SetCookie(void *cookie);
	void *Cookie() const;

	void SetContentCookie(void *cookie);
	void *ContentCookie() const;

	// Listener Support

	bool AddListener(KPartitionListener *listener);
	bool RemoveListener(KPartitionListener *listener);

	// Change Tracking

	void Changed(uint32 flags, uint32 clearFlags = 0);
	void SetChangeFlags(uint32 flags);
	uint32 ChangeFlags() const;
	int32 ChangeCounter() const;
	status_t UninitializeContents(bool logChanges = true);

	void SetAlgorithmData(uint32 data);
	uint32 AlgorithmData() const;
		// temporary storage freely usable by algorithms

	virtual void WriteUserData(UserDataWriter &writer,
							   user_partition_data *data);

	virtual void Dump(bool deep, int32 level);

protected:
	void FireOffsetChanged(off_t offset);
	void FireSizeChanged(off_t size);
	void FireContentSizeChanged(off_t size);
	void FireBlockSizeChanged(uint32 blockSize);
	void FireIndexChanged(int32 index);
	void FireStatusChanged(uint32 status);
	void FireFlagsChanged(uint32 flags);
	void FireNameChanged(const char *name);
	void FireContentNameChanged(const char *name);
	void FireTypeChanged(const char *type);
	void FireIDChanged(partition_id id);
	void FireVolumeIDChanged(dev_t volumeID);
	void FireMountCookieChanged(void *cookie);
	void FireParametersChanged(const char *parameters);
	void FireContentParametersChanged(const char *parameters);
	void FireChildAdded(KPartition *child, int32 index);
	void FireChildRemoved(KPartition *child, int32 index);
	void FireDiskSystemChanged(KDiskSystem *diskSystem);
	void FireCookieChanged(void *cookie);
	void FireContentCookieChanged(void *cookie);

private:
	void _UpdateChildIndices(int32 start, int32 end);
	static int32 _NextID();

protected:
	typedef Vector<KPartition*> PartitionVector;
	struct ListenerSet;

	partition_data		fPartitionData;
	PartitionVector		fChildren;
	KDiskDevice			*fDevice;
	KPartition			*fParent;
	KDiskSystem			*fDiskSystem;
	float				fDiskSystemPriority;
	ListenerSet			*fListeners;
	uint32				fChangeFlags;
	int32				fChangeCounter;
	uint32				fAlgorithmData;
	int32				fReferenceCount;
	bool				fObsolete;
	char				*fPublishedName;
	static int32		sNextID;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartition;

#endif	// _K_DISK_DEVICE_PARTITION_H
