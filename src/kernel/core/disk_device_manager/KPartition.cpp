// KPartition.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <util/kernel_cpp.h>

#include <ddm_userland_interface.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartition.h>
#include <KPartitionListener.h>
#include <KPartitionVisitor.h>
#include <KPath.h>
#include <VectorSet.h>

#include "UserDataWriter.h"

using namespace std;

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// ListenerSet
struct KPartition::ListenerSet : VectorSet<KPartitionListener*> {};

// constructor
KPartition::KPartition(partition_id id)
	: fPartitionData(),
	  fChildren(),
	  fDevice(NULL),
	  fParent(NULL),
	  fDiskSystem(NULL),
	  fListeners(NULL),
	  fChangeFlags(0),
	  fChangeCounter(0),
	  fAlgorithmData(0),
	  fReferenceCount(0),
	  fObsolete(false)
{
	fPartitionData.id = (id >= 0 ? id : _NextID());
	fPartitionData.offset = 0;
	fPartitionData.size = 0;
	fPartitionData.content_size = 0;
	fPartitionData.block_size = 0;
	fPartitionData.child_count = 0;
	fPartitionData.index = -1;
	fPartitionData.status = B_PARTITION_UNRECOGNIZED;
	fPartitionData.flags = B_PARTITION_BUSY | B_PARTITION_DESCENDANT_BUSY;
	fPartitionData.volume = -1;
	fPartitionData.mount_cookie = NULL;
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
	delete fListeners;
	SetDiskSystem(NULL);
	free(fPartitionData.name);
	free(fPartitionData.content_name);
	free(fPartitionData.type);
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
	if (ParentDiskSystem())
		ParentDiskSystem()->FreeCookie(this);
	if (DiskSystem())
		DiskSystem()->FreeContentCookie(this);
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
	return B_BAD_VALUE;
}

// PublishDevice
status_t
KPartition::PublishDevice()
{
	// we're just a stupid base class, what do we know?
	return B_ERROR;
}

// UnpublishDevice
status_t
KPartition::UnpublishDevice()
{
	return B_ERROR;
}

// SetBusy
void
KPartition::SetBusy(bool busy)
{
	if (busy)
		SetFlags(B_PARTITION_BUSY);
	else
		ClearFlags(B_PARTITION_BUSY);
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
		SetFlags(B_PARTITION_DESCENDANT_BUSY);
	else
		ClearFlags(B_PARTITION_DESCENDANT_BUSY);
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
	if (fPartitionData.offset != offset) {
		fPartitionData.offset = offset;
		FireOffsetChanged(offset);
	}
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
	if (fPartitionData.size != size) {
		fPartitionData.size = size;
		FireSizeChanged(size);
	}
}

// Size
off_t
KPartition::Size() const
{
	return fPartitionData.size;
}

// SetContentSize
void
KPartition::SetContentSize(off_t size)
{
	if (fPartitionData.content_size != size) {
		fPartitionData.content_size = size;
		FireContentSizeChanged(size);
	}
}

// ContentSize
off_t
KPartition::ContentSize() const
{
	return fPartitionData.content_size;
}

// SetBlockSize
void
KPartition::SetBlockSize(uint32 blockSize)
{
	if (fPartitionData.block_size != blockSize) {
		fPartitionData.block_size = blockSize;
		FireBlockSizeChanged(blockSize);
	}
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
	if (fPartitionData.index != index) {
		fPartitionData.index = index;
		FireIndexChanged(index);
	}
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
	if (fPartitionData.status != status) {
		fPartitionData.status = status;
		FireStatusChanged(status);
	}
}

// Status
uint32
KPartition::Status() const
{
	return fPartitionData.status;
}

// IsUninitialized
bool
KPartition::IsUninitialized() const
{
	return (Status() == B_PARTITION_UNINITIALIZED);
}

// SetFlags
void
KPartition::SetFlags(uint32 flags)
{
	if (fPartitionData.flags != flags) {
		fPartitionData.flags = flags;
		FireFlagsChanged(flags);
	}
}

// AddFlags
void
KPartition::AddFlags(uint32 flags)
{
	if (~fPartitionData.flags & flags) {
		fPartitionData.flags |= flags;
		FireFlagsChanged(fPartitionData.flags);
	}
}

