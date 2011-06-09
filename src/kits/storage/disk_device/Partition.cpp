/*
 * Copyright 2003-2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <new>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include <Directory.h>
#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceVisitor.h>
#include <DiskSystem.h>
#include <fs_volume.h>
#include <Message.h>
#include <ObjectList.h>
#include <Partition.h>
#include <PartitioningInfo.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>

#include <AutoDeleter.h>

#include <ddm_userland_interface_defs.h>
#include <syscalls.h>

#include "PartitionDelegate.h"


//#define TRACE_PARTITION
#undef TRACE
#ifdef TRACE_PARTITION
# define TRACE(x...) printf(x)
#else
# define TRACE(x...) do {} while (false)
#endif


using std::nothrow;

static const char *skAutoCreatePrefix = "_HaikuAutoCreated";

/*!	\class BPartition
	\brief A BPartition object represent a partition and provides a lot of
		   methods to retrieve information about it and some to manipulate it.

	Not all BPartitions represent actual on-disk partitions. Some exist only
	to make all devices fit smoothly into the framework (e.g. for floppies,
	\see IsVirtual()), others represents merely partition slots
	(\see IsEmpty()).
*/


// compare_string
/*!	\brief \c NULL aware strcmp().

	\c NULL is considered the least of all strings. \c NULL equals \c NULL.

	\param str1 First string.
	\param str2 Second string.
	\return A value less than 0, if \a str1 is less than \a str2,
			0, if they are equal, or a value greater than 0, if
			\a str1 is greater \a str2.
*/
static inline int
compare_string(const char* str1, const char* str2)
{
	if (str1 == NULL) {
		if (str2 == NULL)
			return 0;
		return 1;
	} else if (str2 == NULL)
		return -1;
	return strcmp(str1, str2);
}


// constructor
BPartition::BPartition()
	: fDevice(NULL),
	  fParent(NULL),
	  fPartitionData(NULL),
	  fDelegate(NULL)
{
}


// destructor
/*!	\brief Frees all resources associated with this object.
*/
BPartition::~BPartition()
{
	_Unset();
}


// Offset
/*!	\brief Returns the partition's offset relative to the beginning of the
		   device it resides on.
	\return The partition's offset in bytes relative to the beginning of the
			device it resides on.
*/
off_t
BPartition::Offset() const
{
	return _PartitionData()->offset;
}


// Size
/*!	\brief Returns the size of the partition.
	\return The size of the partition in bytes.
*/
off_t
BPartition::Size() const
{
	return _PartitionData()->size;
}


// ContentSize
off_t
BPartition::ContentSize() const
{
	return _PartitionData()->content_size;
}

// BlockSize
/*!	\brief Returns the block size of the device.
	\return The block size of the device in bytes.
*/
uint32
BPartition::BlockSize() const
{
	return _PartitionData()->block_size;
}


// Index
/*!	\brief Returns the index of the partition in its session's list of
		   partitions.
	\return The index of the partition in its session's list of partitions.
*/
int32
BPartition::Index() const
{
	return _PartitionData()->index;
}


// Status
uint32
BPartition::Status() const
{
	return _PartitionData()->status;
}


// ContainsFileSystem
bool
BPartition::ContainsFileSystem() const
{
	return _PartitionData()->flags & B_PARTITION_FILE_SYSTEM;
}


// ContainsPartitioningSystem
bool
BPartition::ContainsPartitioningSystem() const
{
	return _PartitionData()->flags & B_PARTITION_PARTITIONING_SYSTEM;
}


// IsDevice
bool
BPartition::IsDevice() const
{
	return _PartitionData()->flags & B_PARTITION_IS_DEVICE;
}


// IsReadOnly
bool
BPartition::IsReadOnly() const
{
	return _PartitionData()->flags & B_PARTITION_READ_ONLY;
}


// IsMounted
/*!	\brief Returns whether the volume is mounted.
	\return \c true, if the volume is mounted, \c false otherwise.
*/
bool
BPartition::IsMounted() const
{
	return _PartitionData()->flags & B_PARTITION_MOUNTED;
	// alternatively:
	// return _PartitionData()->volume >= 0;
}


// IsBusy
bool
BPartition::IsBusy() const
{
	return _PartitionData()->flags & B_PARTITION_BUSY;
}


// SupportsChildName
bool
BPartition::SupportsChildName() const
{
	return _SupportsChildOperation(NULL, B_DISK_SYSTEM_SUPPORTS_NAME);
}


