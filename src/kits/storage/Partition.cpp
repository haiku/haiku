//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <Partition.h>

/*!	\class BPartition
	\brief A BPartition object represent a partition and provides a lot of
		   methods to retrieve information about it and some to manipulate it.

	Not all BPartitions represent actual on-disk partitions. Some exist only
	to make all devices fit smoothly into the framework (e.g. for floppies,
	\see IsVirtual()), others represents merely partition slots
	(\see IsEmpty()).
*/

// Session
/*!	\brief Returns the session this partition resides on.
	\return The session this partition resides on.
*/
BSession *
BPartition::Session() const
{
	return NULL;	// not implemented
}

// Device
/*!	\brief Returns the device this partition resides on.
	\return The device this partition resides on.
*/
BDiskDevice *
BPartition::Device() const
{
	return NULL;	// not implemented
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
	return 0;	// not implemented
}

// Size
/*!	\brief Returns the size of the partition.
	\return The size of the partition in bytes.
*/
off_t
BPartition::Size() const
{
	return 0;	// not implemented
}

// BlockSize
/*!	\brief Returns the block size of the device.
	\return The block size of the device in bytes.
*/
int32
BPartition::BlockSize() const
{
	return 0;	// not implemented
}

// Index
/*!	\brief Returns the index of the partition in its session's list of
		   partitions.
	\return The index of the partition in its session's list of partitions.
*/
int32
BPartition::Index() const
{
	return -1;	// not implemented
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
	return 0;	// not implemented
}

// IsHidden
/*!	\brief Returns whether the partition can contain a file system.
	\see Flags().
	\return \c true, if the partition can't contain a file system, \c false
			otherwise.
*/
bool
BPartition::IsHidden() const
{
	return false;	// not implemented
}

// IsVirtual
/*!	\brief Returns whether the object doesn't represents an on-disk partition.
	\see Flags().
	\return \c true, if the object doesn't represent an on-disk partition,
			\c false otherwise.
*/
bool
BPartition::IsVirtual() const
{
	return false;	// not implemented
}

// IsEmpty
/*!	\brief Returns whether the partition is empty.
	\see Flags().
	\return \c true, if the partition is empty, \c false otherwise.
*/
bool
BPartition::IsEmpty() const
{
	return false;	// not implemented
}

// ContainsFileSystem
/*!	\brief Returns whether the partition contains a file system recognized by
		   the system.
	\return \c true, if the partition contains a file system recognized by
			the system, \c false otherwise.
*/
bool
BPartition::ContainsFileSystem() const
{
	return false;	// not implemented
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
	return NULL;	// not implemented
}

// Type
/*!	\brief Returns a human readable string for the type of the partition.
	\return A human readable string for the type of the partition.
*/
const char *
BPartition::Type() const
{
	return NULL;	// not implemented
}

// FileSystemShortName
/*!	\brief Returns a short string identifying the file system on the partition.

	If the partition doesn't contain a recognized file system
	(\see ContainsFileSystem()), \c NULL is returned.

	\return A short string identifying the file system on the partition,
			or \c NULL, if the partition doesn't contain a recognized file
			system.
*/
const char *
BPartition::FileSystemShortName() const
{
	return NULL;	// not implemented
}

// FileSystemLongName
/*!	\brief Returns a longer description of the file system on the partition.

	If the partition doesn't contain a recognized file system
	(\see ContainsFileSystem()), \c NULL is returned.

	\return A longer description of the file system on the partition,
			or \c NULL, if the partition doesn't contain a recognized file
			system.
*/
const char *
BPartition::FileSystemLongName() const
{
	return NULL;	// not implemented
}

// VolumeName
/*!	\brief Returns the name of the volume.

	If the partition doesn't contain a recognized file system
	(\see ContainsFileSystem()), \c NULL is returned.

	\return The name of the volume, or \c NULL, if the partition doesn't
			contain a recognized file system.
*/
const char *
BPartition::VolumeName() const
{
	return NULL;	// not implemented
}

// FileSystemFlags
/*!	\brief Returns the file system flags for the volume.

	If the partition doesn't contain a recognized file system
	(\see ContainsFileSystem()), the return value is undefined.

	Note, that, if the volume is mounted, the returned flags are identical
	with the ones fs_stat_dev() reports. If not mounted they describe merely
	the file system's capabilities. E.g. if the file system supports
	writing and the device is not read-only, the B_FS_IS_READONLY is not set,
	but it will be set, when the volume is mounted read-only. The same applies
	to other capabilities that can be disabled at mount time.

	\return The file system flags of the volume, if the partition contains
			a recognized file system.
*/
uint32
BPartition::FileSystemFlags() const
{
	return 0;	// not implemented
}

// IsMounted
/*!	\brief Returns whether the volume is mounted.
	\return \c true, if the volume is mounted, \c false otherwise.
*/
bool
BPartition::IsMounted() const
{
	return false;	// not implemented
}

// UniqueID
/*!	\brief Returns a unique identifier for this partition.

	The ID is not persistent, i.e. in general won't be the same after
	rebooting.

	\see BDiskDeviceRoster::GetPartitionWithID().

	\return A unique identifier for this partition.
*/
int32
BPartition::UniqueID() const
{
	return 0;	// not implemented
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
	return B_ERROR;	// not implemented
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
	return B_ERROR;	// not implemented
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
	\param dialogCenter The point at which to center the dialog. If omitted,
		   the dialog is displayed centered to the screen.
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
										BPoint dialogCenter,
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
	\param dialogCenter The point at which to center the dialog. If omitted,
		   the dialog is displayed centered to the screen.
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
BPartition::Initialize(const char *fileSystem, BPoint dialogCenter,
					   bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// GetFileSystemList
/*!	\brief Returns a list of all file systems that can be used for
		   initialization.

	The names of the file systems are added as BString objects to the
	supplied list. The caller takes over ownership of the return BString
	objects and is responsible for deleteing them.

	Any of returned names can be passed to Initialize().

	\param list Pointer to a pre-allocated BObjectList the names of the
		   file systems shall be added to.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartition::GetFileSystemList(BObjectList<BString> *list)
{
	return B_ERROR;	// not implemented
}

// constructor
/*!	\brief Creates an uninitialized BPartition object.
*/
BPartition::BPartition()
{
}

// constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BPartition::BPartition(const BPartition &)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BPartition::~BPartition()
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

