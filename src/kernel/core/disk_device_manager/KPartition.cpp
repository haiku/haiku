// KPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>
#include <Errors.h>

#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"
#include "KPartition.h"

using namespace std;

// constructor
// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

KPartition::KPartition(partition_id id)
	: fPartitionData(),
	  fChildren(),
	  fDevice(NULL),
	  fParent(NULL),
	  fDiskSystem(NULL),
	  fReferenceCount(0),
	  fObsolete(false)
{
	fPartitionData.id = (id >= 0 ? id : _NextID());
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
	SetDiskSystem(NULL);
	free(fPartitionData.name);
	free(fPartitionData.content_name);
	free(fPartitionData.type);
	free(fPartitionData.content_type);
	free(fPartitionData.parameters);
	free(fPartitionData.content_parameters);
}

// Register
void
KPartition::Register()
{
	fReferenceCount++;
}

// Unregister
void
KPartition::Unregister()
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	ManagerLocker locker(manager);
	fReferenceCount--;
	if (IsObsolete() && fReferenceCount == 0) {
		// let the manager delete object
		manager->DeletePartition(this);
	}
}

// CountReferences
int32
KPartition::CountReferences() const
{
	return fReferenceCount;
}

// MarkObsolete
void
KPartition::MarkObsolete()
{
	fObsolete = true;
}

// IsObsolete
bool
KPartition::IsObsolete() const
{
	return fObsolete;
}

// PrepareForRemoval
bool
KPartition::PrepareForRemoval()
{
	bool result = RemoveAllChildren();
	UnpublishDevice();
	return result;
}

// PrepareForDeletion
bool
KPartition::PrepareForDeletion()
{
	return true;
}

// Open
status_t
KPartition::Open(int flags, int *fd)
{
	if (!fd)
		return B_BAD_VALUE;
	// get the path
	char path[B_PATH_NAME_LENGTH];
	status_t error = GetPath(path);
	if (error != B_OK)
		return error;
	// open the device
	*fd = open(path, flags);
	if (*fd < 0)
		return errno;
	return B_OK;
}

// PublishDevice
status_t
KPartition::PublishDevice()
{
	// prepare a partition_info
	partition_info info;
	info.offset = Offset();
	info.size = Size();
	info.logical_block_size = BlockSize();
	info.session = 0;
	info.partition = ID();
	if (strlen(Device()->Path()) >= 256)
		return B_NAME_TOO_LONG;
	strcpy(info.device, Device()->Path());
	// get the entry path
	char path[B_PATH_NAME_LENGTH];
	status_t error = GetPath(path);
	if (error != B_OK)
		return error;
	// create the entry
	int fd = creat(path, 0666);
	if (fd < 0)
		return errno;
	// set the partition info
	error = B_OK;
	if (ioctl(fd, B_SET_PARTITION, &info) < 0)
		error = errno;
	close(fd);
	return error;
}

// UnpublishDevice
status_t
KPartition::UnpublishDevice()
{
	// get the entry path
	char path[B_PATH_NAME_LENGTH];
	status_t error = GetPath(path);
	if (error != B_OK)
		return error;
	// remove the entry
	if (remove(path) < 0)
		return errno;
	return B_OK;
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
	// not implemented
	return 0;
}

// GetPath
status_t
KPartition::GetPath(char *path) const
{
	// For a KDiskDevice this version is never invoked, so the check for
	// Parent() is correct.
	if (!path || !Parent() || Index() < 0)
		return B_BAD_VALUE;
	// get the parent's path
	status_t error = Parent()->GetPath(path);
	if (error != B_OK)
		return error;
	// check length for safety
	int32 len = strlen(path);
	if (len >= B_PATH_NAME_LENGTH - 10)
		return B_NAME_TOO_LONG;
	if (Parent() == Device()) {
		// Our parent is a device, so we replace `raw' by our index.
		int32 leafLen = strlen("/raw");
		if (len <= leafLen || strcmp(path + len - leafLen, "/raw"))
			return B_ERROR;
// TODO: For the time being the name is "obos_*" to not interfere with R5's
// names.
//		sprintf(path + len - leafLen + 1, "%ld", Index());
		sprintf(path + len - leafLen + 1, "obos_%ld", Index());
	} else {
		// Our parent is a normal partition, no device: Append our index.
		sprintf(path + len, "_%ld", Index());
	}
	return error;
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
	return fPartitionData.volume;
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
	// Must be called in a {Add,Remove}Child() only!
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
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (!fChildren.AddItem(partition, index))
			return B_NO_MEMORY;
		if (!manager->PartitionAdded(partition)) {
			fChildren.RemoveItem(index);
			return B_NO_MEMORY;
		}
		partition->SetIndex(index);
		_UpdateChildIndices(index);
		fPartitionData.child_count++;
		partition->SetParent(this);
		partition->SetDevice(Device());
		return B_OK;
	}
	return B_ERROR;
}