// Flags
/*!	\brief Returns the flags for this partitions.

	The partition flags are a bitwise combination of:
	- \c B_HIDDEN_PARTITION: The partition can not contain a file system.
	- \c B_VIRTUAL_PARTITION: There exists no on-disk partition this object
	  represents. E.g. for floppies there will be a BPartition object spanning
	  the whole floppy disk.
	- \c B_EMPTY_PARTITION: The partition represents no physical partition,
	  but merely an empty slot. This mainly used to keep the indexing of
	  partitions more persistent. This flag implies also \c B_HIDDEN_PARTITION.

	\return The flags for this partition.
*/
uint32
BPartition::Flags() const
{
	return _PartitionData()->flags;
}


// Name
/*!	\brief Returns the name of the partition.

	Note, that not all partitioning system support names. The method returns
	\c NULL, if the partition doesn't have a name.

	\return The name of the partition, or \c NULL, if the partitioning system
			does not support names.
*/
const char*
BPartition::Name() const
{
	return _PartitionData()->name;
}


// ContentName
const char*
BPartition::ContentName() const
{
	return _PartitionData()->content_name;
}


// Type
/*!	\brief Returns a human readable string for the type of the partition.
	\return A human readable string for the type of the partition.
*/
const char*
BPartition::Type() const
{
	return _PartitionData()->type;
}


// ContentType
const char*
BPartition::ContentType() const
{
	return _PartitionData()->content_type;
}


// ID
/*!	\brief Returns a unique identifier for this partition.

	The ID is not persistent, i.e. in general won't be the same after
	rebooting.

	\see BDiskDeviceRoster::GetPartitionWithID().

	\return A unique identifier for this partition.
*/
int32
BPartition::ID() const
{
	return _PartitionData()->id;
}


// Parameters
const char*
BPartition::Parameters() const
{
	return _PartitionData()->parameters;
}


// ContentParameters
const char*
BPartition::ContentParameters() const
{
	return _PartitionData()->content_parameters;
}


// GetDiskSystem
status_t
BPartition::GetDiskSystem(BDiskSystem* diskSystem) const
{
	const user_partition_data* data = _PartitionData();
	if (!data || !diskSystem)
		return B_BAD_VALUE;

	if (data->disk_system < 0)
		return B_ENTRY_NOT_FOUND;

	return diskSystem->_SetTo(data->disk_system);
}


// GetPath
status_t
BPartition::GetPath(BPath* path) const
{
	// The path is constructed on the fly using our parent
	if (path == NULL || Parent() == NULL || Index() < 0)
		return B_BAD_VALUE;

	// get the parent's path
	status_t error = Parent()->GetPath(path);
	if (error != B_OK)
		return error;

	char indexBuffer[24];

	if (Parent()->IsDevice()) {
		// Our parent is a device, so we replace `raw' by our index.
		const char* leaf = path->Leaf();
		if (!leaf || strcmp(leaf, "raw") != B_OK)
			return B_ERROR;

		snprintf(indexBuffer, sizeof(indexBuffer), "%ld", Index());
	} else {
		// Our parent is a normal partition, no device: Append our index.
		snprintf(indexBuffer, sizeof(indexBuffer), "%s_%ld", path->Leaf(), Index());
	}

	error = path->GetParent(path);
	if (error == B_OK)
		error = path->Append(indexBuffer);

	return error;
}


// GetVolume
/*!	\brief Returns a BVolume for the partition.

	This can only succeed, if the partition is mounted.

	\param volume Pointer to a pre-allocated BVolume, to be initialized to
		   represent the volume.
	\return \c B_OK, if the volume is mounted and the parameter could be set
			accordingly, another error code otherwise.
*/
status_t
BPartition::GetVolume(BVolume* volume) const
{
	if (!volume)
		return B_BAD_VALUE;

	return volume->SetTo(_PartitionData()->volume);
}


