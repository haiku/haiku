//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <errno.h>
#include <new>
#include <unistd.h>
#include <sys/stat.h>

#include <syscalls.h>

#include <Directory.h>
#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceVisitor.h>
#include <DiskSystem.h>
#include <fs_volume.h>
#include <Message.h>
#include <Partition.h>
#include <PartitioningInfo.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>

/*!	\class BPartition
	\brief A BPartition object represent a partition and provides a lot of
		   methods to retrieve information about it and some to manipulate it.

	Not all BPartitions represent actual on-disk partitions. Some exist only
	to make all devices fit smoothly into the framework (e.g. for floppies,
	\see IsVirtual()), others represents merely partition slots
	(\see IsEmpty()).
*/

// AutoDeleter
/*!	\brief Helper class deleting objects automatically.
*/
template<typename C>
class AutoDeleter {
public:
	inline AutoDeleter(C *data = NULL, bool array = false)
		: fData(data), fArray(array) {}

	inline ~AutoDeleter()
	{
		if (fArray)
			delete[] fData;
		else
			delete fData;
	}

	inline void SetTo(C *data, bool array = false)
	{
		fData = data;
		fArray = array;
	}

	C		*fData;
	bool	fArray;
};

// compare_string
/*!	\brief \c NULL aware strcmp().

	\c NULL is considered the least of all strings. \c NULL equals \c NULL.

	\param str1 First string.
	\param str2 Second string.
	\return A value less than 0, if \a str1 is less than \a str2,
			0, if they are equal, or a value greater than 0, if
			\a str1 is greater \a str2.
*/
static inline
int
compare_string(const char *str1, const char *str2)
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
	  fPartitionData(NULL)
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
	return (fPartitionData ? fPartitionData->offset : 0);
}

// Size
/*!	\brief Returns the size of the partition.
	\return The size of the partition in bytes.
*/
off_t
BPartition::Size() const
{
	return (fPartitionData ? fPartitionData->size : 0);
}

// ContentSize
off_t
BPartition::ContentSize() const
{
	return (fPartitionData ? fPartitionData->content_size : 0);
}

// BlockSize
/*!	\brief Returns the block size of the device.
	\return The block size of the device in bytes.
*/
uint32
BPartition::BlockSize() const
{
	return (fPartitionData ? fPartitionData->block_size : 0);
}

// Index
/*!	\brief Returns the index of the partition in its session's list of
		   partitions.
	\return The index of the partition in its session's list of partitions.
*/
int32
BPartition::Index() const
{
	return (fPartitionData ? fPartitionData->index : -1);
}

// Status
uint32
BPartition::Status() const
{
	return (fPartitionData ? fPartitionData->status : 0);
}

// ContainsFileSystem
bool
BPartition::ContainsFileSystem() const
{
	return (fPartitionData
			&& (fPartitionData->flags & B_PARTITION_FILE_SYSTEM));
}

// ContainsPartitioningSystem
bool
BPartition::ContainsPartitioningSystem() const
{
	return (fPartitionData
			&& (fPartitionData->flags & B_PARTITION_PARTITIONING_SYSTEM));
}

// IsDevice
bool
BPartition::IsDevice() const
{
	return (fPartitionData
			&& (fPartitionData->flags & B_PARTITION_IS_DEVICE));
}

// IsReadOnly
bool
BPartition::IsReadOnly() const
{
	return (fPartitionData
			&& (fPartitionData->flags & B_PARTITION_READ_ONLY));
}

// IsMounted
/*!	\brief Returns whether the volume is mounted.
	\return \c true, if the volume is mounted, \c false otherwise.
*/
bool
BPartition::IsMounted() const
{
	return (fPartitionData
			&& (fPartitionData->flags & B_PARTITION_MOUNTED));
	// alternatively:
	// return (fPartitionData && fPartitionData->volume >= 0);
}

// IsBusy
bool
BPartition::IsBusy() const
{
	return (fPartitionData
			&& (fPartitionData->flags
				& (B_PARTITION_BUSY | B_PARTITION_DESCENDANT_BUSY)));
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
	return (fPartitionData ? fPartitionData->flags : 0);
}

