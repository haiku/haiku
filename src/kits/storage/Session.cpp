//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new.h>

#include <Session.h>
#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <Message.h>
#include <Partition.h>

/*!	\class BSession
	\brief A BSession object represent a session and provides a lot of
		   methods to retrieve information about it and some to manipulate it.

	Not all BSession represent actual on-disk sessions. Some exist only
	to make all devices fit smoothly into the framework (e.g. for hard and
	floppy, \see IsVirtual()).
*/


/*!	\brief Name of the DOS/Intel partitioning system.
*/
const char *B_INTEL_PARTITIONING = "intel";

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BSession::~BSession()
{
}

// Device
/*!	\brief Returns the device this session resides on.
	\return The device this session resides on.
*/
BDiskDevice *
BSession::Device() const
{
	return fDevice;
}

// Offset
/*!	\brief Returns the session's offset relative to the beginning of the
		   device it resides on.
	\return The session's offset in bytes relative to the beginning of the
			device it resides on.
*/
off_t
BSession::Offset() const
{
	return fInfo.offset;
}

// Size
/*!	\brief Returns the size of the session.
	\return The size of the session in bytes.
*/
off_t
BSession::Size() const
{
	return fInfo.size;
}

// BlockSize
/*!	\brief Returns the block size of the device.
	\return The block size of the device in bytes.
*/
int32
BSession::BlockSize() const
{
	return (fDevice ? fDevice->BlockSize() : 0);
}

// CountPartitions
/*!	\brief Returns the number of partitions on this session.
	\return The number of partitions on this session.
*/
int32
BSession::CountPartitions() const
{
	return fPartitions.CountItems();
}

// PartitionAt
/*!	\brief Returns a contained partition by index.
	\param index The index of the partition to be returned.
	\return The partition with the requested index, or \c NULL, if \a index
			is out of range.
*/
BPartition *
BSession::PartitionAt(int32 index) const
{
	return fPartitions.ItemAt(index);
}

// Index
/*!	\brief Returns the index of the session in its device's list of session.
	\return The index of the session in its session's list of sessions.
*/
int32
BSession::Index() const
{
	return fIndex;
}

// Flags
/*!	\brief Returns the flags for this session.

	The session flags are a bitwise combination of:
	- \c B_DATA_SESSION: The session is a non-audio session.
	- \c B_VIRTUAL_SESSION: There exists no on-disk session this object
	  represents. E.g. for hard disks there will be a BSession object spanning
	  the whole disk.

	\return The flags for this partition.
*/
uint32
BSession::Flags() const
{
	return fInfo.flags;
}

// IsAudio
/*!	\brief Returns whether the session is an audio session.
	\return \c true, if the session is an audio session, \c false otherwise.
*/
bool
BSession::IsAudio() const
{
	return !IsData();
}

// IsData
/*!	\brief Returns whether the session is a data (i.e. non-audio) session.
	\return \c true, if the session is a data session, \c false otherwise.
*/
bool
BSession::IsData() const
{
	return (fInfo.flags & B_DATA_SESSION);
}

// IsVirtual
/*!	\brief Returns whether the object doesn't represents an on-disk session.
	\see Flags().
	\return \c true, if the object doesn't represent an on-disk session,
			\c false otherwise.
*/
bool
BSession::IsVirtual() const
{
	return (fInfo.flags & B_VIRTUAL_SESSION);
}

// PartitioningSystem
/*!	\brief Returns the name of the partitioning system used for this session.

	If the session is a audio session or virtual (\see Flags()), the method
	returns \c NULL, otherwise the name of the partitioning system used.

	\return The name of the partitioning system used for this session, if any.
			\c NULL otherwise.
*/
const char *
BSession::PartitioningSystem() const
{
	return (fPartitioningSystem.Length() > 0
			? fPartitioningSystem.String() : NULL);
}

// UniqueID
/*!	\brief Returns a unique identifier for this session.

	The ID is not persistent, i.e. in general won't be the same after
	rebooting.

	\see BDiskDeviceRoster::GetSessionWithID().

	\return A unique identifier for this session.
*/
int32
BSession::UniqueID() const
{
	return fUniqueID;
}

// VisitEachPartition
/*!	\brief Iterates through the session's partitions.

	The supplied visitor's Visit(BPartition*) is invoked for each partition.
	If Visit() returns \c true, the iteration is terminated and the BPartition
	object just visited is returned.

	\param visitor The visitor.
	\return The BPartition object at which the iteration was terminated, or
			\c NULL, if the iteration has not been terminated.
*/
BPartition *
BSession::VisitEachPartition(BDiskDeviceVisitor *visitor)
{
	if (visitor) {
		for (int32 i = 0; BPartition *partition = PartitionAt(i); i++) {
			if (visitor->Visit(partition))
				return partition;
		}
	}
	return NULL;
}

// PartitionWithID
/*!	\brief Returns the partition on the session, that has a certain ID.
	\param id The ID of the partition to be returned.
	\return The partition with ID \a id, or \c NULL, if a partition with that
			ID does not exist on this session.
*/
BPartition *
BSession::PartitionWithID(int32 id)
{
	IDFinderVisitor visitor(id);
	return VisitEachPartition(&visitor);
}