// GetIcon
/*!	\brief Returns an icon for this partition.

	Note, that currently there are only per-device icons, i.e. the method
	returns the same icon for each partition of a device. But this may change
	in the future.

	\param icon Pointer to a pre-allocated BBitmap to be set to the icon of
		   the partition.
	\param which Size of the icon to be retrieved. Can be \c B_MINI_ICON or
		   \c B_LARGE_ICON.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartition::GetIcon(BBitmap* icon, icon_size which) const
{
	if (icon == NULL)
		return B_BAD_VALUE;

	status_t error;

	if (IsMounted()) {
		// mounted: get the icon from the volume
		BVolume volume;
		error = GetVolume(&volume);
		if (error == B_OK)
			error = volume.GetIcon(icon, which);
	} else {
		// not mounted: retrieve the icon ourselves
		if (BDiskDevice* device = Device()) {
			BPath path;
			error = device->GetPath(&path);
			// get the icon
			if (error == B_OK)
				error = get_device_icon(path.Path(), icon, which);
		} else
			error = B_ERROR;
	}
	return error;
}


status_t
BPartition::GetIcon(uint8** _data, size_t* _size, type_code* _type) const
{
	if (_data == NULL || _size == NULL || _type == NULL)
		return B_BAD_VALUE;

	status_t error;

	if (IsMounted()) {
		// mounted: get the icon from the volume
		BVolume volume;
		error = GetVolume(&volume);
		if (error == B_OK)
			error = volume.GetIcon(_data, _size, _type);
	} else {
		// not mounted: retrieve the icon ourselves
		if (BDiskDevice* device = Device()) {
			BPath path;
			error = device->GetPath(&path);
			// get the icon
			if (error == B_OK)
				error = get_device_icon(path.Path(), _data, _size, _type);
		} else
			error = B_ERROR;
	}
	return error;
}


// GetMountPoint
/*!	\brief Returns the mount point for the partition.

	If the partition is mounted this is the actual mount point. If it is not
	mounted, but contains a file system, derived from the partition name
	the name for a not yet existing directory in the root directory is
	constructed and the path to it returned.

	For partitions not containing a file system the method returns an error.

	\param mountPoint Pointer to the path to be set to refer the mount point
		   (respectively potential mount point) of the partition.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
BPartition::GetMountPoint(BPath* mountPoint) const
{
	if (!mountPoint || !ContainsFileSystem())
		return B_BAD_VALUE;

	// if the partition is mounted, return the actual mount point
	BVolume volume;
	if (GetVolume(&volume) == B_OK) {
		BDirectory dir;
		status_t error = volume.GetRootDirectory(&dir);
		if (error == B_OK)
			error = mountPoint->SetTo(&dir, NULL);
		return error;
	}

	// partition not mounted
	// get the volume name
	const char* volumeName = ContentName();
	if (!volumeName || strlen(volumeName) == 0)
		volumeName = Name();
	if (!volumeName || strlen(volumeName) == 0)
		volumeName = "unnamed volume";

	// construct a path name from the volume name
	// replace '/'s and prepend a '/'
	BString mountPointPath(volumeName);
	mountPointPath.ReplaceAll('/', '-');
	mountPointPath.Insert("/", 0);

	// make the name unique
	BString basePath(mountPointPath);
	int counter = 1;
	while (true) {
		BEntry entry;
		status_t error = entry.SetTo(mountPointPath.String());
		if (error != B_OK)
			return error;

		if (!entry.Exists())
			break;
		mountPointPath = basePath;
		mountPointPath << counter;
		counter++;
	}

	return mountPoint->SetTo(mountPointPath.String());
}


// Mount
/*!	\brief Mounts the volume.

	The volume can only be mounted, if the partition contains a recognized
	file system (\see ContainsFileSystem()) and it is not already mounted.

	If no mount point is given, one will be created automatically under the
	root directory (derived from the volume name). If one is given, the
	directory must already exist.

	\param mountPoint The directory where to mount the file system. May be
		   \c NULL, in which case a mount point in the root directory will be
		   created automatically.
	\param mountFlags Currently only \c B_MOUNT_READ_ONLY is defined, which
		   forces the volume to be mounted read-only.
	\param parameters File system specific mount parameters.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
dev_t
BPartition::Mount(const char* mountPoint, uint32 mountFlags,
	const char* parameters)
{
	if (IsMounted() || !ContainsFileSystem())
		return B_BAD_VALUE;

	// get the partition path
	BPath partitionPath;
	status_t error = GetPath(&partitionPath);
	if (error != B_OK)
		return error;

	// create a mount point, if none is given
	bool deleteMountPoint = false;
	BPath mountPointPath, markerPath;
	if (!mountPoint) {
		// get a unique mount point
		error = GetMountPoint(&mountPointPath);
		if (error != B_OK)
			return error;

		mountPoint = mountPointPath.Path();
		markerPath = mountPointPath;
		markerPath.Append(skAutoCreatePrefix);

		// create the directory
		if (mkdir(mountPoint, S_IRWXU | S_IRWXG | S_IRWXO) < 0)
			return errno;

		if (mkdir(markerPath.Path(), S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			rmdir(mountPoint);
			return errno;
		}

		deleteMountPoint = true;
	}

	// mount the partition
	dev_t device = fs_mount_volume(mountPoint, partitionPath.Path(), NULL,
		mountFlags, parameters);

	// delete the mount point on error, if we created it
	if (device < B_OK && deleteMountPoint) {
		rmdir(markerPath.Path());
		rmdir(mountPoint);
	}

	// update object, if successful
	if (device >= 0)
		error = Device()->Update();

	return device;
}


// Unmount
/*!	\brief Unmounts the volume.

	The volume can of course only be unmounted, if it currently is mounted.

	\param unmountFlags Currently only \c B_FORCE_UNMOUNT is defined, which
		   forces the partition to be unmounted, even if there are still
		   open FDs. Be careful using this flag -- you risk the user's data.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartition::Unmount(uint32 unmountFlags)
{
	if (!IsMounted())
		return B_BAD_VALUE;

	// get the partition path
	BPath path;
	status_t status = GetMountPoint(&path);
	if (status != B_OK)
		return status;

	// unmount
	status = fs_unmount_volume(path.Path(), unmountFlags);

	// update object, if successful
	if (status == B_OK) {
		status = Device()->Update();

		// Check if we created this mount point on the fly.
		// If so, clean it up.
		BPath markerPath = path;
		markerPath.Append(skAutoCreatePrefix);
		BEntry pathEntry (markerPath.Path());
		if (pathEntry.InitCheck() == B_OK && pathEntry.Exists()) {
			rmdir(markerPath.Path());
			rmdir(path.Path());
		}
	}

	return status;
}


// Device
/*!	\brief Returns the device this partition resides on.
	\return The device this partition resides on.
*/
BDiskDevice*
BPartition::Device() const
{
	return fDevice;
}