// Name
/*!	\brief Returns the name of the partition.

	Note, that not all partitioning system support names. The method returns
	\c NULL, if the partition doesn't have a name.

	\return The name of the partition, or \c NULL, if the partitioning system
			does not support names.
*/
const char *
BPartition::Name() const
{
	return (fPartitionData ? fPartitionData->name : NULL);
}

// ContentName
const char *
BPartition::ContentName() const
{
	return (fPartitionData ? fPartitionData->content_name : NULL);
}

// Type
/*!	\brief Returns a human readable string for the type of the partition.
	\return A human readable string for the type of the partition.
*/
const char *
BPartition::Type() const
{
	return (fPartitionData ? fPartitionData->type : NULL);
}

// ContentType
const char *
BPartition::ContentType() const
{
	return (fPartitionData ? fPartitionData->content_type : NULL);
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
	return (fPartitionData ? fPartitionData->id : -1);
}

// GetDiskSystem
status_t
BPartition::GetDiskSystem(BDiskSystem *diskSystem) const
{
	if (!fPartitionData || !diskSystem)
		return B_BAD_VALUE;
	if (fPartitionData->disk_system < 0)
		return B_ENTRY_NOT_FOUND;
	return diskSystem->_SetTo(fPartitionData->disk_system);
}

// GetPath
status_t
BPartition::GetPath(BPath *path) const
{
	// not implemented
	return B_ERROR;
}

