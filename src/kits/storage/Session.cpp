//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <Session.h>

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

// Device
/*!	\brief Returns the device this session resides on.
	\return The device this session resides on.
*/
BDiskDevice *
BSession::Device() const
{
	return NULL;	// not implemented
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
	return 0;	// not implemented
}

// Size
/*!	\brief Returns the size of the session.
	\return The size of the session in bytes.
*/
off_t
BSession::Size() const
{
	return 0;	// not implemented
}

// CountPartitions
/*!	\brief Returns the number of partitions on this session.
	\return The number of partitions on this session.
*/
int32
BSession::CountPartitions() const
{
	return 0;	// not implemented
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
	return NULL;	// not implemented
}

// Index
/*!	\brief Returns the index of the session in its device's list of session.
	\return The index of the session in its session's list of sessions.
*/
int32
BSession::Index() const
{
	return -1;	// not implemented
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
	return 0;	// not implemented
}

// IsAudio
/*!	\brief Returns whether the session is an audio session.
	\return \c true, if the session is an audio session, \c false otherwise.
*/
bool
BSession::IsAudio() const
{
	return false;	// not implemented
}

// IsData
/*!	\brief Returns whether the session is a data (i.e. non-audio) session.
	\return \c true, if the session is a data session, \c false otherwise.
*/
bool
BSession::IsData() const
{
	return false;	// not implemented
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
	return false;	// not implemented
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
	return NULL;	// not implemented
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
	return 0;	// not implemented
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
	return NULL;	// not implemented
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
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BSession::~BSession()
{
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
	return *this;	// not implemented
}