// Parent
BPartition*
BPartition::Parent() const
{
	return fParent;
}


// ChildAt
BPartition*
BPartition::ChildAt(int32 index) const
{
	if (fDelegate) {
		Delegate* child = fDelegate->ChildAt(index);
		return child ? child->Partition() : NULL;
	}

	return _ChildAt(index);
}


// CountChildren
int32
BPartition::CountChildren() const
{
	if (fDelegate)
		return fDelegate->CountChildren();

	return _CountChildren();
}


// CountDescendants
int32
BPartition::CountDescendants() const
{
	int32 count = 1;
	for (int32 i = 0; BPartition* child = ChildAt(i); i++)
		count += child->CountDescendants();
	return count;
}


// FindDescendant
BPartition*
BPartition::FindDescendant(partition_id id) const
{
	IDFinderVisitor visitor(id);
	return VisitEachDescendant(&visitor);
}


// GetPartitioningInfo
status_t
BPartition::GetPartitioningInfo(BPartitioningInfo* info) const
{
	if (!info)
		return B_BAD_VALUE;
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->GetPartitioningInfo(info);
}


// VisitEachChild
BPartition*
BPartition::VisitEachChild(BDiskDeviceVisitor* visitor) const
{
	if (visitor) {
		int32 level = _Level();
		for (int32 i = 0; BPartition* child = ChildAt(i); i++) {
			if (child->_AcceptVisitor(visitor, level))
				return child;
		}
	}
	return NULL;
}


// VisitEachDescendant
BPartition*
BPartition::VisitEachDescendant(BDiskDeviceVisitor* visitor) const
{
	if (visitor)
		return const_cast<BPartition*>(this)->_VisitEachDescendant(visitor);
	return NULL;
}


// CanDefragment
bool
BPartition::CanDefragment(bool* whileMounted) const
{
	return _SupportsOperation(B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING,
		B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED, whileMounted);
}


// Defragment
status_t
BPartition::Defragment() const
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->Defragment();
}