// CreateChild
status_t
KPartition::CreateChild(partition_id id, int32 index, KPartition **_child)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count)
		return B_BAD_VALUE;
	// create and add partition
	KPartition *child = new(nothrow) KPartition(id);
	if (!child)
		return B_NO_MEMORY;
	status_t error = AddChild(child, index);
	// cleanup / set result
	if (error != B_OK)
		delete child;
	else if (_child)
		*_child = child;
	return error;
}

// RemoveChild
bool
KPartition::RemoveChild(int32 index)
{
	if (index < 0 || index >= fPartitionData.child_count)
		return false;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		KPartition *partition = fChildren.ItemAt(index);
		PartitionRegistrar _(partition);
		if (!partition || !manager->PartitionRemoved(partition)
			|| !fChildren.RemoveItem(index)) {
			return false;
		}
		_UpdateChildIndices(index + 1);
		partition->SetIndex(-1);
		fPartitionData.child_count--;
		partition->SetParent(NULL);
		partition->SetDevice(NULL);
		return true;
	}
	return false;
}

// RemoveChild
bool
KPartition::RemoveChild(KPartition *child)
{
	if (child) {
		int32 index = fChildren.IndexOf(child);
		if (index >= 0)
			return RemoveChild(index);
	}
	return false;
}

// RemoveAllChildren
bool
KPartition::RemoveAllChildren()
{
	int32 count = CountChildren();
	for (int32 i = count - 1; i >= 0; i--) {
		if (!RemoveChild(i))
			return false;
	}
	return true;
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
	// unload former disk system
	if (fDiskSystem) {
		fDiskSystem->Unload();
		fDiskSystem = NULL;
	}
	// set and load new one
	fDiskSystem = diskSystem;
	if (fDiskSystem)
		fDiskSystem->Load();	// can't fail, since it's already loaded
}

// DiskSystem
KDiskSystem *
KPartition::DiskSystem() const
{
	return fDiskSystem;
}

// ParentDiskSystem
KDiskSystem *
KPartition::ParentDiskSystem() const
{
	return (Parent() ? Parent()->DiskSystem() : NULL);
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

// Dump
void
KPartition::Dump(bool deep, int32 level)
{
	if (level < 0 || level > 255)
		return;
	char prefix[256];
	sprintf(prefix, "%*s%*s", (int)level, "", (int)level, "");
	char path[B_PATH_NAME_LENGTH];
	GetPath(path);
	if (level > 0)
		OUT("%spartition %ld: %s\n", prefix, ID(), path);
	OUT("%s  offset:            %lld\n", prefix, Offset());
	OUT("%s  size:              %lld\n", prefix, Size());
	OUT("%s  block size:        %lu\n", prefix, BlockSize());
	OUT("%s  child count:       %ld\n", prefix, CountChildren());
	OUT("%s  index:             %ld\n", prefix, Index());
	OUT("%s  status:            %lu\n", prefix, Status());
	OUT("%s  flags:             %lx\n", prefix, Flags());
	OUT("%s  volume:            %ld\n", prefix, VolumeID());
	OUT("%s  disk system:       %s\n", prefix,
		(DiskSystem() ? DiskSystem()->Name() : NULL));
	OUT("%s  name:              %s\n", prefix, Name());
	OUT("%s  content name:      %s\n", prefix, ContentName());
	OUT("%s  type:              %s\n", prefix, Type());
	OUT("%s  content type:      %s\n", prefix, ContentType());
	OUT("%s  params:            %s\n", prefix, Parameters());
	OUT("%s  content params:    %s\n", prefix, ContentParameters());
	if (deep) {
		for (int32 i = 0; KPartition *child = ChildAt(i); i++)
			child->Dump(true, level + 1);
	}
}

// _UpdateChildIndices
void
KPartition::_UpdateChildIndices(int32 index)
{
	for (int32 i = index; KPartition *child = fChildren.ItemAt(i); i++)
		child->SetIndex(index);
}

// _NextID
int32
KPartition::_NextID()
{
	return atomic_add(&fNextID, 1);
}


// fNextID
int32 KPartition::fNextID = 0;

