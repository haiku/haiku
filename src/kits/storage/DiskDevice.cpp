//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevice.h>

/*!	\class BDiskDevice
	\brief A BDiskDevice object represents a storage device.
*/

// constructor
/*!	\brief Creates an uninitialized BDiskDevice object.
*/
BDiskDevice::BDiskDevice()
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskDevice::~BDiskDevice()
{
}

// Size
/*!	\brief Returns the size of the device.
	\return The size of the device in bytes.
*/
off_t
BDiskDevice::Size() const
{
	return 0;	// not implemented
}

// BlockSize
/*!	\brief Returns the block size of the device.
	\return The block size of the device in bytes.
*/
int32
BDiskDevice::BlockSize() const
{
	return 0;	// not implemented
}

// CountSessions
/*!	\brief Returns the number of sessions on this device.
	\return The number of sessions on this device.
*/
int32
BDiskDevice::CountSessions() const
{
	return 0;	// not implemented
}

// SessionAt
/*!	\brief Returns a contained session by index.
	\param index The index of the session to be returned.
	\return The session with the requested index, or \c NULL, if \a index
			is out of range.
*/
BSession *
BDiskDevice::SessionAt(int32 index) const
{
	return NULL;	// not implemented
}

// CountPartitions
/*!	\brief Returns the number of partitions on this device.
	\return The number of partitions on this device.
*/
int32
BDiskDevice::CountPartitions() const
{
	return 0;	// not implemented
}

// DevicePath
/*!	\brief Returns the path to the device.
	\return The path to the device.
*/
const char *
BDiskDevice::DevicePath() const
{
	return NULL;	// not implemented
}

// GetName
/*!	\brief Returns a human readable name for the device.

	The method tries to interpret the device path, which it can do only for
	floppy, IDE and SCSI devices, for others the device path is returned.

	\param name Pointer to a pre-allocated BString to be set to the device
		   name.
	\param includeBusID \c true, if the bus ID shall be included in the name
		   to be returned.
	\param includeLUN \c true, if the LUN shall be included in the name
		   to be returned.
	\return A human readable name for the device.
*/
void
BDiskDevice::GetName(BString *name, bool includeBusID, bool includeLUN) const
{
}

// GetName
/*!	\brief Returns a human readable name for the device.

	The method tries to interpret the device path, which it can do only for
	floppy, IDE and SCSI devices, for others the device path is returned.

	\param name Pointer to a pre-allocated char buffer into which the device
		   name shall be copied.
	\param includeBusID \c true, if the bus ID shall be included in the name
		   to be returned.
	\param includeLUN \c true, if the LUN shall be included in the name
		   to be returned.
	\return A human readable name for the device.
*/
void
BDiskDevice::GetName(char *name, bool includeBusID, bool includeLUN) const
{
}

// IsReadOnly
/*!	\brief Returns whether the device is read-only.
	\return \c true, if the device is read-only, \c false otherwise.
*/
bool
BDiskDevice::IsReadOnly() const
{
	return false;	// not implemented
}

// IsRemovable
/*!	\brief Returns whether the device media are removable.
	\return \c true, if the device media are removable, \c false otherwise.
*/
bool
BDiskDevice::IsRemovable() const
{
	return false;	// not implemented
}

// HasMedia
/*!	\brief Returns whether the device contains a media.
	\return \c true, if the device contains a media, \c false otherwise.
*/
bool
BDiskDevice::HasMedia() const
{
	return false;	// not implemented
}

// IsFloppy
/*!	\brief Returns whether the device is a floppy device.
	\return \c true, if the device is a floppy device, \c false otherwise.
*/
bool
BDiskDevice::IsFloppy() const
{
	return false;	// not implemented
}

// Type
/*!	\brief Returns the type of the device.

	The type may be one of the following (note, that not all types make sense
	for storage device, but they are listed anyway):
	- \c B_DISK: Hard disks, floppy disks, etc.
	- \c B_TAPE: Tape drives.
	- \c B_PRINTER: Printers.
	- \c B_CPU: CPU devices.
	- \c B_WORM: Write-once, read-many devives.
	- \c B_CD: CD ROMs.
	- \c B_SCANNER: Scanners.
	- \c B_OPTICAL: Optical devices.
	- \c B_JUKEBOX: Jukeboxes.
	- \c B_NETWORK: Network devices.
	\return The type of the device.
*/
uint8
BDiskDevice::Type() const
{
	return 0;	// not implemented
}

// UniqueID
/*!	\brief Returns a unique identifier for this device.

	The ID is not persistent, i.e. in general won't be the same after
	rebooting.

	\see BDiskDeviceRoster::GetDeviceWithID().

	\return A unique identifier for this device.
*/
int32
BDiskDevice::UniqueID() const
{
	return -1;	// not implemented
}

// Eject
/*!	\brief Eject the device's media.

	The device media must, of course, be removable, and the device must
	support ejecting the media.

	\return \c B_OK, if the media are ejected successfully, another error code
		    otherwise.
*/
status_t
BDiskDevice::Eject()
{
	return B_ERROR;	// not implemented
}

// LowLevelFormat
/*!	\brief Low level formats the device.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDevice::LowLevelFormat()
{
	return B_ERROR;	// not implemented
}

// Update
/*!	\brief Updates the object to reflect the latest changes to the device.

	Note, that subobjects (BSessions, BPartitions) may be deleted during this
	operation. It is also possible, that the device doesn't exist anymore --
	e.g. if it is hot-pluggable. Then an error is returned and the object is
	uninitialized.

	\return \c B_OK, if the update went fine, another error code otherwise.
*/
status_t
BDiskDevice::Update()
{
	return B_ERROR;	// not implemented
}

// VisitEachSession
/*!	\brief Iterates through the device's sessions.

	The supplied visitor's Visit(BSession*) is invoked for each session.
	If Visit() returns \c true, the iteration is terminated and the BSession
	object just visited is returned.

	\param visitor The visitor.
	\return The BSession object at which the iteration was terminated, or
			\c NULL, if the iteration has not been terminated.
*/
BSession *
BDiskDevice::VisitEachSession(BDiskDeviceVisitor *visitor)
{
	return NULL;	// not implemented
}

// VisitEachPartition
/*!	\brief Iterates through the device's partitions.

	The supplied visitor's Visit(BPartition*) is invoked for each partition.
	If Visit() returns \c true, the iteration is terminated and the BPartition
	object just visited is returned.

	\param visitor The visitor.
	\return The BPartition object at which the iteration was terminated, or
			\c NULL, if the iteration has not been terminated.
*/
BPartition *
BDiskDevice::VisitEachPartition(BDiskDeviceVisitor *visitor)
{
	return NULL;	// not implemented
}

// Traverse
/*!	\brief Pre-order traverses the tree of the spanned by the BDiskDevice and
		   its subobjects.

	The supplied visitor's Visit() is invoked for the device object itself, 
	for each session and for each partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true as well.

	\param visitor The visitor.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDevice::Traverse(BDiskDeviceVisitor *visitor)
{
	return false;	// not implemented
}

// copy constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BDiskDevice::BDiskDevice(const BDiskDevice &)
{
}

// =
/*!	\brief Privatized assignment operator to avoid usage.
*/
BDiskDevice &
BDiskDevice::operator=(const BDiskDevice &)
{
	return *this;
}