// GetVolume
/*!	\brief Returns a BVolume for the partition.

	The can succeed only, if the partition is mounted.

	\param volume Pointer to a pre-allocated BVolume, to be initialized to
		   represent the volume.
	\return \c B_OK, if the volume is mounted and the parameter could be set
			accordingly, another error code otherwise.
*/
status_t
BPartition::GetVolume(BVolume *volume) const
{
	status_t error = (fPartitionData && volume ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = volume->SetTo(fPartitionData->volume);
	return error;
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
BPartition::GetIcon(BBitmap *icon, icon_size which) const
{
/*
	status_t error = (icon ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (IsMounted()) {
			// mounted: get the icon from the volume
			BVolume volume;
			error = GetVolume(&volume);
			if (error == B_OK)
				error = volume.GetIcon(icon, which);
		} else {
			// not mounted: retrieve the icon ourselves
			if (BDiskDevice *device = Device()) {
				// get the icon
				if (error == B_OK)
					error = get_device_icon(device->Path(), icon, which);
			} else 
				error = B_ERROR;
		}
	}
	return error;
*/
	// not implemented
	return B_ERROR;
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
BPartition::GetMountPoint(BPath *mountPoint) const
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
	const char *volumeName = ContentName();
	if (volumeName || strlen(volumeName) == 0)
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
status_t
BPartition::Mount(const char *mountPoint, uint32 mountFlags,
	const char *parameters)
{
	if (!fPartitionData || IsMounted() || !ContainsFileSystem())
		return B_BAD_VALUE;

	// get the partition path
	BPath partitionPath;
	status_t error = GetPath(&partitionPath);
	if (error != B_OK)
		return error;

	// create a mount point, if none is given
	bool deleteMountPoint = false;
	BPath mountPointPath;
	if (!mountPoint) {
		// get a unique mount point
		error = GetMountPoint(&mountPointPath);
		if (error != B_OK)
			return error;

		mountPoint = mountPointPath.Path();

		// create the directory
		if (mkdir(mountPoint, S_IRWXU | S_IRWXG | S_IRWXO) < 0)
			return errno;

		deleteMountPoint = true;
	}

	// mount the partition
	error = fs_mount_volume(mountPoint, partitionPath.Path(), NULL, mountFlags,
		parameters);

	// delete the mount point on error, if we created it
	if (error != B_OK && deleteMountPoint)
		rmdir(mountPoint);

	return error;
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
	if (!fPartitionData || !IsMounted())
		return B_BAD_VALUE;

	// get the partition path
	BPath partitionPath;
	status_t error = GetPath(&partitionPath);
	if (error != B_OK)
		return error;

	// unmount
	return fs_unmount_volume(partitionPath.Path(), unmountFlags);
}

// Device
/*!	\brief Returns the device this partition resides on.
	\return The device this partition resides on.
*/
BDiskDevice *
BPartition::Device() const
{
	return fDevice;
}

// Parent
BPartition *
BPartition::Parent() const
{
	return fParent;
}

// ChildAt
BPartition *
BPartition::ChildAt(int32 index) const
{
	if (!fPartitionData || index < 0 || index >= fPartitionData->child_count)
		return NULL;
	return (BPartition*)fPartitionData->children[index]->user_data;
}

// CountChildren
int32
BPartition::CountChildren() const
{
	return (fPartitionData ? fPartitionData->child_count : 0);
}

// FindDescendant
BPartition *
BPartition::FindDescendant(partition_id id) const
{
	IDFinderVisitor visitor(id);
	return const_cast<BPartition*>(this)->VisitEachDescendant(&visitor);
}

// GetPartitioningInfo
status_t
BPartition::GetPartitioningInfo(BPartitioningInfo *info) const
{
	if (!info || !fPartitionData || !_IsShadow())
		return B_BAD_VALUE;
	return info->_SetTo(_ShadowID(), _ChangeCounter());
}

// VisitEachChild
BPartition *
BPartition::VisitEachChild(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		int32 level = _Level();
		for (int32 i = 0; BPartition *child = ChildAt(i); i++) {
			if (child->_AcceptVisitor(visitor, level))
				return child;
		}
	}
	return NULL;
}

// VisitEachDescendant
BPartition *
BPartition::VisitEachDescendant(BDiskDeviceVisitor *visitor)
{
	if (visitor)
		return _VisitEachDescendant(visitor);
	return NULL;
}

// CanDefragment
bool
BPartition::CanDefragment(bool *whileMounted) const
{
	return (fPartitionData && _IsShadow()
			&& _kern_supports_defragmenting_partition(_ShadowID(),
													  _ChangeCounter(),
													  whileMounted));
}

// Defragment
status_t
BPartition::Defragment() const
{
	if (!fPartitionData || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_defragment_partition(_ShadowID(), _ChangeCounter());
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanRepair
bool
BPartition::CanRepair(bool checkOnly, bool *whileMounted) const
{
	return (fPartitionData && _IsShadow()
			&& _kern_supports_repairing_partition(_ShadowID(), _ChangeCounter(),
												  checkOnly, whileMounted));
}

// Repair
status_t
BPartition::Repair(bool checkOnly) const
{
	if (!fPartitionData || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_repair_partition(_ShadowID(), _ChangeCounter(),
											checkOnly);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanResize
bool
BPartition::CanResize(bool *canResizeContents, bool *whileMounted) const
{
	return (fPartitionData && !IsDevice() && Parent() && _IsShadow()
			&& _kern_supports_resizing_partition(_ShadowID(), _ChangeCounter(),
					canResizeContents, whileMounted));
}

// ValidateResize
status_t
BPartition::ValidateResize(off_t *size) const
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow() || !size)
		return B_BAD_VALUE;
	return _kern_validate_resize_partition(_ShadowID(), _ChangeCounter(),
										   size);
}

// Resize
status_t
BPartition::Resize(off_t size)
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_resize_partition(_ShadowID(), _ChangeCounter(),
											size);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanMove
bool
BPartition::CanMove(BObjectList<BPartition> *unmovableDescendants,
					BObjectList<BPartition> *movableOnlyIfUnmounted) const
{
	// check parameters
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow())
		return false;
	// count descendants and allocate partition_id arrays large enough
	int32 descendantCount = _CountDescendants();
	partition_id *unmovableIDs = NULL;
	partition_id *needUnmountingIDs = NULL;
	AutoDeleter<partition_id> deleter1;
	AutoDeleter<partition_id> deleter2;
	if (descendantCount > 0) {
		// allocate arrays
		unmovableIDs = new(nothrow) partition_id[descendantCount];
		needUnmountingIDs = new(nothrow) partition_id[descendantCount];
		deleter1.SetTo(unmovableIDs, true);
		deleter2.SetTo(needUnmountingIDs, true);
		if (!unmovableIDs || !needUnmountingIDs)
			return false;
		// init arrays
		for (int32 i = 0; i < descendantCount; i++) {
			unmovableIDs[i] = -1;
			needUnmountingIDs[i] = -1;
		}
	}
	// get the info
	bool result = _kern_supports_moving_partition(_ShadowID(),
						_ChangeCounter(), unmovableIDs, needUnmountingIDs,
						descendantCount);
	if (result) {
		// get unmovable BPartition objects for returned IDs
		if (unmovableDescendants) {
			for (int32 i = 0; i < descendantCount && unmovableIDs[i] != -1;
				 i++) {
				BPartition *descendant = FindDescendant(unmovableIDs[i]);
				if (!descendant || !unmovableDescendants->AddItem(descendant))
					return false;
			}
		}
		// get BPartition objects needing to be unmounted for returned IDs
		if (movableOnlyIfUnmounted) {
			for (int32 i = 0;
				 i < descendantCount && needUnmountingIDs[i] != -1;
				 i++) {
				BPartition *descendant = FindDescendant(needUnmountingIDs[i]);
				if (!descendant || !movableOnlyIfUnmounted->AddItem(descendant))
					return false;
			}
		}
	}
	return result;
}

// ValidateMove
status_t
BPartition::ValidateMove(off_t *newOffset) const
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow()
		|| !newOffset) {
		return B_BAD_VALUE;
	}
	return _kern_validate_move_partition(_ShadowID(), _ChangeCounter(),
										 newOffset);
}

// Move
status_t
BPartition::Move(off_t newOffset)
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_resize_partition(_ShadowID(), _ChangeCounter(),
											newOffset);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanSetName
bool
BPartition::CanSetName() const
{
	return (fPartitionData && Parent() && _IsShadow()
			&& _kern_supports_setting_partition_name(_ShadowID(),
													 _ChangeCounter()));
}

// ValidateSetName
status_t
BPartition::ValidateSetName(char *name) const
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow() || !name)
		return B_BAD_VALUE;
	return _kern_validate_set_partition_name(_ShadowID(), _ChangeCounter(),
											 name);
}