// CanRepair
bool
BPartition::CanRepair(bool checkOnly, bool* whileMounted) const
{
	uint32 flag;
	uint32 whileMountedFlag;
	if (checkOnly) {
		flag = B_DISK_SYSTEM_SUPPORTS_CHECKING;
		whileMountedFlag = B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED;
	} else {
		flag = B_DISK_SYSTEM_SUPPORTS_REPAIRING;
		whileMountedFlag = B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED;
	}

	return _SupportsOperation(flag, whileMountedFlag, whileMounted);
}


// Repair
status_t
BPartition::Repair(bool checkOnly) const
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->Repair(checkOnly);
}


// CanResize
bool
BPartition::CanResize(bool* canResizeContents, bool* whileMounted) const
{
	BPartition* parent = Parent();
	if (!parent)
		return false;

	if (!parent->_SupportsChildOperation(this,
			B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD)) {
		return false;
	}

	if (!_HasContent())
		return true;

	return _SupportsOperation(B_DISK_SYSTEM_SUPPORTS_RESIZING,
		B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED, whileMounted);
}


// ValidateResize
status_t
BPartition::ValidateResize(off_t* size) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	status_t error = parent->fDelegate->ValidateResizeChild(fDelegate, size);
	if (error != B_OK)
		return error;

	if (_HasContent()) {
// TODO: We would actually need the parameter for the content size.
		off_t contentSize = *size;
		error = fDelegate->ValidateResize(&contentSize);
		if (error != B_OK)
			return error;

		if (contentSize > *size)
			return B_BAD_VALUE;
	}

	return B_OK;
}


// Resize
status_t
BPartition::Resize(off_t size)
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	status_t error;
	off_t contentSize = size;
	if (_HasContent()) {
		error = fDelegate->ValidateResize(&contentSize);
		if (error != B_OK)
			return error;

		if (contentSize > size)
			return B_BAD_VALUE;
	}

	// If shrinking the partition, resize content first, otherwise last.
	bool shrink = (Size() > size);

	if (shrink && ContentType() != NULL) {
		error = fDelegate->Resize(contentSize);
		if (error != B_OK)
			return error;
	}

	error = parent->fDelegate->ResizeChild(fDelegate, size);
	if (error != B_OK)
		return error;

	if (!shrink && ContentType() != NULL) {
		error = fDelegate->Resize(contentSize);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// CanMove
bool
BPartition::CanMove(BObjectList<BPartition>* unmovableDescendants,
	BObjectList<BPartition>* movableOnlyIfUnmounted) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return false;

	if (!parent->_SupportsChildOperation(this,
			B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD)) {
		return false;
	}

	bool whileMounted;
	bool movable = _SupportsOperation(B_DISK_SYSTEM_SUPPORTS_MOVING,
		B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED, &whileMounted);
	if (!movable)
		return false;

	if (!whileMounted)
		movableOnlyIfUnmounted->AddItem(const_cast<BPartition*>(this));

	// collect descendent partitions
	// TODO: ...
// TODO: Currently there's no interface for asking descendents. They'll still
// have the same offset (relative to their parent) after moving. The only thing
// we really have to ask is whether they need to be unmounted.

	return true;
}


// ValidateMove
status_t
BPartition::ValidateMove(off_t* offset) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	status_t error = parent->fDelegate->ValidateMoveChild(fDelegate, offset);
	if (error != B_OK)
		return error;

	if (_HasContent()) {
		off_t contentOffset = *offset;
		error = fDelegate->ValidateMove(&contentOffset);
		if (error != B_OK)
			return error;

		if (contentOffset != *offset)
			return B_BAD_VALUE;
	}

	return B_OK;
}


// Move
status_t
BPartition::Move(off_t offset)
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	status_t error = parent->fDelegate->MoveChild(fDelegate, offset);
	if (error != B_OK)
		return error;

	if (_HasContent()) {
		error = fDelegate->Move(offset);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// CanSetName
bool
BPartition::CanSetName() const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return false;

	return parent->_SupportsChildOperation(this,
		B_DISK_SYSTEM_SUPPORTS_SETTING_NAME);
}


// ValidateSetName
status_t
BPartition::ValidateSetName(BString* name) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	return parent->fDelegate->ValidateSetName(fDelegate, name);
}


// SetName
status_t
BPartition::SetName(const char* name)
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	return parent->fDelegate->SetName(fDelegate, name);
}


// CanSetContentName
bool
BPartition::CanSetContentName(bool* whileMounted) const
{
	return _SupportsOperation(B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME,
		B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED,
		whileMounted);
}