// ClearFlags
void
KPartition::ClearFlags(uint32 flags)
{
	if (fPartitionData.flags & flags) {
		fPartitionData.flags &= ~flags;
		FireFlagsChanged(fPartitionData.flags);
	}
}

// Flags
uint32
KPartition::Flags() const
{
	return fPartitionData.flags;
}

// ContainsFileSystem
bool
KPartition::ContainsFileSystem() const
{
	return (fPartitionData.flags & B_PARTITION_FILE_SYSTEM);
}

// ContainsPartitioningSystem
bool
KPartition::ContainsPartitioningSystem() const
{
	return (fPartitionData.flags & B_PARTITION_PARTITIONING_SYSTEM);
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
	status_t error = set_string(fPartitionData.name, name);
	FireNameChanged(fPartitionData.name);
	return error;
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
	status_t error = set_string(fPartitionData.content_name, name);
	FireContentNameChanged(fPartitionData.content_name);
	return error;
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
	status_t error = set_string(fPartitionData.type, type);
	FireTypeChanged(fPartitionData.type);
	return error;
}

// Type
const char *
KPartition::Type() const
{
	return fPartitionData.type;
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
	if (fPartitionData.id != id) {
		fPartitionData.id = id;
		FireIDChanged(id);
	}
}

// ID
partition_id
KPartition::ID() const
{
	return fPartitionData.id;
}

// GetPath
status_t
KPartition::GetPath(KPath *path) const
{
	// For a KDiskDevice this version is never invoked, so the check for
	// Parent() is correct.
	if (!path || path->InitCheck() != B_OK || !Parent() || Index() < 0)
		return B_BAD_VALUE;
	// get the parent's path
	status_t error = Parent()->GetPath(path);
	if (error != B_OK)
		return error;
	if (Parent()->IsDevice()) {
		// Our parent is a device, so we replace `raw' by our index.
		const char *leaf = path->Leaf();
		if (!leaf || strcmp(leaf, "raw") != B_OK)
			return B_ERROR;
		#ifdef _KERNEL_MODE
			char indexBuffer[12];
			snprintf(indexBuffer, sizeof(indexBuffer), "%ld", Index());
		#else
			const char *prefix = "haiku_";
			char indexBuffer[strlen(prefix) + 12];
			snprintf(indexBuffer, sizeof(indexBuffer), "%s%ld", prefix,
				Index());
		#endif
		error = path->ReplaceLeaf(indexBuffer);
	} else {
		// Our parent is a normal partition, no device: Append our index.
		char indexBuffer[13];
		snprintf(indexBuffer, sizeof(indexBuffer), "_%ld", Index());
		error = path->Append(indexBuffer, false);
	}
	return error;
}

// SetVolumeID
void
KPartition::SetVolumeID(dev_t volumeID)
{
	if (fPartitionData.volume != volumeID) {
		fPartitionData.volume = volumeID;
		FireVolumeIDChanged(volumeID);
		if (VolumeID() >= 0)
			AddFlags(B_PARTITION_MOUNTED);
		else
			ClearFlags(B_PARTITION_MOUNTED);
	}
}

// VolumeID
dev_t
KPartition::VolumeID() const
{
	return fPartitionData.volume;
}

// SetMountCookie
void
KPartition::SetMountCookie(void *cookie)
{
	if (fPartitionData.mount_cookie != cookie) {
		fPartitionData.mount_cookie = cookie;
		FireMountCookieChanged(cookie);
	}
}

// MountCookie
void *
KPartition::MountCookie() const
{
	return fPartitionData.mount_cookie;
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
	status_t error = set_string(fPartitionData.parameters, parameters);
	FireParametersChanged(fPartitionData.parameters);
	return error;
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
	status_t error = set_string(fPartitionData.content_parameters, parameters);
	FireContentParametersChanged(fPartitionData.content_parameters);
	return error;
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
	if (fDevice && fDevice->IsReadOnlyMedia())
		AddFlags(B_PARTITION_READ_ONLY);
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
		status_t error = fChildren.Insert(partition, index);
		if (error != B_OK)
			return error;
		if (!manager->PartitionAdded(partition)) {
			fChildren.Erase(index);
			return B_NO_MEMORY;
		}
		partition->SetIndex(index);
		_UpdateChildIndices(index);
		fPartitionData.child_count++;
		partition->SetParent(this);
		partition->SetDevice(Device());
		// notify listeners
		FireChildAdded(partition, index);
		return B_OK;
	}
	return B_ERROR;
}