// SetName
status_t
BPartition::SetName(const char *name)
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow() || !name)
		return B_BAD_VALUE;
	status_t error = _kern_set_partition_name(_ShadowID(), _ChangeCounter(),
											  name);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanSetContentName
bool
BPartition::CanSetContentName(bool *whileMounted) const
{
	return (fPartitionData && _IsShadow()
			&& _kern_supports_setting_partition_content_name(_ShadowID(),
															 _ChangeCounter(),
															 whileMounted));
}

// ValidateSetContentName
status_t
BPartition::ValidateSetContentName(char *name) const
{
	if (!fPartitionData || !_IsShadow() || !name)
		return B_BAD_VALUE;
	return _kern_validate_set_partition_content_name(_ShadowID(),
													 _ChangeCounter(), name);
}

// SetContentName
status_t
BPartition::SetContentName(const char *name)
{
	if (!fPartitionData || !_IsShadow() || !name)
		return B_BAD_VALUE;
	status_t error = _kern_set_partition_content_name(_ShadowID(),
													  _ChangeCounter(), name);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanSetType
bool
BPartition::CanSetType() const
{
	return (fPartitionData && Parent() && _IsShadow()
			&& _kern_supports_setting_partition_type(_ShadowID(),
													 _ChangeCounter()));
}

// ValidateSetType
status_t
BPartition::ValidateSetType(const char *type) const
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow() || !type)
		return B_BAD_VALUE;
	return _kern_validate_set_partition_type(_ShadowID(), _ChangeCounter(),
											 type);
}

// SetType
status_t
BPartition::SetType(const char *type)
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow() || !type)
		return B_BAD_VALUE;
	status_t error = _kern_set_partition_type(_ShadowID(), _ChangeCounter(),
											  type);
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanEditParameters
bool
BPartition::CanEditParameters() const
{
	return (fPartitionData && Parent() && _IsShadow()
			&& _kern_supports_setting_partition_parameters(_ShadowID(),
														   _ChangeCounter()));
}

// GetParameterEditor
status_t
BPartition::GetParameterEditor(BDiskDeviceParameterEditor **editor)
{
	// not implemented
	return B_ERROR;
}