// ValidateSetContentName
status_t
BPartition::ValidateSetContentName(BString* name) const
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->ValidateSetContentName(name);
}


// SetContentName
status_t
BPartition::SetContentName(const char* name)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->SetContentName(name);
}


// CanSetType
bool
BPartition::CanSetType() const
{
	BPartition* parent = Parent();
	if (!parent)
		return false;

	return parent->_SupportsChildOperation(this,
		B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE);
}


// ValidateSetType
status_t
BPartition::ValidateSetType(const char* type) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	return parent->fDelegate->ValidateSetType(fDelegate, type);
}


// SetType
status_t
BPartition::SetType(const char* type)
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	return parent->fDelegate->SetType(fDelegate, type);
}


// CanEditParameters
bool
BPartition::CanEditParameters() const
{
	BPartition* parent = Parent();
	if (!parent)
		return false;

	return parent->_SupportsChildOperation(this,
		B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS);
}


// GetParameterEditor
status_t
BPartition::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->GetParameterEditor(type, editor);
}


// SetParameters
status_t
BPartition::SetParameters(const char* parameters)
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return B_NO_INIT;

	return parent->fDelegate->SetParameters(fDelegate, parameters);
}


// CanEditContentParameters
bool
BPartition::CanEditContentParameters(bool* whileMounted) const
{
	return _SupportsOperation(B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS,
		B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED,
		whileMounted);
}


// SetContentParameters
status_t
BPartition::SetContentParameters(const char* parameters)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->SetContentParameters(parameters);
}


// GetNextSupportedType
status_t
BPartition::GetNextSupportedType(int32* cookie, BString* type) const
{
	TRACE("%p->BPartition::GetNextSupportedType(%ld)\n", this, *cookie);

	BPartition* parent = Parent();
	if (!parent || !fDelegate) {
		TRACE("  not prepared (parent: %p, fDelegate: %p)!\n", parent,
			fDelegate);
		return B_NO_INIT;
	}

	return parent->fDelegate->GetNextSupportedChildType(fDelegate, cookie,
		type);
}


// GetNextSupportedChildType
status_t
BPartition::GetNextSupportedChildType(int32* cookie, BString* type) const
{
	TRACE("%p->BPartition::GetNextSupportedChildType(%ld)\n", this, *cookie);

	if (!fDelegate) {
		TRACE("  not prepared!\n");
		return B_NO_INIT;
	}

	return fDelegate->GetNextSupportedChildType(NULL, cookie, type);
}


// IsSubSystem
bool
BPartition::BPartition::IsSubSystem(const char* diskSystem) const
{
	BPartition* parent = Parent();
	if (!parent || !fDelegate)
		return false;

	return parent->fDelegate->IsSubSystem(fDelegate, diskSystem);
}


// CanInitialize
bool
BPartition::CanInitialize(const char* diskSystem) const
{
	return fDelegate && fDelegate->CanInitialize(diskSystem);
}


// ValidateInitialize
status_t
BPartition::ValidateInitialize(const char* diskSystem, BString* name,
	const char* parameters)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->ValidateInitialize(diskSystem, name, parameters);
}


// Initialize
status_t
BPartition::Initialize(const char* diskSystem, const char* name,
	const char* parameters)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->Initialize(diskSystem, name, parameters);
}


// Uninitialize
status_t
BPartition::Uninitialize()
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


// CanCreateChild
bool
BPartition::CanCreateChild() const
{
	return _SupportsChildOperation(NULL, B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD);
}


// ValidateCreateChild
status_t
BPartition::ValidateCreateChild(off_t* offset, off_t* size, const char* type,
	BString* name, const char* parameters) const
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->ValidateCreateChild(offset, size, type, name, parameters);
}


// CreateChild
status_t
BPartition::CreateChild(off_t offset, off_t size, const char* type,
	const char* name, const char* parameters, BPartition** child)
{
	if (!fDelegate)
		return B_NO_INIT;

	return fDelegate->CreateChild(offset, size, type, name, parameters, child);
}


// CanDeleteChild
bool
BPartition::CanDeleteChild(int32 index) const
{
	BPartition* child = ChildAt(index);
	if (!fDelegate || !child)
		return false;

	return _SupportsChildOperation(child,
		B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD);
}


// DeleteChild
status_t
BPartition::DeleteChild(int32 index)
{
	if (!fDelegate)
		return B_NO_INIT;
	BPartition* child = ChildAt(index);
	if (!child || child->Parent() != this)
		return B_BAD_VALUE;

	return fDelegate->DeleteChild(child->fDelegate);
}


// constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BPartition::BPartition(const BPartition &)
{
}


// =
/*!	\brief Privatized assignment operator to avoid usage.
*/
BPartition &
BPartition::operator=(const BPartition &)
{
	return *this;
}


// _SetTo
status_t
BPartition::_SetTo(BDiskDevice* device, BPartition* parent,
	user_partition_data* data)
{
	_Unset();
	if (!device || !data)
		return B_BAD_VALUE;
	fPartitionData = data;
	fDevice = device;
	fParent = parent;
	fPartitionData->user_data = this;

	// create and init children
	status_t error = B_OK;
	for (int32 i = 0; error == B_OK && i < fPartitionData->child_count; i++) {
		BPartition* child = new(nothrow) BPartition;
		if (child) {
			error = child->_SetTo(fDevice, this, fPartitionData->children[i]);
			if (error != B_OK)
				delete child;
		} else
			error = B_NO_MEMORY;
	}

	// cleanup on error
	if (error != B_OK)
		_Unset();
	return error;
}


// _Unset
void
BPartition::_Unset()
{
	// delete children
	if (fPartitionData) {
		for (int32 i = 0; i < fPartitionData->child_count; i++) {
			if (BPartition* child = ChildAt(i))
				delete child;
		}
		fPartitionData->user_data = NULL;
	}

	fDevice = NULL;
	fParent = NULL;
	fPartitionData = NULL;
	fDelegate = NULL;
}


// _RemoveObsoleteDescendants
status_t
BPartition::_RemoveObsoleteDescendants(user_partition_data* data, bool* updated)
{
	// remove all children not longer persistent
	// Not exactly efficient: O(n^2), considering BList::RemoveItem()
	// O(1). We could do better (O(n*log(n))), when sorting the arrays before,
	// but then the list access is more random and we had to find the
	// BPartition to remove, which makes the list operation definitely O(n).
	int32 count = CountChildren();
	for (int32 i = count - 1; i >= 0; i--) {
		BPartition* child = ChildAt(i);
		bool found = false;
		for (int32 k = data->child_count - 1; k >= 0; k--) {
			if (data->children[k]->id == child->ID()) {
				// found partition: ask it to remove its obsolete descendants
				found = true;
				status_t error = child->_RemoveObsoleteDescendants(
					data->children[k], updated);
				if (error != B_OK)
					return error;

				// set the user data to the BPartition object to find it
				// quicker later
				data->children[k]->user_data = child;
				break;
			}
		}

		// if partition is obsolete, remove it
		if (!found) {
			*updated = true;
			_RemoveChild(i);
		}
	}
	return B_OK;
}


// _Update
status_t
BPartition::_Update(user_partition_data* data, bool* updated)
{
	user_partition_data* oldData = fPartitionData;
	fPartitionData = data;
	// check for changes
	if (data->offset != oldData->offset
		|| data->size != oldData->size
		|| data->block_size != oldData->block_size
		|| data->status != oldData->status
		|| data->flags != oldData->flags
		|| data->volume != oldData->volume
		|| data->disk_system != oldData->disk_system	// not needed
		|| compare_string(data->name, oldData->name)
		|| compare_string(data->content_name, oldData->content_name)
		|| compare_string(data->type, oldData->type)
		|| compare_string(data->content_type, oldData->content_type)
		|| compare_string(data->parameters, oldData->parameters)
		|| compare_string(data->content_parameters,
						  oldData->content_parameters)) {
		*updated = true;
	}

	// add new children and update existing ones
	status_t error = B_OK;
	for (int32 i = 0; i < data->child_count; i++) {
		user_partition_data* childData = data->children[i];
		BPartition* child = (BPartition*)childData->user_data;
		if (child) {
			// old partition
			error = child->_Update(childData, updated);
			if (error != B_OK)
				return error;
		} else {
			// new partition
			*updated = true;
			child = new(nothrow) BPartition;
			if (!child)
				return B_NO_MEMORY;

			error = child->_SetTo(fDevice, this, childData);
			if (error != B_OK) {
				delete child;
				return error;
			}

			childData->user_data = child;
		}
	}
	return error;
}


