// KPartition.cpp

#include <new>
#include <stdlib.h>

//#include <Partition.h>
	// TODO: Move the definitions needed in the kernel to a separate header.

#include "KPartition.h"

using namespace std;
using BPrivate::DiskDevice::KDiskDevice;
using BPrivate::DiskDevice::KDiskSystem;

// set_string
// simple helper
static
status_t
set_string(char *&location, const char *newValue)
{
	// unset old value
	if (location) {
		free(location);
		location = NULL;
	}
	// set new value
	status_t error = B_OK;
	if (newValue) {
		location = strdup(newValue);
		if (!location)
			error = B_NO_MEMORY;
	}
	return error;
}

// constructor
KPartition::KPartition(partition_id id)
	: fPartitionData(),
	  fChildren(),
	  fDevice(NULL),
	  fParent(NULL),
	  fDiskSystem(NULL),
	  fParentDiskSystem(NULL),
	  fReferenceCount(0)
{
	fPartitionData.id = id;
	fPartitionData.offset = 0;
	fPartitionData.size = 0;
	fPartitionData.block_size = 0;
	fPartitionData.child_count = 0;
	fPartitionData.index = -1;
	fPartitionData.status = /*B_PARTITION_UNRECOGNIZED*/ 0;
	fPartitionData.flags = 0;
	fPartitionData.volume = -1;
	fPartitionData.name = NULL;
	fPartitionData.content_name = NULL;
	fPartitionData.type = NULL;
	fPartitionData.content_type = NULL;
	fPartitionData.parameters = NULL;
	fPartitionData.content_parameters = NULL;
	fPartitionData.cookie = NULL;
	fPartitionData.content_cookie = NULL;
}

// destructor
KPartition::~KPartition()
{
	free(fPartitionData.name);
	free(fPartitionData.content_name);
	free(fPartitionData.type);
	free(fPartitionData.content_type);
	free(fPartitionData.parameters);
	free(fPartitionData.content_parameters);
}

// Register
bool
KPartition::Register()
{
	fReferenceCount++;
	return true;
}

// Unregister
bool
KPartition::Unregister()
{
	fReferenceCount--;
	return true;
}

// CountReferences
int32
KPartition::CountReferences() const
{
	return fReferenceCount;
}

// SetBusy
void
KPartition::SetBusy(bool busy)
{
	if (busy)
		fPartitionData.flags |= B_PARTITION_BUSY;
	else
		fPartitionData.flags &= ~B_PARTITION_BUSY;
}

// IsBusy
bool
KPartition::IsBusy() const
{
	return (fPartitionData.flags & B_PARTITION_BUSY);
}

// SetDescendantBusy
void
KPartition::SetDescendantBusy(bool busy)
{
	if (busy)
		fPartitionData.flags |= B_PARTITION_DESCENDANT_BUSY;
	else
		fPartitionData.flags &= ~B_PARTITION_DESCENDANT_BUSY;
}

// IsDescendantBusy
bool
KPartition::IsDescendantBusy() const
{
	return (fPartitionData.flags & B_PARTITION_DESCENDANT_BUSY);
}

// SetOffset
void
KPartition::SetOffset(off_t offset)
{
	fPartitionData.offset = offset;
}

// Offset
off_t
KPartition::Offset() const
{
	return fPartitionData.offset;
}

// SetSize
void
KPartition::SetSize(off_t size)
{
	fPartitionData.size = size;
}

// Size
off_t
KPartition::Size() const
{
	return fPartitionData.size;
}

// SetBlockSize
void
KPartition::SetBlockSize(uint32 blockSize)
{
	fPartitionData.block_size = blockSize;
}

// BlockSize
uint32
KPartition::BlockSize() const
{
	return fPartitionData.block_size;
}

// SetIndex
void
KPartition::SetIndex(int32 index)
{
	fPartitionData.index = index;
}

// Index
int32
KPartition::Index() const
{
	return fPartitionData.index;
}

// SetStatus
void
KPartition::SetStatus(uint32 status)
{
	fPartitionData.status = status;
}

// Status
uint32
KPartition::Status() const
{
	return fPartitionData.status;
}

// SetFlags
void
KPartition::SetFlags(uint32 flags)
{
	fPartitionData.flags = flags;
}

// Flags
uint32
KPartition::Flags() const
{
	return fPartitionData.flags;
}

// IsMountable
bool
KPartition::IsMountable() const
{
	return (fPartitionData.flags & B_PARTITION_MOUNTABLE);
}

// IsPartitionable
bool
KPartition::IsPartitionable() const
{
	return (fPartitionData.flags & B_PARTITION_PARTITIONABLE);
}

// IsReadOnly
bool
KPartition::IsReadOnly() const
{
	return (fPartitionData.flags & B_PARTITION_READ_ONLY);
}

// IsMounted
bool
KPartition::IsMounted() const
{
	return (fPartitionData.flags & B_PARTITION_MOUNTED);
}

// IsDevice
bool
KPartition::IsDevice() const
{
	return (fPartitionData.flags & B_PARTITION_IS_DEVICE);
}

// SetName
status_t
KPartition::SetName(const char *name)
{
	return set_string(fPartitionData.name, name);
}