// RemoveChild
bool
KPartition::RemoveChild(int32 index)
{
	if (index < 0 || index >= fPartitionData.child_count)
		return false;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		KPartition *partition = fChildren.ElementAt(index);
		PartitionRegistrar _(partition);
		if (!partition || !manager->PartitionRemoved(partition)
			|| !fChildren.Erase(index)) {
			return false;
		}
		_UpdateChildIndices(index + 1);
		partition->SetIndex(-1);
		fPartitionData.child_count--;
		partition->SetParent(NULL);
		partition->SetDevice(NULL);
		// notify listeners
		FireChildAdded(partition, index);
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
	return (index >= 0 && index < fChildren.Count()
			? fChildren.ElementAt(index) : NULL);
}

// CountChildren
int32
KPartition::CountChildren() const
{
	return fPartitionData.child_count;
}

// CountDescendants
int32
KPartition::CountDescendants() const
{
	int32 count = 1;
	for (int32 i = 0; KPartition *child = ChildAt(i); i++)
		count += child->CountDescendants();
	return count;
}

// VisitEachDescendant
KPartition *
KPartition::VisitEachDescendant(KPartitionVisitor *visitor)
{
	if (!visitor)
		return NULL;
	if (visitor->VisitPre(this))
		return this;
	for (int32 i = 0; KPartition *child = ChildAt(i); i++) {
		if (KPartition *result = child->VisitEachDescendant(visitor))
			return result;
	}
	if (visitor->VisitPost(this))
		return this;
	return NULL;	
}

// CreateShadowPartition
status_t
KPartition::CreateShadowPartition()
{
	// implemented by derived classes
	return B_ERROR;
}

// UnsetShadowPartition
void
KPartition::UnsetShadowPartition(bool doDelete)
{
	// implemented by derived classes
}

// SetDiskSystem
void
KPartition::SetDiskSystem(KDiskSystem *diskSystem)
{
	// unload former disk system
	if (fDiskSystem) {
		fPartitionData.content_type = NULL;
		fDiskSystem->Unload();
		fDiskSystem = NULL;
	}
	// set and load new one
	fDiskSystem = diskSystem;
	if (fDiskSystem)
		fDiskSystem->Load();	// can't fail, since it's already loaded
	// update concerned partition flags
	if (fDiskSystem) {
		fPartitionData.content_type = fDiskSystem->PrettyName();
		if (fDiskSystem->IsFileSystem())
			AddFlags(B_PARTITION_FILE_SYSTEM);
		else
			AddFlags(B_PARTITION_PARTITIONING_SYSTEM);
	}
	// notify listeners
	FireDiskSystemChanged(fDiskSystem);
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
	if (fPartitionData.cookie != cookie) {
		fPartitionData.cookie = cookie;
		FireCookieChanged(cookie);
	}
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
	if (fPartitionData.content_cookie != cookie) {
		fPartitionData.content_cookie = cookie;
		FireContentCookieChanged(cookie);
	}
}

// ContentCookie
void *
KPartition::ContentCookie() const
{
	return fPartitionData.content_cookie;
}

// AddListener
bool
KPartition::AddListener(KPartitionListener *listener)
{
	if (!listener)
		return false;
	// lazy create listeners
	if (!fListeners) {
		fListeners = new(nothrow) ListenerSet;
		if (!fListeners)
			return false;
	}
	// add listener
	return (fListeners->Insert(listener) == B_OK);
}

// RemoveListener
bool
KPartition::RemoveListener(KPartitionListener *listener)
{
	if (!listener || !fListeners)
		return false;
	// remove listener and delete the set, if empty now
	bool result = (fListeners->Remove(listener) > 0);
	if (fListeners->IsEmpty()) {
		delete fListeners;
		fListeners = NULL;
	}
	return result;
}

// Changed
void
KPartition::Changed(uint32 flags, uint32 clearFlags)
{
	fChangeFlags &= ~clearFlags;
	fChangeFlags |= flags;
	fChangeCounter++;
	if (Parent())
		Parent()->Changed(B_PARTITION_CHANGED_DESCENDANTS);
}