// SetParameters
status_t
BPartition::SetParameters(const char *parameters)
{
	if (!fPartitionData || IsDevice() || !Parent() || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_set_partition_parameters(_ShadowID(),
													_ChangeCounter(),
													parameters,
													(parameters
													 ? strlen(parameters)+1
													 : 0));
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanEditContentParameters
bool
BPartition::CanEditContentParameters(bool *whileMounted) const
{
	return (fPartitionData && _IsShadow()
			&& _kern_supports_setting_partition_content_parameters(
					_ShadowID(), _ChangeCounter(), whileMounted));
}

// GetContentParameterEditor
status_t
BPartition::GetContentParameterEditor(BDiskDeviceParameterEditor **editor)
{
	// not implemented
	return B_ERROR;
}

// SetContentParameters
status_t
BPartition::SetContentParameters(const char *parameters)
{
	if (!fPartitionData || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_set_partition_content_parameters(_ShadowID(),
															_ChangeCounter(),
															parameters,
															(parameters
															 ? strlen(parameters)+1
															 : 0));
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanInitialize
bool
BPartition::CanInitialize(const char *diskSystem) const
{
	return (fPartitionData && diskSystem && _IsShadow()
			&& _kern_supports_initializing_partition(_ShadowID(),
													 _ChangeCounter(),
													 diskSystem));
}

// GetInitializationParameterEditor
status_t
BPartition::GetInitializationParameterEditor(const char *system,
	BDiskDeviceParameterEditor **editor) const
{
	// not implemented
	return B_ERROR;
}

// ValidateInitialize
status_t
BPartition::ValidateInitialize(const char *diskSystem, char *name,
							   const char *parameters)
{
	if (!fPartitionData || !_IsShadow() || !diskSystem || !name)
		return B_BAD_VALUE;
	return _kern_validate_initialize_partition(_ShadowID(), _ChangeCounter(),
											   diskSystem, name, parameters,
											   (parameters ? strlen(parameters)+1 : 0));
}

// Initialize
status_t
BPartition::Initialize(const char *diskSystem, const char *name,
					   const char *parameters)
{
	if (!fPartitionData || !_IsShadow() || !diskSystem || !name)
		return B_BAD_VALUE;
	status_t error = _kern_initialize_partition(_ShadowID(), _ChangeCounter(),
												diskSystem, name, parameters,
												(parameters ? strlen(parameters)+1 : 0));
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// Uninitialize
status_t
BPartition::Uninitialize()
{
	if (!fPartitionData || !_IsShadow())
		return B_BAD_VALUE;
	status_t error = _kern_uninitialize_partition(_ShadowID(),
												  _ChangeCounter());
	if (error == B_OK)
		error = Device()->Update();
	return error;
}

// CanCreateChild
bool
BPartition::CanCreateChild() const
{
	return (fPartitionData && _IsShadow()
			&& _kern_supports_creating_child_partition(_ShadowID(),
													   _ChangeCounter()));
}

// GetChildCreationParameterEditor
status_t
BPartition::GetChildCreationParameterEditor(const char *system,	
	BDiskDeviceParameterEditor **editor) const
{
	// not implemented
	return B_ERROR;
}

// ValidateCreateChild
status_t
BPartition::ValidateCreateChild(off_t *offset, off_t *size, const char *type,
								const char *parameters) const
{
	if (!fPartitionData || !_IsShadow() || !offset || !size || !type)
		return B_BAD_VALUE;
	return _kern_validate_create_child_partition(_ShadowID(), _ChangeCounter(),
												 offset, size, type,
												 parameters,
												 (parameters ? strlen(parameters)+1 : 0));
}

// CreateChild
status_t
BPartition::CreateChild(off_t offset, off_t size, const char *type,
						const char *parameters, BPartition **child)
{
	if (!fPartitionData || !_IsShadow() || !type)
		return B_BAD_VALUE;
	// send the request
	partition_id childID = -1;
	status_t error = _kern_create_child_partition(_ShadowID(),
							_ChangeCounter(), offset, size, type, parameters,
							(parameters ? strlen(parameters)+1 : 0), &childID);
	// update the device
	if (error == B_OK)
		error = Device()->Update();
	// find the newly created child
	if (error == B_OK && child) {
		*child = FindDescendant(childID);
		if (!*child)
			error = B_ERROR;
	}
	return error;
}

// CanDeleteChild
bool
BPartition::CanDeleteChild(int32 index) const
{
	BPartition *child = ChildAt(index);
	return (child && child->_IsShadow()
			&& _kern_supports_deleting_child_partition(child->_ShadowID(),
					child->_ChangeCounter()));
}

// DeleteChild
status_t
BPartition::DeleteChild(int32 index)
{
	BPartition *child = ChildAt(index);
	if (!fPartitionData || !_IsShadow() || !child)
		return B_BAD_VALUE;
	// send the request
	status_t error = _kern_delete_partition(child->_ShadowID(),
											child->_ChangeCounter());
	// update the device
	if (error == B_OK)
		error = Device()->Update();
	return error;
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
BPartition::_SetTo(BDiskDevice *device, BPartition *parent,
				   user_partition_data *data)
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
		BPartition *child = new(nothrow) BPartition;
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
			if (BPartition *child = ChildAt(i))
				delete child;
		}
		fPartitionData->user_data = NULL;
	}
	fDevice = NULL;
	fParent = NULL;
	fPartitionData = NULL;
}

// _RemoveObsoleteDescendants
status_t
BPartition::_RemoveObsoleteDescendants(user_partition_data *data,
									   bool *updated)
{
	// remove all children not longer persistent
	// Not exactly efficient: O(n^2), considering BList::RemoveItem()
	// O(1). We could do better (O(n*log(n))), when sorting the arrays before,
	// but then the list access is more random and we had to find the
	// BPartition to remove, which makes the list operation definitely O(n).
	int32 count = CountChildren();
	for (int32 i = count - 1; i >= 0; i--) {
		BPartition *child = ChildAt(i);
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
BPartition::_Update(user_partition_data *data, bool *updated)
{
	user_partition_data *oldData = fPartitionData;
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
		user_partition_data *childData = data->children[i];
		BPartition *child = (BPartition*)childData->user_data;
		if (child) {
			// old partition
			error = child->_Update(childData, updated);
			if (error != B_OK)
				return error;
		} else {
			// new partition
			*updated = true;
			child = new(nothrow) BPartition;
			if (child) {
				error = child->_SetTo(fDevice, this, data->children[i]);
				if (error != B_OK)
					delete child;
			} else
				return B_NO_MEMORY;
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

// _IsShadow
bool
BPartition::_IsShadow() const
{
	return (fPartitionData && fPartitionData->shadow_id >= 0);
}

// _ShadowID
partition_id
BPartition::_ShadowID() const
{
	return (fPartitionData ? fPartitionData->shadow_id : -1);
}

// _DiskSystem
disk_system_id
BPartition::_DiskSystem() const
{
	return (fPartitionData ? fPartitionData->disk_system : -1);
}

// _ChangeCounter
int32
BPartition::_ChangeCounter() const
{
	return (fPartitionData ? fPartitionData->change_counter : -1);
}

// _CountDescendants
int32
BPartition::_CountDescendants() const
{
	int32 count = 1;
	for (int32 i = 0; BPartition *child = ChildAt(i); i++)
		count += child->_CountDescendants();
	return count;
}

// _Level
int32
BPartition::_Level() const
{
	int32 level = 0;
	const BPartition *ancestor = this;
	while ((ancestor = ancestor->Parent()))
		level++;
	return level;
}

// _AcceptVisitor
bool
BPartition::_AcceptVisitor(BDiskDeviceVisitor *visitor, int32 level)
{
	return visitor->Visit(this, level);
}

// _VisitEachDescendant
BPartition *
BPartition::_VisitEachDescendant(BDiskDeviceVisitor *visitor, int32 level)
{
	if (level < 0)
		level = _Level();
	if (_AcceptVisitor(visitor, level))
		return this;
	for (int32 i = 0; BPartition *child = ChildAt(i); i++) {
		if (BPartition *result = child->_VisitEachDescendant(visitor,
															 level + 1)) {
			return result;
		}
	}
	return NULL;
}

