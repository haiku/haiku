//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new.h>

#include <DiskDeviceRoster.h>
#include <DiskDevice.h>
#include <Message.h>
#include <Partition.h>
#include <RegistrarDefs.h>
#include <Session.h>

/*!	\class BDiskDeviceRoster
	\brief An interface for iterating through the disk devices known to the
		   system and for a notification mechanism provided to listen to their
		   changes.
*/

// constructor
/*!	\brief Creates a BDiskDeviceRoster object.

	The object is ready to be used after construction.
*/
BDiskDeviceRoster::BDiskDeviceRoster()
	: fManager(),
	  fCookie(0)
{
	BMessage request(B_REG_GET_DISK_DEVICE_MESSENGER);
	BMessage reply;
	if (_send_to_roster_(&request, &reply, false) == B_OK
		&& reply.what == B_REG_SUCCESS) {
		reply.FindMessenger("messenger", &fManager);
	}
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
BDiskDeviceRoster::~BDiskDeviceRoster()
{
}

// GetNextDevice
/*!	\brief Returns the next BDiskDevice.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized to
		   represent the next device.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: The end of the list of devices had already been
	  reached.
	- another error code
*/
status_t
BDiskDeviceRoster::GetNextDevice(BDiskDevice *device)
{
	status_t error = (device ? B_OK : B_BAD_VALUE);
	// compose request message
	BMessage request(B_REG_NEXT_DISK_DEVICE);
	if (error == B_OK)
		error = request.AddInt32("cookie", fCookie);
	// send request
	BMessage reply;
	if (error == B_OK)
		error = fManager.SendMessage(&request, &reply);
	// analyze reply
	if (error == B_OK) {
		// result
		status_t result = B_OK;
		error = reply.FindInt32("result", &result);
		if (error == B_OK)
			error = result;
		// cookie
		if (error == B_OK)
			error = reply.FindInt32("cookie", &fCookie);
		// device
		BMessage archive;
		if (error == B_OK)
			error = reply.FindMessage("device", &archive);
		if (error == B_OK)
			error = device->_Unarchive(&archive);
	}
	return error;
}

// Rewind
/*!	\brief Rewinds the device list iterator.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::Rewind()
{
	fCookie = 0;
	return B_OK;
}

// VisitEachDevice
/*!	\brief Iterates through the all devices.

	The supplied visitor's Visit(BDiskDevice*) is invoked for each device.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true. If supplied, \a device is set to the concerned device.

	\param visitor The visitor.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device at which the iteration was terminated.
		   May be \c NULL.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::VisitEachDevice(BDiskDeviceVisitor *visitor,
								   BDiskDevice *device)
{
	return false;	// not implemented
}

// VisitEachPartition
/*!	\brief Iterates through the all devices' partitions.

	The supplied visitor's Visit(BPartition*) is invoked for each partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true. If supplied, \a device is set to the concerned device
	and in \a partition the pointer to the partition object is returned.

	\param visitor The visitor.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device at which the iteration was terminated.
		   May be \c NULL.
	\param partition Pointer to a pre-allocated BPartition pointer to be set
		   to the partition at which the iteration was terminated.
		   May be \c NULL.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::VisitEachPartition(BDiskDeviceVisitor *visitor,
									  BDiskDevice *device,
									  BPartition **partition)
{
	return false;	// not implemented
}

// Traverse
/*!	\brief Pre-order traverses the tree of the spanned by the BDiskDevices and
		   their subobjects.

	The supplied visitor's Visit() is invoked for each device, for each
	session and for each partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true as well.

	\param visitor The visitor.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::Traverse(BDiskDeviceVisitor *visitor)
{
	return false;	// not implemented
}

// VisitEachMountedPartition
/*!	\brief Iterates through the all devices' partitions that are mounted.

	The supplied visitor's Visit(BPartition*) is invoked for each mounted
	partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true. If supplied, \a device is set to the concerned device
	and in \a partition the pointer to the partition object is returned.

	\param visitor The visitor.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device at which the iteration was terminated.
		   May be \c NULL.
	\param partition Pointer to a pre-allocated BPartition pointer to be set
		   to the partition at which the iteration was terminated.
		   May be \c NULL.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::VisitEachMountedPartition(BDiskDeviceVisitor *visitor,
											 BDiskDevice *device,
											 BPartition **partition)
{
	return false;	// not implemented
}

// VisitEachMountablePartition
/*!	\brief Iterates through the all devices' partitions that are mountable.

	The supplied visitor's Visit(BPartition*) is invoked for each mountable
	partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true. If supplied, \a device is set to the concerned device
	and in \a partition the pointer to the partition object is returned.

	\param visitor The visitor.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device at which the iteration was terminated.
		   May be \c NULL.
	\param partition Pointer to a pre-allocated BPartition pointer to be set
		   to the partition at which the iteration was terminated.
		   May be \c NULL.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::VisitEachMountablePartition(BDiskDeviceVisitor *visitor,
											   BDiskDevice *device,
											   BPartition **partition)
{
	return false;	// not implemented
}

// VisitEachInitializablePartition
/*!	\brief Iterates through the all devices' partitions that are initializable.

	The supplied visitor's Visit(BPartition*) is invoked for each
	initializable partition.
	If Visit() returns \c true, the iteration is terminated and this method
	returns \c true. If supplied, \a device is set to the concerned device
	and in \a partition the pointer to the partition object is returned.

	\param visitor The visitor.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device at which the iteration was terminated.
		   May be \c NULL.
	\param partition Pointer to a pre-allocated BPartition pointer to be set
		   to the partition at which the iteration was terminated.
		   May be \c NULL.
	\return \c true, if the iteration was terminated, \c false otherwise.
*/
bool
BDiskDeviceRoster::VisitEachInitializablePartition(BDiskDeviceVisitor *visitor,
												   BDiskDevice *device,
												   BPartition **partition)
{
	return false;	// not implemented
}

// GetDeviceWithID
/*!	\brief Returns a BDiskDevice for a given ID.

	The supplied \a device is initialized to the device identified by \a id.

	\param id The ID of the device to be retrieved.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device identified by \a id.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: A device with ID \a id could not be found.
	- other error codes
*/
status_t
BDiskDeviceRoster::GetDeviceWithID(int32 id, BDiskDevice *device) const
{
	return B_ERROR;	// not implemented
}

// GetSessionWithID
/*!	\brief Returns a BSession for a given ID.

	The supplied \a device is initialized to the device the session identified
	by \a id resides on, and \a session is set to point to the respective
	BSession.

	\param id The ID of the session to be retrieved.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device the session identified by \a id resides on.
	\param session Pointer to a pre-allocated BSession pointer to be set to
		   the session identified by \a id.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: A session with ID \a id could not be found.
	- other error codes
*/
status_t
BDiskDeviceRoster::GetSessionWithID(int32 id, BDiskDevice *device,
									BSession **session) const
{
	return B_ERROR;	// not implemented
}

// GetPartitionWithID
/*!	\brief Returns a BPartition for a given ID.

	The supplied \a device is initialized to the device the partition
	identified by \a id resides on, and \a partition is set to point to the
	respective BPartition.

	\param id The ID of the partition to be retrieved.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device the partition identified by \a id resides on.
	\param partition Pointer to a pre-allocated BPartition pointer to be set
		   to the partition identified by \a id.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: A partition with ID \a id could not be found.
	- other error codes
*/
status_t
BDiskDeviceRoster::GetPartitionWithID(int32 id, BDiskDevice *device,
									  BPartition **partition) const
{
}

// StartWatching
/*!	\brief Adds a target to the list of targets to be notified on disk device
		   events.

	\todo List the event mask flags, the events and describe the layout of the
		  notification message.

	If \a target is already listening to events, this method replaces the
	former event mask with \a eventMask.

	\param target A BMessenger identifying the target to which the events
		   shall be sent.
	\param eventMask A mask specifying on which events the target shall be
		   notified.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::StartWatching(BMessenger target, uint32 eventMask)
{
	return B_ERROR;	// not implemented
}

// StopWatching
/*!	\brief Remove a target from the list of targets to be notified on disk
		   device events.
	\param target A BMessenger identifying the target to which notfication
		   message shall not longer be sent.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::StopWatching(BMessenger target)
{
	return B_ERROR;	// not implemented
}