// GetPartitioningParameters
/*!	\brief Asks the user to set the parameters for partitioning this session.

	A dialog window will be opened, centered at \a dialogCenter, asking the
	user for setting the parameters to be used for partitioning this session
	with the partitioning system specified by \a partitioningSystem.

	The method does not return until the user has confirmed or cancelled the
	dialog. In the latter case \a cancelled is set to \c true, otherwise to
	\c false.

	\param partitioningSystem The partitioning system the parameters shall be
		   asked for.
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
BSession::GetPartitioningParameters(const char *partitioningSystem,
									BString *parameters, BPoint dialogCenter,
									bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// Partition
/*!	\brief Partitions the session according to the supplied parameters.
	\param partitioningSystem The partitioning system to be used for
		   partitioning.
	\param parameters Partitioning system specific parameters.
	\return
	- \c B_OK: Everything went fine.
	- another error code, if an error occured
*/
status_t
BSession::Partition(const char *partitioningSystem, const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Partition
/*!	\brief Partitions the session after asking the user for the respective
		   parameters.

	A dialog window will be opened, centered at \a dialogCenter, asking the
	user for setting the parameters to be used for partitioning this session
	with the partitioning system specified by \a partitioningSystem.

	The method does not return until the user has either confirmed the dialog
	and the partitioning is done, or the dialog has been cancelled. In the
	latter case \a cancelled is set to \c true, otherwise to \c false.

	\param partitioningSystem The partitioning system to be used for
		   partitioning.
	\param dialogCenter The point at which to center the dialog. If omitted,
		   the dialog is displayed centered to the screen.
	\param cancelled Pointer to a pre-allocated bool to be set to \c true, if
		   the dialog has been cancelled by the user, or to \c false
		   otherwise. May be \c NULL.
	\return
	- \c B_OK: The parameters have been retrieved successfully and the
	  partitioning went fine, too.
	- \c B_ERROR: Either the dialog has been cancelled -- then \a cancelled
	  is set accordingly -- or some other error occured.
	- another error code
*/
status_t
BSession::Partition(const char *partitioningSystem, BPoint dialogCenter,
					bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// GetPartitioningSystemList
/*!	\brief Returns a list of all partitioning systems that can be used for
		   partitioning.

	The names of the partioning systems are added as BString objects to the
	supplied list. The caller takes over ownership of the return BString
	objects and is responsible for deleteing them.

	Any of returned names can be passed to Partition().

	\param list Pointer to a pre-allocated BObjectList the names of the
		   partitioning systems shall be added to.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BSession::GetPartitioningSystemList(BObjectList<BString> *list)
{
	return B_ERROR;	// not implemented
}

// constructor
/*!	\brief Creates an uninitialized BSession object.
*/
BSession::BSession()
	: fDevice(NULL),
	  fPartitions(10, true),
	  fUniqueID(-1),
	  fChangeCounter(0),
	  fIndex(-1),
	  fPartitioningSystem()
{
	fInfo.offset = 0;
	fInfo.size = 0;
	fInfo.flags = 0;
}

// copy constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BSession::BSession(const BSession &)
{
}

// =
/*!	\brief Privatized assignment operator to avoid usage.
*/
BSession &
BSession::operator=(const BSession &)
{
	return *this;
}

// _Unset
void
BSession::_Unset()
{
	fDevice = NULL;
	fPartitions.MakeEmpty();
	fUniqueID = -1;
	fChangeCounter = 0;
	fIndex = -1;
	fInfo.offset = 0;
	fInfo.size = 0;
	fInfo.flags = 0;
	fPartitioningSystem.SetTo("");
}

// _Unarchive
status_t
BSession::_Unarchive(BMessage *archive)
{
//printf("BSession::_Unarchive()\n");
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
		// fInfo.*
		if (error == B_OK)
			error = archive->FindInt64("offset", &fInfo.offset);
		if (error == B_OK)
			error = archive->FindInt64("size", &fInfo.size);
		if (error == B_OK)
			error = archive->FindInt32("flags", (int32*)&fInfo.flags);
//printf("  check: %s\n", strerror(error));
		// other data
		if (error == B_OK)
			error = archive->FindString("partitioning", &fPartitioningSystem);
//printf("  check: %s\n", strerror(error));
		// partitions
		type_code fieldType;
		int32 count = 0;
		if (error == B_OK) {
			if (archive->GetInfo("partitions", &fieldType, &count) != B_OK)
				count = 0;
		}
		for (int32 i = 0; error == B_OK && i < count; i++) {
			// get the archived partitions
			BMessage partitionArchive;
			error = archive->FindMessage("partitions", i, &partitionArchive);
			// allocate a partition object
			BPartition *partition = NULL;
			if (error == B_OK) {
				partition = new(nothrow) BPartition;
				if (!partition)
					error = B_NO_MEMORY;
			}
			// unarchive the partition
			if (error == B_OK)
				error = partition->_Unarchive(&partitionArchive);
			// add the partition
			if (error == B_OK && !_AddPartition(partition))
				error = B_NO_MEMORY;
			// cleanup on error
			if (error != B_OK && partition)
				delete partition;
		}
	}
	// cleanup on error
	if (error != B_OK)
		_Unset();
//printf("BSession::_Unarchive() done: %s\n", strerror(error));
	return error;
}

// _SetDevice
void
BSession::_SetDevice(BDiskDevice *device)
{
	fDevice = device;
}

// _AddPartition
bool
BSession::_AddPartition(BPartition *partition)
{
	bool result = fPartitions.AddItem(partition);
	if (result)
		partition->_SetSession(this);
	return result;
}

