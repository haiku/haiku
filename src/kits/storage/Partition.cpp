//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new>

#include <Partition.h>

#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceVisitor.h>
#include <Message.h>
#include <Volume.h>

#include "ddm_userland_interface.h"

/*!	\class BPartition
	\brief A BPartition object represent a partition and provides a lot of
		   methods to retrieve information about it and some to manipulate it.

	Not all BPartitions represent actual on-disk partitions. Some exist only
	to make all devices fit smoothly into the framework (e.g. for floppies,
	\see IsVirtual()), others represents merely partition slots
	(\see IsEmpty()).
*/

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
	// not implemented
	return B_ERROR;
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

// Mount
/*!	\brief Mounts the volume.

	The volume can only be mounted, if the partition contains a recognized
	file system (\see ContainsFileSystem()) and it is not already mounted.

	\param mountFlags Currently only \c B_MOUNT_READ_ONLY is defined, which
		   forces the volume to be mounted read-only.
	\param parameters File system specific mount parameters.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartition::Mount(uint32 mountFlags, const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Unmount
/*!	\brief Unmounts the volume.

	The volume can of course only be unmounted, if it currently is mounted.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartition::Unmount()
{
	return B_ERROR;	// not implemented
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
	// not implemented
	return B_ERROR;
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

// _IsShadow
bool
BPartition::_IsShadow() const
{
	return (fPartitionData && fPartitionData->shadow_id >= 0);
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


#if 0

// GetInitializationParameters
/*!	\brief Asks the user to set the parameters for initializing this partition.

	A dialog window will be opened, centered at \a dialogCenter, asking the
	user for setting the parameters to be used for initializing this partition
	with the file system specified by \a fileSystem.

	The method does not return until the user has confirmed or cancelled the
	dialog. In the latter case \a cancelled is set to \c true, otherwise to
	\c false.

	\param fileSystem The file system the parameters shall be asked for.
	\param volumeName The volume name set by the user.
	\param parameters Pointer to a pre-allocated BString to be set to the
		   parameters the user has specified.
	\param dialogCenter The rectangle over which to center the dialog. If
		   omitted, the dialog is displayed centered to the screen.
	\param cancelled Pointer to a pre-allocated bool to be set to \c true, if
		   the dialog has been cancelled by the user, or to \c false
		   otherwise. May be \c NULL.
	\return
	- \c B_OK: The parameters have been retrieved successfully.
	- \c B_ERROR: Either the dialog has been cancelled -- then \a cancelled
	  is set accordingly -- or some other error occured.
	- another error code
*/
status_t
BPartition::GetInitializationParameters(const char *fileSystem,
										BString *volumeName,
										BString *parameters,
										BRect dialogCenter,
										bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// Initialize
/*!	\brief Initializes the partition according to the supplied parameters.
	\param fileSystem The file system to be used for initialization.
	\param volumeName The new volume name.
	\param parameters File system specific parameters.
	\return
	- \c B_OK: Everything went fine.
	- another error code, if an error occured
*/
status_t
BPartition::Initialize(const char *fileSystem, const char *volumeName,
					   const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Initialize
/*!	\brief Initializes the partition after asking the user for the respective
		   parameters.

	A dialog window will be opened, centered at \a dialogCenter, asking the
	user for setting the parameters to be used for initializing this partition
	with the file system specified by \a fileSystem.

	The method does not return until the user has either confirmed the dialog
	and the initialization is done, or the dialog has been cancelled. In the
	latter case \a cancelled is set to \c true, otherwise to \c false.

	\param fileSystem The file system to be used for the initialization.
	\param dialogCenter The rectangle over which to center the dialog. If
		   omitted, the dialog is displayed centered to the screen.
	\param cancelled Pointer to a pre-allocated bool to be set to \c true, if
		   the dialog has been cancelled by the user, or to \c false
		   otherwise. May be \c NULL.
	\return
	- \c B_OK: The parameters have been retrieved successfully and the
	  initialization went fine, too.
	- \c B_ERROR: Either the dialog has been cancelled -- then \a cancelled
	  is set accordingly -- or some other error occured.
	- another error code
*/
status_t
BPartition::Initialize(const char *fileSystem, BRect dialogCenter,
					   bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// constructor
/*!	\brief Creates an uninitialized BPartition object.
*/
BPartition::BPartition()
	: fSession(NULL),
	  fUniqueID(-1),
	  fChangeCounter(0),
	  fVolumeID(-1)
{
	fInfo.info.offset = 0;
	fInfo.info.size = 0;
	fInfo.flags = 0;
	fInfo.partition_name[0] = '\0';
	fInfo.partition_type[0] = '\0';
	fInfo.file_system_short_name[0] = '\0';
	fInfo.file_system_long_name[0] = '\0';
	fInfo.volume_name[0] = '\0';
	fInfo.file_system_flags = 0;
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

// _Unset
void
BPartition::_Unset()
{
	fSession = NULL;
	fUniqueID = -1;
	fChangeCounter = 0;
	fIndex = -1;
	fInfo.info.offset = 0;
	fInfo.info.size = 0;
	fInfo.flags = 0;
	fInfo.partition_name[0] = '\0';
	fInfo.partition_type[0] = '\0';
	fInfo.file_system_short_name[0] = '\0';
	fInfo.file_system_long_name[0] = '\0';
	fInfo.volume_name[0] = '\0';
	fInfo.file_system_flags = 0;
	fVolumeID = -1;
}

// find_string
static
status_t
find_string(BMessage *message, const char *name, char *buffer)
{
	const char *str;
	status_t error = message->FindString(name, &str);
	if (error == B_OK)
		strcpy(buffer, str);
	return error;
}

// _Unarchive
status_t
BPartition::_Unarchive(BMessage *archive)
{
//printf("BPartition::_Unarchive()\n");
	_Unset();
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// ID, change counter and index
		if (error == B_OK)
			error = archive->FindInt32("id", &fUniqueID);
		if (error == B_OK)
			error = archive->FindInt32("change_counter", &fChangeCounter);
		if (error == B_OK)
			error = archive->FindInt32("index", &fIndex);
//printf("  check: %s\n", strerror(error));
		// fInfo.info.*
		if (error == B_OK)
			error = archive->FindInt64("offset", &fInfo.info.offset);
		if (error == B_OK)
			error = archive->FindInt64("size", &fInfo.info.size);
//printf("  check: %s\n", strerror(error));
		// fInfo.*
		if (error == B_OK)
			error = archive->FindInt32("flags", (int32*)&fInfo.flags);
		if (error == B_OK)
			error = find_string(archive, "name", fInfo.partition_name);
		if (error == B_OK)
			error = find_string(archive, "type", fInfo.partition_type);
		if (error == B_OK) {
			error = find_string(archive, "fs_short_name",
								fInfo.file_system_short_name);
		}
		if (error == B_OK) {
			error = find_string(archive, "fs_long_name",
								fInfo.file_system_long_name);
		}
		if (error == B_OK)
			error = find_string(archive, "volume_name", fInfo.volume_name);
		if (error == B_OK) {
			error = archive->FindInt32("fs_flags",
									   (int32*)&fInfo.file_system_flags);
		}
//printf("  check: %s\n", strerror(error));
		// dev_t, if mounted
		if (error == B_OK) {
			if (archive->FindInt32("volume_id", &fVolumeID) != B_OK)
				fVolumeID = -1;
		}
	}
	// cleanup on error
	if (error != B_OK)
		_Unset();
//printf("BPartition::_Unarchive() done: %s\n", strerror(error));
	return error;
}

// _SetSession
void
BPartition::_SetSession(BSession *session)
{
	fSession = session;
}

#endif	// 0