// Name
const char *
KPartition::Name() const
{
	return fPartitionData.name;
}

// SetContentName
status_t
KPartition::SetContentName(const char *name)
{
	return set_string(fPartitionData.content_name, name);
}

// ContentName
const char *
KPartition::ContentName() const
{
	return fPartitionData.content_name;
}

// SetType
status_t
KPartition::SetType(const char *type)
{
	return set_string(fPartitionData.type, type);
}

// Type
const char *
KPartition::Type() const
{
	return fPartitionData.type;
}

// SetContentType
status_t
KPartition::SetContentType(const char *type)
{
	return set_string(fPartitionData.content_type, type);
}

// ContentType
const char *
KPartition::ContentType() const
{
	return fPartitionData.content_type;
}

// PartitionData
partition_data *
KPartition::PartitionData()
{
	return &fPartitionData;
}

// PartitionData
const partition_data *
KPartition::PartitionData() const
{
	return &fPartitionData;
}

// SetID
void
KPartition::SetID(partition_id id)
{
	fPartitionData.id = id;
}

// ID
partition_id
KPartition::ID() const
{
	return fPartitionData.id;
}

// ChangeCounter
int32
KPartition::ChangeCounter() const
{
}

// GetPath
status_t
KPartition::GetPath(char *path) const
{
	// not implemented
	return B_ERROR;
}

// SetVolumeID
void
KPartition::SetVolumeID(dev_t volumeID)
{
	fPartitionData.volume = volumeID;
}

// VolumeID
dev_t
KPartition::VolumeID() const
{
	return fPartitionData.id;
}

// Mount
status_t
KPartition::Mount(uint32 mountFlags, const char *parameters)
{
	// not implemented
	return B_ERROR;
}

// Unmount
status_t
KPartition::Unmount()
{
	// not implemented
	return B_ERROR;
}

// SetParameters
status_t
KPartition::SetParameters(const char *parameters)
{
	return set_string(fPartitionData.parameters, parameters);
}

// Parameters
const char *
KPartition::Parameters() const
{
	return fPartitionData.parameters;
}

// SetContentParameters
status_t
KPartition::SetContentParameters(const char *parameters)
{
	return set_string(fPartitionData.content_parameters, parameters);
}

// ContentParameters
const char *
KPartition::ContentParameters() const
{
	return fPartitionData.content_parameters;
}

// SetDevice
void
KPartition::SetDevice(KDiskDevice *device)
{
	fDevice = device;
}

// Device
KDiskDevice *
KPartition::Device() const
{
	return fDevice;
}

// SetParent
void
KPartition::SetParent(KPartition *parent)
{
	fParent = parent;
}

// Parent
KPartition *
KPartition::Parent() const
{
	return fParent;
}

// AddChild
status_t
KPartition::AddChild(KPartition *partition, int32 index)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count || !partition)
		return B_BAD_VALUE;
	// add partition
// TODO: Lock the disk device manager!
	if (!fChildren.AddItem(partition, index))
		return B_NO_MEMORY;
	fPartitionData.child_count++;
	partition->SetParent(this);
	partition->SetDevice(Device());
	return B_OK;
}

// RemoveChild
KPartition *
KPartition::RemoveChild(int32 index)
{
	KPartition *partition = NULL;
	if (index >= 0 && index < fPartitionData.child_count) {
// TODO: Lock the disk device manager!
		partition = fChildren.ItemAt(index);
		if (partition && fChildren.RemoveItem(index)) {
			fPartitionData.child_count--;
			partition->SetParent(NULL);
			partition->SetDevice(NULL);
		}
	}
	return partition;
}

// ChildAt
KPartition *
KPartition::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}

// CountChildren
int32
KPartition::CountChildren() const
{
	return fPartitionData.child_count;
}

// CreateShadowPartition
KPartition *
KPartition::CreateShadowPartition()
{
	// not implemented
	return NULL;
}

// DeleteShadowPartition
void
KPartition::DeleteShadowPartition()
{
	// not implemented
}

// ShadowPartition
KPartition *
KPartition::ShadowPartition() const
{
	// not implemented
	return NULL;
}

// IsShadowPartition
bool
KPartition::IsShadowPartition() const
{
	// not implemented
	return false;
}

// SetDiskSystem
void
KPartition::SetDiskSystem(KDiskSystem *diskSystem)
{
	fDiskSystem = diskSystem;
}

// DiskSystem
KDiskSystem *
KPartition::DiskSystem() const
{
	return fDiskSystem;
}

// SetParentDiskSystem
void
KPartition::SetParentDiskSystem(KDiskSystem *diskSystem)
{
	fParentDiskSystem = diskSystem;
}

// ParentDiskSystem
KDiskSystem *
KPartition::ParentDiskSystem() const
{
	return fParentDiskSystem;
}

// SetCookie
void
KPartition::SetCookie(void *cookie)
{
	fPartitionData.cookie = cookie;
}

// Cookie
void *
KPartition::Cookie() const
{
	return fPartitionData.cookie;
}

// SetContentCookie
void
KPartition::SetContentCookie(void *cookie)
{
	fPartitionData.content_cookie = cookie;
}

// ContentCookie
void *
KPartition::ContentCookie() const
{
	return fPartitionData.content_cookie;
}