// SetChangeFlags
void
KPartition::SetChangeFlags(uint32 flags)
{
	fChangeFlags = flags;
}

// ChangeFlags
uint32
KPartition::ChangeFlags() const
{
	return fChangeFlags;
}

// ChangeCounter
int32
KPartition::ChangeCounter() const
{
	return fChangeCounter;
}

// UninitializeContents
status_t
KPartition::UninitializeContents(bool logChanges)
{
	if (DiskSystem()) {
		uint32 flags = B_PARTITION_CHANGED_INITIALIZATION
					   | B_PARTITION_CHANGED_CONTENT_TYPE
					   | B_PARTITION_CHANGED_STATUS
					   | B_PARTITION_CHANGED_FLAGS;
		// children
		if (CountChildren() > 0) {
			if (!RemoveAllChildren())
				return B_ERROR;
			flags |= B_PARTITION_CHANGED_CHILDREN;
		}
		// volume
		if (VolumeID() >= 0) {
			SetVolumeID(-1);
			flags |= B_PARTITION_CHANGED_VOLUME;
		}
		// content name
		if (ContentName()) {
			SetContentName(NULL);
			flags |= B_PARTITION_CHANGED_CONTENT_NAME;
		}
		// content parameters
		if (ContentParameters()) {
			SetContentParameters(NULL);
			flags |= B_PARTITION_CHANGED_CONTENT_PARAMETERS;
		}
		// content size
		if (ContentSize() > 0) {
			SetContentSize(0);
			flags |= B_PARTITION_CHANGED_CONTENT_SIZE;
		}
		// block size
		if (Parent() && Parent()->BlockSize() != BlockSize()) {
			SetBlockSize(Parent()->BlockSize());
			flags |= B_PARTITION_CHANGED_BLOCK_SIZE;
		}
		// disk system
		DiskSystem()->FreeContentCookie(this);
		SetDiskSystem(NULL);
		// status
		SetStatus(B_PARTITION_UNINITIALIZED);
		// flags
		ClearFlags(B_PARTITION_FILE_SYSTEM | B_PARTITION_PARTITIONING_SYSTEM);
		if (!Device()->IsReadOnlyMedia())
			ClearFlags(B_PARTITION_READ_ONLY);
		// log changes
		if (logChanges) {
			Changed(flags, B_PARTITION_CHANGED_DEFRAGMENTATION
						   | B_PARTITION_CHANGED_CHECK
						   | B_PARTITION_CHANGED_REPAIR);
		}
	}
	return B_OK;
}

// SetAlgorithmData
void
KPartition::SetAlgorithmData(uint32 data)
{
	fAlgorithmData = data;
}

// AlgorithmData
uint32
KPartition::AlgorithmData() const
{
	return fAlgorithmData;
}

// WriteUserData
void
KPartition::WriteUserData(UserDataWriter &writer, user_partition_data *data)
{
	// allocate
	char *name = writer.PlaceString(Name());
	char *contentName = writer.PlaceString(ContentName());
	char *type = writer.PlaceString(Type());
	char *contentType = writer.PlaceString(ContentType());
	char *parameters = writer.PlaceString(Parameters());
	char *contentParameters = writer.PlaceString(ContentParameters());
	// fill in data
	if (data) {
		data->id = ID();
		data->shadow_id = -1;
		data->offset = Offset();
		data->size = Size();
		data->content_size = ContentSize();
		data->block_size = BlockSize();
		data->status = Status();
		data->flags = Flags();
		data->volume = VolumeID();
		data->index = Index();
		data->change_counter = ChangeCounter();
		data->disk_system = (DiskSystem() ? DiskSystem()->ID() : -1);
		data->name = name;
		data->content_name = contentName;
		data->type = type;
		data->content_type = contentType;
		data->parameters = parameters;
		data->content_parameters = contentParameters;
		data->child_count = CountChildren();
		// make buffer relocatable
		writer.AddRelocationEntry(&data->name);
		writer.AddRelocationEntry(&data->content_name);
		writer.AddRelocationEntry(&data->type);
		writer.AddRelocationEntry(&data->content_type);
		writer.AddRelocationEntry(&data->parameters);
		writer.AddRelocationEntry(&data->content_parameters);
	}
	// children
	for (int32 i = 0; KPartition *child = ChildAt(i); i++) {
		user_partition_data *childData
			= writer.AllocatePartitionData(child->CountChildren());
		if (data) {
			data->children[i] = childData;
			writer.AddRelocationEntry(&data->children[i]);
		}
		child->WriteUserData(writer, childData);
	}
}

