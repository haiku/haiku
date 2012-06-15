/*
 * Copyright 2009, Bryce Groff, bgroff@hawaii.edu.
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2003-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KPartition.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <DiskDeviceRoster.h>
#include <Drivers.h>
#include <Errors.h>
#include <fs_volume.h>
#include <KernelExport.h>

#include <ddm_userland_interface.h>
#include <fs/devfs.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KPartitionListener.h>
#include <KPartitionVisitor.h>
#include <KPath.h>
#include <util/kernel_cpp.h>
#include <VectorSet.h>
#include <vfs.h>

#include "UserDataWriter.h"


using namespace std;


// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


struct KPartition::ListenerSet : VectorSet<KPartitionListener*> {};


int32 KPartition::sNextID = 0;


KPartition::KPartition(partition_id id)
	:
	fPartitionData(),
	fChildren(),
	fDevice(NULL),
	fParent(NULL),
	fDiskSystem(NULL),
	fDiskSystemPriority(-1),
	fListeners(NULL),
	fChangeFlags(0),
	fChangeCounter(0),
	fAlgorithmData(0),
	fReferenceCount(0),
	fObsolete(false),
	fPublishedName(NULL)
{
	fPartitionData.id = id >= 0 ? id : _NextID();
	fPartitionData.offset = 0;
	fPartitionData.size = 0;
	fPartitionData.content_size = 0;
	fPartitionData.block_size = 0;
	fPartitionData.child_count = 0;
	fPartitionData.index = -1;
	fPartitionData.status = B_PARTITION_UNRECOGNIZED;
	fPartitionData.flags = B_PARTITION_BUSY;
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


void
KPartition::Register()
{
	fReferenceCount++;
}


void
KPartition::Unregister()
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	ManagerLocker locker(manager);
	fReferenceCount--;
	if (IsObsolete() && fReferenceCount == 0) {
		// let the manager delete object
		manager->DeletePartition(this);
	}
}


int32
KPartition::CountReferences() const
{
	return fReferenceCount;
}


void
KPartition::MarkObsolete()
{
	fObsolete = true;
}


bool
KPartition::IsObsolete() const
{
	return fObsolete;
}


bool
KPartition::PrepareForRemoval()
{
	bool result = RemoveAllChildren();
	UninitializeContents();
	UnpublishDevice();
	if (ParentDiskSystem())
		ParentDiskSystem()->FreeCookie(this);
	if (DiskSystem())
		DiskSystem()->FreeContentCookie(this);
	return result;
}


bool
KPartition::PrepareForDeletion()
{
	return true;
}


status_t
KPartition::Open(int flags, int* fd)
{
	if (!fd)
		return B_BAD_VALUE;

	// get the path
	KPath path;
	status_t error = GetPath(&path);
	if (error != B_OK)
		return error;

	// open the device
	*fd = open(path.Path(), flags);
	if (*fd < 0)
		return errno;

	return B_OK;
}


status_t
KPartition::PublishDevice()
{
	if (fPublishedName)
		return B_OK;

	// get the name to publish
	char buffer[B_FILE_NAME_LENGTH];
	status_t error = GetFileName(buffer, B_FILE_NAME_LENGTH);
	if (error != B_OK)
		return error;

	// prepare a partition_info
	partition_info info;
	info.offset = Offset();
	info.size = Size();
	info.logical_block_size = BlockSize();
	info.session = 0;
	info.partition = ID();
	if (strlcpy(info.device, Device()->Path(), sizeof(info.device))
			>= sizeof(info.device)) {
		return B_NAME_TOO_LONG;
	}

	fPublishedName = strdup(buffer);
	if (!fPublishedName)
		return B_NO_MEMORY;

	error = devfs_publish_partition(buffer, &info);
	if (error != B_OK) {
		dprintf("KPartition::PublishDevice(): Failed to publish partition "
			"%" B_PRId32 ": %s\n", ID(), strerror(error));
		free(fPublishedName);
		fPublishedName = NULL;
		return error;
	}

	return B_OK;
}


status_t
KPartition::UnpublishDevice()
{
	if (!fPublishedName)
		return B_OK;

	// get the path
	KPath path;
	status_t error = GetPath(&path);
	if (error != B_OK) {
		dprintf("KPartition::UnpublishDevice(): Failed to get path for "
			"partition %" B_PRId32 ": %s\n", ID(), strerror(error));
		return error;
	}

	error = devfs_unpublish_partition(path.Path());
	if (error != B_OK) {
		dprintf("KPartition::UnpublishDevice(): Failed to unpublish partition "
			"%" B_PRId32 ": %s\n", ID(), strerror(error));
	}

	free(fPublishedName);
	fPublishedName = NULL;

	return error;
}


status_t
KPartition::RepublishDevice()
{
	if (!fPublishedName)
		return B_OK;

	char newNameBuffer[B_FILE_NAME_LENGTH];
	status_t error = GetFileName(newNameBuffer, B_FILE_NAME_LENGTH);
	if (error != B_OK) {
		UnpublishDevice();
		return error;
	}

	if (strcmp(fPublishedName, newNameBuffer) == 0)
		return B_OK;

	for (int i = 0; i < CountChildren(); i++)
		ChildAt(i)->RepublishDevice();

	char* newName = strdup(newNameBuffer);
	if (!newName) {
		UnpublishDevice();
		return B_NO_MEMORY;
	}

	error = devfs_rename_partition(Device()->Path(), fPublishedName, newName);

	if (error != B_OK) {
		free(newName);
		UnpublishDevice();
		dprintf("KPartition::RepublishDevice(): Failed to republish partition "
			"%" B_PRId32 ": %s\n", ID(), strerror(error));
		return error;
	}

	free(fPublishedName);
	fPublishedName = newName;

	return B_OK;
}


bool
KPartition::IsPublished() const
{
	return fPublishedName != NULL;
}


void
KPartition::SetBusy(bool busy)
{
	if (busy)
		AddFlags(B_PARTITION_BUSY);
	else
		ClearFlags(B_PARTITION_BUSY);
}


bool
KPartition::IsBusy() const
{
	return (fPartitionData.flags & B_PARTITION_BUSY) != 0;
}


bool
KPartition::IsBusy(bool includeDescendants)
{
	if (!includeDescendants)
		return IsBusy();

	struct IsBusyVisitor : KPartitionVisitor {
		virtual bool VisitPre(KPartition* partition)
		{
			return partition->IsBusy();
		}
	} checkVisitor;

	return VisitEachDescendant(&checkVisitor) != NULL;
}


bool
KPartition::CheckAndMarkBusy(bool includeDescendants)
{
	if (IsBusy(includeDescendants))
		return false;

	MarkBusy(includeDescendants);

	return true;
}


void
KPartition::MarkBusy(bool includeDescendants)
{
	if (includeDescendants) {
		struct MarkBusyVisitor : KPartitionVisitor {
			virtual bool VisitPre(KPartition* partition)
			{
				partition->AddFlags(B_PARTITION_BUSY);
				return false;
			}
		} markVisitor;

		VisitEachDescendant(&markVisitor);
	} else
		SetBusy(true);
}


void
KPartition::UnmarkBusy(bool includeDescendants)
{
	if (includeDescendants) {
		struct UnmarkBusyVisitor : KPartitionVisitor {
			virtual bool VisitPre(KPartition* partition)
			{
				partition->ClearFlags(B_PARTITION_BUSY);
				return false;
			}
		} visitor;

		VisitEachDescendant(&visitor);
	} else
		SetBusy(false);
}


void
KPartition::SetOffset(off_t offset)
{
	if (fPartitionData.offset != offset) {
		fPartitionData.offset = offset;
		FireOffsetChanged(offset);
	}
}


off_t
KPartition::Offset() const
{
	return fPartitionData.offset;
}


void
KPartition::SetSize(off_t size)
{
	if (fPartitionData.size != size) {
		fPartitionData.size = size;
		FireSizeChanged(size);
	}
}


off_t
KPartition::Size() const
{
	return fPartitionData.size;
}


void
KPartition::SetContentSize(off_t size)
{
	if (fPartitionData.content_size != size) {
		fPartitionData.content_size = size;
		FireContentSizeChanged(size);
	}
}


off_t
KPartition::ContentSize() const
{
	return fPartitionData.content_size;
}


void
KPartition::SetBlockSize(uint32 blockSize)
{
	if (fPartitionData.block_size != blockSize) {
		fPartitionData.block_size = blockSize;
		FireBlockSizeChanged(blockSize);
	}
}


uint32
KPartition::BlockSize() const
{
	return fPartitionData.block_size;
}


void
KPartition::SetIndex(int32 index)
{
	if (fPartitionData.index != index) {
		fPartitionData.index = index;
		FireIndexChanged(index);
	}
}


int32
KPartition::Index() const
{
	return fPartitionData.index;
}


void
KPartition::SetStatus(uint32 status)
{
	if (fPartitionData.status != status) {
		fPartitionData.status = status;
		FireStatusChanged(status);
	}
}


uint32
KPartition::Status() const
{
	return fPartitionData.status;
}


bool
KPartition::IsUninitialized() const
{
	return Status() == B_PARTITION_UNINITIALIZED;
}


void
KPartition::SetFlags(uint32 flags)
{
	if (fPartitionData.flags != flags) {
		fPartitionData.flags = flags;
		FireFlagsChanged(flags);
	}
}


void
KPartition::AddFlags(uint32 flags)
{
	if (~fPartitionData.flags & flags) {
		fPartitionData.flags |= flags;
		FireFlagsChanged(fPartitionData.flags);
	}
}


void
KPartition::ClearFlags(uint32 flags)
{
	if (fPartitionData.flags & flags) {
		fPartitionData.flags &= ~flags;
		FireFlagsChanged(fPartitionData.flags);
	}
}


uint32
KPartition::Flags() const
{
	return fPartitionData.flags;
}


bool
KPartition::ContainsFileSystem() const
{
	return (fPartitionData.flags & B_PARTITION_FILE_SYSTEM) != 0;
}


bool
KPartition::ContainsPartitioningSystem() const
{
	return (fPartitionData.flags & B_PARTITION_PARTITIONING_SYSTEM) != 0;
}


bool
KPartition::IsReadOnly() const
{
	return (fPartitionData.flags & B_PARTITION_READ_ONLY) != 0;
}


bool
KPartition::IsMounted() const
{
	return (fPartitionData.flags & B_PARTITION_MOUNTED) != 0;
}


bool
KPartition::IsDevice() const
{
	return (fPartitionData.flags & B_PARTITION_IS_DEVICE) != 0;
}


status_t
KPartition::SetName(const char* name)
{
	status_t error = set_string(fPartitionData.name, name);
	FireNameChanged(fPartitionData.name);
	return error;
}


const char*
KPartition::Name() const
{
	return fPartitionData.name;
}


status_t
KPartition::SetContentName(const char* name)
{
	status_t error = set_string(fPartitionData.content_name, name);
	FireContentNameChanged(fPartitionData.content_name);
	return error;
}


const char*
KPartition::ContentName() const
{
	return fPartitionData.content_name;
}


status_t
KPartition::SetType(const char* type)
{
	status_t error = set_string(fPartitionData.type, type);
	FireTypeChanged(fPartitionData.type);
	return error;
}


const char*
KPartition::Type() const
{
	return fPartitionData.type;
}


const char*
KPartition::ContentType() const
{
	return fPartitionData.content_type;
}


partition_data*
KPartition::PartitionData()
{
	return &fPartitionData;
}


const partition_data*
KPartition::PartitionData() const
{
	return &fPartitionData;
}


void
KPartition::SetID(partition_id id)
{
	if (fPartitionData.id != id) {
		fPartitionData.id = id;
		FireIDChanged(id);
	}
}


partition_id
KPartition::ID() const
{
	return fPartitionData.id;
}


status_t
KPartition::GetFileName(char* buffer, size_t size) const
{
	// If the parent is the device, the name is the index of the partition.
	if (Parent() == NULL || Parent()->IsDevice()) {
		if (snprintf(buffer, size, "%" B_PRId32, Index()) >= (int)size)
			return B_NAME_TOO_LONG;
		return B_OK;
	}

	// The partition has a non-device parent, so we append the index to the
	// parent partition's name.
	status_t error = Parent()->GetFileName(buffer, size);
	if (error != B_OK)
		return error;

	size_t len = strlen(buffer);
	if (snprintf(buffer + len, size - len, "_%" B_PRId32, Index()) >= int(size - len))
		return B_NAME_TOO_LONG;
	return B_OK;
}


status_t
KPartition::GetPath(KPath* path) const
{
	// For a KDiskDevice this version is never invoked, so the check for
	// Parent() is correct.
	if (!path || path->InitCheck() != B_OK || !Parent() || Index() < 0)
		return B_BAD_VALUE;

	// init the path with the device path
	status_t error = path->SetPath(Device()->Path());
	if (error != B_OK)
		return error;

	// replace the leaf name with the partition's file name
	char name[B_FILE_NAME_LENGTH];
	error = GetFileName(name, sizeof(name));
	if (error == B_OK)
		error = path->ReplaceLeaf(name);

	return error;
}


void
KPartition::SetVolumeID(dev_t volumeID)
{
	if (fPartitionData.volume == volumeID)
		return;

	fPartitionData.volume = volumeID;
	FireVolumeIDChanged(volumeID);
	if (VolumeID() >= 0)
		AddFlags(B_PARTITION_MOUNTED);
	else
		ClearFlags(B_PARTITION_MOUNTED);

	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_DEVICE_UPDATE);
	message.AddInt32("event", volumeID >= 0
		? B_DEVICE_PARTITION_MOUNTED : B_DEVICE_PARTITION_UNMOUNTED);
	message.AddInt32("id", ID());
	if (volumeID >= 0)
		message.AddInt32("volume", volumeID);

	manager->Notify(message, B_DEVICE_REQUEST_MOUNTING);
}


dev_t
KPartition::VolumeID() const
{
	return fPartitionData.volume;
}


void
KPartition::SetMountCookie(void* cookie)
{
	if (fPartitionData.mount_cookie != cookie) {
		fPartitionData.mount_cookie = cookie;
		FireMountCookieChanged(cookie);
	}
}


void*
KPartition::MountCookie() const
{
	return fPartitionData.mount_cookie;
}


status_t
KPartition::Mount(uint32 mountFlags, const char* parameters)
{
	// not implemented
	return B_ERROR;
}


status_t
KPartition::Unmount()
{
	// not implemented
	return B_ERROR;
}


status_t
KPartition::SetParameters(const char* parameters)
{
	status_t error = set_string(fPartitionData.parameters, parameters);
	FireParametersChanged(fPartitionData.parameters);
	return error;
}


const char*
KPartition::Parameters() const
{
	return fPartitionData.parameters;
}


status_t
KPartition::SetContentParameters(const char* parameters)
{
	status_t error = set_string(fPartitionData.content_parameters, parameters);
	FireContentParametersChanged(fPartitionData.content_parameters);
	return error;
}


const char*
KPartition::ContentParameters() const
{
	return fPartitionData.content_parameters;
}


void
KPartition::SetDevice(KDiskDevice* device)
{
	fDevice = device;
	if (fDevice != NULL && fDevice->IsReadOnlyMedia())
		AddFlags(B_PARTITION_READ_ONLY);
}


KDiskDevice*
KPartition::Device() const
{
	return fDevice;
}


void
KPartition::SetParent(KPartition* parent)
{
	// Must be called in a {Add,Remove}Child() only!
	fParent = parent;
}


KPartition*
KPartition::Parent() const
{
	return fParent;
}


status_t
KPartition::AddChild(KPartition* partition, int32 index)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count || !partition)
		return B_BAD_VALUE;

	// add partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		status_t error = fChildren.Insert(partition, index);
		if (error != B_OK)
			return error;
		if (!manager->PartitionAdded(partition)) {
			fChildren.Erase(index);
			return B_NO_MEMORY;
		}
		// update siblings index's
		partition->SetIndex(index);
		_UpdateChildIndices(count, index);
		fPartitionData.child_count++;

		partition->SetParent(this);
		partition->SetDevice(Device());

		// publish to devfs
		partition->PublishDevice();

		// notify listeners
		FireChildAdded(partition, index);
		return B_OK;
	}
	return B_ERROR;
}


status_t
KPartition::CreateChild(partition_id id, int32 index, off_t offset, off_t size,
	KPartition** _child)
{
	// check parameters
	int32 count = fPartitionData.child_count;
	if (index == -1)
		index = count;
	if (index < 0 || index > count)
		return B_BAD_VALUE;

	// create and add partition
	KPartition* child = new(std::nothrow) KPartition(id);
	if (child == NULL)
		return B_NO_MEMORY;

	child->SetOffset(offset);
	child->SetSize(size);

	status_t error = AddChild(child, index);

	// cleanup / set result
	if (error != B_OK)
		delete child;
	else if (_child)
		*_child = child;

	return error;
}


bool
KPartition::RemoveChild(int32 index)
{
	if (index < 0 || index >= fPartitionData.child_count)
		return false;

	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		KPartition* partition = fChildren.ElementAt(index);
		PartitionRegistrar _(partition);
		if (!partition || !manager->PartitionRemoved(partition)
			|| !fChildren.Erase(index)) {
			return false;
		}
		_UpdateChildIndices(index, fChildren.Count());
		partition->SetIndex(-1);
		fPartitionData.child_count--;
		partition->SetParent(NULL);
		partition->SetDevice(NULL);
		// notify listeners
		FireChildRemoved(partition, index);
		return true;
	}
	return false;
}


bool
KPartition::RemoveChild(KPartition* child)
{
	if (child) {
		int32 index = fChildren.IndexOf(child);
		if (index >= 0)
			return RemoveChild(index);
	}
	return false;
}


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


KPartition*
KPartition::ChildAt(int32 index) const
{
	return index >= 0 && index < fChildren.Count()
		? fChildren.ElementAt(index) : NULL;
}


int32
KPartition::CountChildren() const
{
	return fPartitionData.child_count;
}


int32
KPartition::CountDescendants() const
{
	int32 count = 1;
	for (int32 i = 0; KPartition* child = ChildAt(i); i++)
		count += child->CountDescendants();
	return count;
}


KPartition*
KPartition::VisitEachDescendant(KPartitionVisitor* visitor)
{
	if (!visitor)
		return NULL;
	if (visitor->VisitPre(this))
		return this;
	for (int32 i = 0; KPartition* child = ChildAt(i); i++) {
		if (KPartition* result = child->VisitEachDescendant(visitor))
			return result;
	}
	if (visitor->VisitPost(this))
		return this;
	return NULL;
}


void
KPartition::SetDiskSystem(KDiskSystem* diskSystem, float priority)
{
	// unload former disk system
	if (fDiskSystem) {
		fPartitionData.content_type = NULL;
		fDiskSystem->Unload();
		fDiskSystem = NULL;
		fDiskSystemPriority = -1;
	}
	// set and load new one
	fDiskSystem = diskSystem;
	if (fDiskSystem) {
		fDiskSystem->Load();
			// can't fail, since it's already loaded
	}
	// update concerned partition flags
	if (fDiskSystem) {
		fPartitionData.content_type = fDiskSystem->PrettyName();
		fDiskSystemPriority = priority;
		if (fDiskSystem->IsFileSystem())
			AddFlags(B_PARTITION_FILE_SYSTEM);
		else
			AddFlags(B_PARTITION_PARTITIONING_SYSTEM);
	}
	// notify listeners
	FireDiskSystemChanged(fDiskSystem);

	KDiskDeviceManager* manager = KDiskDeviceManager::Default();

	char messageBuffer[512];
	KMessage message;
	message.SetTo(messageBuffer, sizeof(messageBuffer), B_DEVICE_UPDATE);
	message.AddInt32("event", B_DEVICE_PARTITION_INITIALIZED);
	message.AddInt32("id", ID());

	manager->Notify(message, B_DEVICE_REQUEST_PARTITION);
}


KDiskSystem*
KPartition::DiskSystem() const
{
	return fDiskSystem;
}


float
KPartition::DiskSystemPriority() const
{
	return fDiskSystemPriority;
}


KDiskSystem*
KPartition::ParentDiskSystem() const
{
	return Parent() ? Parent()->DiskSystem() : NULL;
}


void
KPartition::SetCookie(void* cookie)
{
	if (fPartitionData.cookie != cookie) {
		fPartitionData.cookie = cookie;
		FireCookieChanged(cookie);
	}
}


void*
KPartition::Cookie() const
{
	return fPartitionData.cookie;
}


void
KPartition::SetContentCookie(void* cookie)
{
	if (fPartitionData.content_cookie != cookie) {
		fPartitionData.content_cookie = cookie;
		FireContentCookieChanged(cookie);
	}
}


void*
KPartition::ContentCookie() const
{
	return fPartitionData.content_cookie;
}


bool
KPartition::AddListener(KPartitionListener* listener)
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
	return fListeners->Insert(listener) == B_OK;
}


bool
KPartition::RemoveListener(KPartitionListener* listener)
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


void
KPartition::Changed(uint32 flags, uint32 clearFlags)
{
	fChangeFlags &= ~clearFlags;
	fChangeFlags |= flags;
	fChangeCounter++;
	if (Parent())
		Parent()->Changed(B_PARTITION_CHANGED_DESCENDANTS);
}


void
KPartition::SetChangeFlags(uint32 flags)
{
	fChangeFlags = flags;
}


uint32
KPartition::ChangeFlags() const
{
	return fChangeFlags;
}


int32
KPartition::ChangeCounter() const
{
	return fChangeCounter;
}


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
			status_t error = vfs_unmount(VolumeID(),
				B_FORCE_UNMOUNT | B_UNMOUNT_BUSY_PARTITION);
			if (error != B_OK) {
				dprintf("KPartition::UninitializeContents(): Failed to unmount "
					"device %" B_PRIdDEV ": %s\n", VolumeID(), strerror(error));
			}

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
				| B_PARTITION_CHANGED_CHECK | B_PARTITION_CHANGED_REPAIR);
		}
	}

	return B_OK;
}


void
KPartition::SetAlgorithmData(uint32 data)
{
	fAlgorithmData = data;
}


uint32
KPartition::AlgorithmData() const
{
	return fAlgorithmData;
}


void
KPartition::WriteUserData(UserDataWriter& writer, user_partition_data* data)
{
	// allocate
	char* name = writer.PlaceString(Name());
	char* contentName = writer.PlaceString(ContentName());
	char* type = writer.PlaceString(Type());
	char* contentType = writer.PlaceString(ContentType());
	char* parameters = writer.PlaceString(Parameters());
	char* contentParameters = writer.PlaceString(ContentParameters());
	// fill in data
	if (data) {
		data->id = ID();
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
	for (int32 i = 0; KPartition* child = ChildAt(i); i++) {
		user_partition_data* childData
			= writer.AllocatePartitionData(child->CountChildren());
		if (data) {
			data->children[i] = childData;
			writer.AddRelocationEntry(&data->children[i]);
		}
		child->WriteUserData(writer, childData);
	}
}


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
		OUT("%spartition %" B_PRId32 ": %s\n", prefix, ID(), path.Path());
	OUT("%s  offset:            %lld\n", prefix, Offset());
	OUT("%s  size:              %lld (%.2f MB)\n", prefix, Size(),
		Size() / (1024.0*1024));
	OUT("%s  content size:      %lld\n", prefix, ContentSize());
	OUT("%s  block size:        %" B_PRIu32 "\n", prefix, BlockSize());
	OUT("%s  child count:       %" B_PRId32 "\n", prefix, CountChildren());
	OUT("%s  index:             %" B_PRId32 "\n", prefix, Index());
	OUT("%s  status:            %" B_PRIu32 "\n", prefix, Status());
	OUT("%s  flags:             %" B_PRIx32 "\n", prefix, Flags());
	OUT("%s  volume:            %" B_PRIdDEV "\n", prefix, VolumeID());
	OUT("%s  disk system:       %s\n", prefix,
		(DiskSystem() ? DiskSystem()->Name() : NULL));
	OUT("%s  name:              %s\n", prefix, Name());
	OUT("%s  content name:      %s\n", prefix, ContentName());
	OUT("%s  type:              %s\n", prefix, Type());
	OUT("%s  content type:      %s\n", prefix, ContentType());
	OUT("%s  params:            %s\n", prefix, Parameters());
	OUT("%s  content params:    %s\n", prefix, ContentParameters());
	if (deep) {
		for (int32 i = 0; KPartition* child = ChildAt(i); i++)
			child->Dump(true, level + 1);
	}
}


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


void
KPartition::FireNameChanged(const char* name)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->NameChanged(this, name);
		}
	}
}


void
KPartition::FireContentNameChanged(const char* name)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentNameChanged(this, name);
		}
	}
}


void
KPartition::FireTypeChanged(const char* type)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->TypeChanged(this, type);
		}
	}
}


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


void
KPartition::FireMountCookieChanged(void* cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->MountCookieChanged(this, cookie);
		}
	}
}


void
KPartition::FireParametersChanged(const char* parameters)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ParametersChanged(this, parameters);
		}
	}
}


void
KPartition::FireContentParametersChanged(const char* parameters)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentParametersChanged(this, parameters);
		}
	}
}


void
KPartition::FireChildAdded(KPartition* child, int32 index)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ChildAdded(this, child, index);
		}
	}
}


void
KPartition::FireChildRemoved(KPartition* child, int32 index)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ChildRemoved(this, child, index);
		}
	}
}


void
KPartition::FireDiskSystemChanged(KDiskSystem* diskSystem)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->DiskSystemChanged(this, diskSystem);
		}
	}
}


void
KPartition::FireCookieChanged(void* cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->CookieChanged(this, cookie);
		}
	}
}


void
KPartition::FireContentCookieChanged(void* cookie)
{
	if (fListeners) {
		for (ListenerSet::Iterator it = fListeners->Begin();
			 it != fListeners->End(); ++it) {
			(*it)->ContentCookieChanged(this, cookie);
		}
	}
}


void
KPartition::_UpdateChildIndices(int32 start, int32 end)
{
	if (start < end) {
		for (int32 i = start; i < end; i++) {
			fChildren.ElementAt(i)->SetIndex(i);
			fChildren.ElementAt(i)->RepublishDevice();
		}
	} else {
		for (int32 i = start; i > end; i--) {
			fChildren.ElementAt(i)->SetIndex(i);
			fChildren.ElementAt(i)->RepublishDevice();
		}
	}
}


int32
KPartition::_NextID()
{
	return atomic_add(&sNextID, 1);
}