// _RemoveChild
void
BPartition::_RemoveChild(int32 index)
{
	int32 count = CountChildren();
	if (!fPartitionData || index < 0 || index >= count)
		return;

	// delete the BPartition and its children
	delete ChildAt(index);

	// compact the children array
	for (int32 i = index + 1; i < count; i++)
		fPartitionData->children[i - 1] = fPartitionData->children[i];
	fPartitionData->child_count--;
}


// _ChildAt
BPartition*
BPartition::_ChildAt(int32 index) const
{
	if (index < 0 || index >= fPartitionData->child_count)
		return NULL;
	return (BPartition*)fPartitionData->children[index]->user_data;
}


// _CountChildren
int32
BPartition::_CountChildren() const
{
	return fPartitionData->child_count;
}


// _CountDescendants
int32
BPartition::_CountDescendants() const
{
	int32 count = 1;
	for (int32 i = 0; BPartition* child = _ChildAt(i); i++)
		count += child->_CountDescendants();
	return count;
}


// _Level
int32
BPartition::_Level() const
{
	int32 level = 0;
	const BPartition* ancestor = this;
	while ((ancestor = ancestor->Parent()))
		level++;
	return level;
}


// _AcceptVisitor
bool
BPartition::_AcceptVisitor(BDiskDeviceVisitor* visitor, int32 level)
{
	return visitor->Visit(this, level);
}


// _VisitEachDescendant
BPartition*
BPartition::_VisitEachDescendant(BDiskDeviceVisitor* visitor, int32 level)
{
	if (level < 0)
		level = _Level();
	if (_AcceptVisitor(visitor, level))
		return this;
	for (int32 i = 0; BPartition* child = ChildAt(i); i++) {
		if (BPartition* result = child->_VisitEachDescendant(visitor,
				level + 1)) {
			return result;
		}
	}
	return NULL;
}


// _PartitionData
const user_partition_data*
BPartition::_PartitionData() const
{
	return fDelegate ? fDelegate->PartitionData() : fPartitionData;
}


// _HasContent
bool
BPartition::_HasContent() const
{
	return (ContentType() != NULL);
}


// _SupportsOperation
bool
BPartition::_SupportsOperation(uint32 flag, uint32 whileMountedFlag,
	bool* whileMounted) const
{
	if (!fDelegate)
		return false;

	uint32 supported = fDelegate->SupportedOperations(flag | whileMountedFlag);

	if (whileMounted)
		*whileMounted = supported & whileMountedFlag;

	return supported & flag;
}


// _SupportsChildOperation
bool
BPartition::_SupportsChildOperation(const BPartition* child, uint32 flag) const
{
	if (!fDelegate || (child && !child->fDelegate))
		return false;

	uint32 supported = fDelegate->SupportedChildOperations(
		child ? child->fDelegate : NULL, flag);

	return supported & flag;
}


// _CreateDelegates
status_t
BPartition::_CreateDelegates()
{
	if (fDelegate || !fPartitionData)
		return B_NO_INIT;

	// create and init delegate
	fDelegate = new(nothrow) Delegate(this);
	if (!fDelegate)
		return B_NO_MEMORY;

	status_t error = fDelegate->InitHierarchy(fPartitionData,
		fParent ? fParent->fDelegate : NULL);
	if (error != B_OK)
		return error;

	// create child delegates
	int32 count = _CountChildren();
	for (int32 i = 0; i < count; i++) {
		BPartition* child = _ChildAt(i);
		error = child->_CreateDelegates();
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// _InitDelegates
status_t
BPartition::_InitDelegates()
{
	// init delegate
	status_t error = fDelegate->InitAfterHierarchy();
	if (error != B_OK)
		return error;

	// recursively init child delegates
	int32 count = CountChildren();
	for (int32 i = 0; i < count; i++) {
		error = ChildAt(i)->_InitDelegates();
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// _DeleteDelegates
void
BPartition::_DeleteDelegates()
{
	// recursively delete child delegates
	int32 count = CountChildren();
	for (int32 i = count - 1; i >= 0; i--)
		ChildAt(i)->_DeleteDelegates();

	// delete delegate
	delete fDelegate;
	fDelegate = NULL;

	// Commit suicide, if the delegate was our only link to reality (i.e.
	// there's no physically existing partition we represent).
	if (fPartitionData == NULL)
		delete this;
}


// _IsModified
bool
BPartition::_IsModified() const
{
	if (!fDelegate)
		return false;

	return fDelegate->IsModified();
}