// Dump
void
KPartition::Dump(bool deep, int32 level)
{
	if (level < 0 || level > 255)
		return;
	char prefix[256];
	sprintf(prefix, "%*s%*s", (int)level, "", (int)level, "");
	KPath path;
	GetPath(&path);
	if (level > 0)
		OUT("%spartition %ld: %s\n", prefix, ID(), path.Path());
	OUT("%s  offset:            %lld\n", prefix, Offset());
	OUT("%s  size:              %lld (%.2f MB)\n", prefix, Size(), Size() / (1024.0*1024));
	OUT("%s  content size:      %lld\n", prefix, ContentSize());
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

// FireOffsetChanged
void
KPartition::FireOffsetChanged(off_t offset)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->OffsetChanged(this, offset);
		}
	}
}

// FireSizeChanged
void
KPartition::FireSizeChanged(off_t size)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->SizeChanged(this, size);
		}
	}
}

// FireContentSizeChanged
void
KPartition::FireContentSizeChanged(off_t size)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentSizeChanged(this, size);
		}
	}
}

// FireBlockSizeChanged
void
KPartition::FireBlockSizeChanged(uint32 blockSize)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->BlockSizeChanged(this, blockSize);
		}
	}
}

// FireIndexChanged
void
KPartition::FireIndexChanged(int32 index)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->IndexChanged(this, index);
		}
	}
}

// FireStatusChanged
void
KPartition::FireStatusChanged(uint32 status)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->StatusChanged(this, status);
		}
	}
}

// FireFlagsChanged
void
KPartition::FireFlagsChanged(uint32 flags)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->FlagsChanged(this, flags);
		}
	}
}

// FireNameChanged
void
KPartition::FireNameChanged(const char *name)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->NameChanged(this, name);
		}
	}
}

// FireContentNameChanged
void
KPartition::FireContentNameChanged(const char *name)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentNameChanged(this, name);
		}
	}
}

// FireTypeChanged
void
KPartition::FireTypeChanged(const char *type)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->TypeChanged(this, type);
		}
	}
}

// FireIDChanged
void
KPartition::FireIDChanged(partition_id id)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->IDChanged(this, id);
		}
	}
}

// FireVolumeIDChanged
void
KPartition::FireVolumeIDChanged(dev_t volumeID)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->VolumeIDChanged(this, volumeID);
		}
	}
}

// FireMountCookieChanged
void
KPartition::FireMountCookieChanged(void *cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->MountCookieChanged(this, cookie);
		}
	}
}

// FireParametersChanged
void
KPartition::FireParametersChanged(const char *parameters)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ParametersChanged(this, parameters);
		}
	}
}

// FireContentParametersChanged
void
KPartition::FireContentParametersChanged(const char *parameters)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentParametersChanged(this, parameters);
		}
	}
}

// FireChildAdded
void
KPartition::FireChildAdded(KPartition *child, int32 index)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ChildAdded(this, child, index);
		}
	}
}

// FireChildRemoved
void
KPartition::FireChildRemoved(KPartition *child, int32 index)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ChildRemoved(this, child, index);
		}
	}
}

// FireDiskSystemChanged
void
KPartition::FireDiskSystemChanged(KDiskSystem *diskSystem)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->DiskSystemChanged(this, diskSystem);
		}
	}
}

// FireCookieChanged
void
KPartition::FireCookieChanged(void *cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->CookieChanged(this, cookie);
		}
	}
}

// FireContentCookieChanged
void
KPartition::FireContentCookieChanged(void *cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentCookieChanged(this, cookie);
		}
	}
}

// _UpdateChildIndices
void
KPartition::_UpdateChildIndices(int32 index)
{
	for (int32 i = index; i < fChildren.Count(); i++)
		fChildren.ElementAt(i)->SetIndex(i);
}

// _NextID
int32
KPartition::_NextID()
{
	return atomic_add(&fNextID, 1);
}


// fNextID
int32 KPartition::fNextID = 0;

