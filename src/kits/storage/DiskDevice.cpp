//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevice.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <DiskDevice.h>
#include <DiskDeviceVisitor.h>
#include <Drivers.h>
#include <Message.h>
#include <Path.h>

#include "ddm_userland_interface.h"

/*!	\class BDiskDevice
	\brief A BDiskDevice object represents a storage device.
*/

// constructor
/*!	\brief Creates an uninitialized BDiskDevice object.
*/
BDiskDevice::BDiskDevice()
	: fDeviceData(NULL)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskDevice::~BDiskDevice()
{
}

// HasMedia
/*!	\brief Returns whether the device contains a media.
	\return \c true, if the device contains a media, \c false otherwise.
*/
bool
BDiskDevice::HasMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_HAS_MEDIA);
}

// IsRemovableMedia
/*!	\brief Returns whether the device media are removable.
	\return \c true, if the device media are removable, \c false otherwise.
*/
bool
BDiskDevice::IsRemovableMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_REMOVABLE);
}

// IsReadOnlyMedia
bool
BDiskDevice::IsReadOnlyMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_READ_ONLY);
}

// IsWriteOnceMedia
bool
BDiskDevice::IsWriteOnceMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_WRITE_ONCE);
}

// Eject
/*!	\brief Eject the device's media.

	The device media must, of course, be removable, and the device must
	support ejecting the media.

	\param update If \c true, Update() is invoked after successful ejection.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The device is not properly initialized.
	- \c B_BAD_VALUE: The device media is not removable.
	- other error codes
*/
status_t
BDiskDevice::Eject(bool update)
{
/*	// get path
	const char *path = Path();
	status_t error = (path ? B_OK : B_NO_INIT);
	// check whether the device media is removable
	if (error == B_OK && !IsRemovable())
		error = B_BAD_VALUE;
	// open, eject and close the device
	if (error == B_OK) {
		int fd = open(path, O_RDONLY);
		if (fd < 0 || ioctl(fd, B_EJECT_DEVICE) != 0)
			error = errno;
		if (fd >= 0)
			close(fd);
	}
	if (error == B_OK && update)
		error = Update();
	return error;
*/
	// not implemented
	return B_ERROR;
}

// SetTo
status_t
BDiskDevice::SetTo(partition_id id)
{
	return _SetTo(id, true, false, 0);
}

// Update
/*!	\brief Updates the object to reflect the latest changes to the device.

	Note, that subobjects (BSessions, BPartitions) may be deleted during this
	operation. It is also possible, that the device doesn't exist anymore --
	e.g. if it is hot-pluggable. Then an error is returned and the object is
	uninitialized.

	\param updated Pointer to a bool variable which shall be set to \c true,
		   if the object needed to be updated and to \c false otherwise.
		   May be \c NULL. Is not touched, if the method fails.
	\return \c B_OK, if the update went fine, another error code otherwise.
*/
status_t
BDiskDevice::Update(bool *updated)
{
/*
	// get a messenger for the disk device manager
// TODO: Cache the messenger? Maybe add a global variable?
	BMessenger manager;
	status_t error = get_disk_device_messenger(&manager);
	// compose request message
	BMessage request(B_REG_UPDATE_DISK_DEVICE);
	if (error == B_OK)
		error = request.AddInt32("device_id", fUniqueID);
	if (error == B_OK)
		error = request.AddInt32("change_counter", fChangeCounter);
	if (error == B_OK)
		error = request.AddInt32("update_policy", B_REG_DEVICE_UPDATE_CHANGED);
	// send request
	BMessage reply;
	if (error == B_OK)
		error = manager.SendMessage(&request, &reply);
	// analyze reply
	bool upToDate = true;
	if (error == B_OK) {
		// result
		status_t result = B_OK;
		error = reply.FindInt32("result", &result);
		if (error == B_OK)
			error = result;
		if (error == B_OK)
			error = reply.FindBool("up_to_date", &upToDate);
	}
	// get the archived device, if not up to date
	if (error == B_OK && !upToDate) {
		BMessage archive;
		error = reply.FindMessage("device", &archive);
		if (error == B_OK)
			error = _Update(&archive);
	}
	// set result / cleanup on error
	if (error == B_OK) {
		if (updated)
			*updated = !upToDate;
	} else
		Unset();
	return error;
*/
	// not implemented
	return B_ERROR;
}

// Unset
void
BDiskDevice::Unset()
{
	BPartition::_Unset();
	free(fDeviceData);
	fDeviceData = NULL;
}

// InitCheck
status_t
BDiskDevice::InitCheck() const
{
	return (fDeviceData ? B_OK : B_NO_INIT);
}

// GetPath
status_t
BDiskDevice::GetPath(BPath *path) const
{
	if (!path || !fDeviceData)
		return B_BAD_VALUE;
	return path->SetTo(fDeviceData->path);
}

// IsModified
bool
BDiskDevice::IsModified() const
{
	return (InitCheck() == B_OK && _IsShadow()
			&& _kern_is_disk_device_modified(ID()));
}

// PrepareModifications
status_t
BDiskDevice::PrepareModifications()
{
	// check initialization
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (_IsShadow())
		return B_ERROR;
	// ask kernel to prepare for modifications
	error = _kern_prepare_disk_device_modifications(ID());
	if (error != B_OK)
		return error;
	// update
	// TODO: Add an _Update(bool shadow)?
	return _SetTo(ID(), true, true, 0);
}

// CommitModifications
status_t
BDiskDevice::CommitModifications(bool synchronously,
								 BMessenger progressMessenger,
								 bool receiveCompleteProgressUpdates)
{
	// not implemented
	return B_ERROR;
}

// CancelModifications
status_t
BDiskDevice::CancelModifications()
{
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (!_IsShadow())
		return B_BAD_VALUE;
	error = _kern_cancel_disk_device_modifications(ID());
	if (error == B_OK)
		error = _SetTo(ID(), true, false, 0);
	return error;
}

// _SetTo
status_t
BDiskDevice::_SetTo(partition_id id, bool deviceOnly, bool shadow,
					size_t neededSize)
{
	Unset();
	// get the device data
	void *buffer = NULL;
	size_t bufferSize = 0;
	if (neededSize > 0) {
		// allocate initial buffer
		buffer = malloc(neededSize);
		if (!buffer)
			return B_NO_MEMORY;
		bufferSize = neededSize;
	}
	status_t error = B_OK;
	do {
		error = _kern_get_disk_device_data(id, deviceOnly, shadow,
										   (user_disk_device_data*)buffer,
										   bufferSize, &neededSize);
		if (error == B_BUFFER_OVERFLOW) {
			// buffer to small re-allocate it
			if (buffer)
				free(buffer);
			buffer = malloc(neededSize);
			if (buffer)
				bufferSize = neededSize;
			else
				error = B_NO_MEMORY;
		}
	} while (error == B_BUFFER_OVERFLOW);
	// set the data
	if (error == B_OK)
		error = _SetTo((user_disk_device_data*)buffer);
	// cleanup on error
	if (error != B_OK)
		free(buffer);
	return error;
}

// _SetTo
status_t
BDiskDevice::_SetTo(user_disk_device_data *data)
{
	Unset();
	if (!data)
		return B_BAD_VALUE;
	fDeviceData = data;
	status_t error = BPartition::_SetTo(this, NULL,
										&fDeviceData->device_partition_data);
	if (error != B_OK) {
		// If _SetTo() fails, the caller retains ownership of the supplied
		// data. So, unset fDeviceData before calling Unset().
		fDeviceData = NULL;
		Unset();
	}
	return error;
}

// _AcceptVisitor
bool
BDiskDevice::_AcceptVisitor(BDiskDeviceVisitor *visitor, int32 level)
{
	return visitor->Visit(this);
}


#if 0

// Path
/*!	\brief Returns the path to the device.
	\return The path to the device.
*/
const char *
BDiskDevice::Path() const
{
	return (fPath[0] != '\0' ? fPath : NULL);
}

// skip_string_component
static
const char *
skip_string_component(const char *str, const char *component)
{
	const char *remainder = NULL;
	if (str && component) {
		size_t len = strlen(component);
		if (!strncmp(str, component, len))
			remainder = str + len;
	}
	return remainder;
}

// GetName
/*!	\brief Returns a human readable name for the device.

	The method tries to interpret the device path, which it can do only for
	floppy, IDE and SCSI devices, for others the device path is returned.

	\param name Pointer to a pre-allocated BString to be set to the device
		   name.
	\param includeBusID \c true, if the bus ID shall be included in the name
		   to be returned (of interest for SCSI devices).
	\param includeLUN \c true, if the LUN shall be included in the name
		   to be returned (of interest for SCSI devices).
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name.
	- \c B_NO_INIT: The device is not properly initialized.
*/
status_t
BDiskDevice::GetName(BString *name, bool includeBusID, bool includeLUN) const
{
	// check params and initialization
	status_t error = (name ? B_OK : B_BAD_VALUE);
	const char *path = Path();
	if (error == B_OK && !path)
		error = B_BAD_VALUE;
	// analyze the device path
	if (error == B_OK) {
		bool recognized = false;
		const char *prefix = "/dev/disk/";
		size_t prefixLen = strlen(prefix);
		if (!strncmp(path, prefix, prefixLen)) {
			path = path + prefixLen;
			// check first component
			const char *tail = NULL;
			// floppy
			if (skip_string_component(path, "floppy/")) {
				name->SetTo("floppy");
				recognized = true;
			// ide
			} else if ((tail = skip_string_component(path, "ide/"))) {
				int controller = 0;
				bool master = false;
				const char *_tail = tail;
				if ((((tail = skip_string_component(_tail, "atapi/")))
					 || ((tail = skip_string_component(_tail, "ata/"))))
					&& isdigit(*tail)) {
					controller = atoi(tail);
					master = false;
					if ((tail = strchr(tail, '/'))) {
						tail++;
						if (skip_string_component(tail, "master/")) {
							master = true;
							recognized = true;
						} else if (skip_string_component(tail, "slave/"))
							recognized = true;
					}
				}
				if (recognized) {
					name->SetTo("IDE ");
					*name << (master ? "master" : "slave") << " bus:"
						<< controller;
				}
			// scsi
			} else if ((tail = skip_string_component(path, "scsi/"))) {
				int bus = 0;
				int id = 0;
				int lun = 0;
				if (sscanf(tail, "%d/%d/%d/raw", &bus, &id, &lun) == 3) {
					recognized = true;
					name->SetTo("SCSI");
					if (includeBusID)
						*name << " bus:" << bus;
					*name << " id:" << id;
					if (includeLUN)
						*name << " lun:" << lun;
				}
			}
		}
		if (!recognized)
			name->SetTo(path);
	}
	return error;
}

// GetName
/*!	\brief Returns a human readable name for the device.

	The method tries to interpret the device path, which it can do only for
	floppy, IDE and SCSI devices, for others the device path is returned.

	\param name Pointer to a pre-allocated char buffer into which the device
		   name shall be copied.
	\param includeBusID \c true, if the bus ID shall be included in the name
		   to be returned (of interest for SCSI devices).
	\param includeLUN \c true, if the LUN shall be included in the name
		   to be returned (of interest for SCSI devices).
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name.
	- \c B_NO_INIT: The device is not properly initialized.
*/
status_t
BDiskDevice::GetName(char *name, bool includeBusID, bool includeLUN) const
{
	status_t error = (name ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BString _name;
		error = GetName(&_name, includeBusID, includeLUN);
		if (error == B_OK)
			strcpy(name, _name.String());
	}
	return error;
}

// IsReadOnly
/*!	\brief Returns whether the device is read-only.
	\return \c true, if the device is read-only, \c false otherwise.
*/
bool
BDiskDevice::IsReadOnly() const
{
	return fReadOnly;
}

// IsFloppy
/*!	\brief Returns whether the device is a floppy device.
	\return \c true, if the device is a floppy device, \c false otherwise.
*/
bool
BDiskDevice::IsFloppy() const
{
	return !strcmp(fPath, "/dev/disk/floppy/raw");
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
	return fType;
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
	return fUniqueID;
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
	if (visitor) {
		for (int32 i = 0; BSession *session = SessionAt(i); i++) {
			if (visitor->Visit(session))
				return session;
		}
	}
	return NULL;
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
	if (visitor) {
		for (int32 i = 0; BSession *session = SessionAt(i); i++) {
			if (BPartition *partition = session->VisitEachPartition(visitor))
				return partition;
		}
	}
	return NULL;
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
	if (visitor) {
		// visit the device itself
		if (visitor->Visit(this))
			return true;
		for (int32 i = 0; BSession *session = SessionAt(i); i++) {
			// visit the session
			if (visitor->Visit(session))
				return true;
			// visit all partitions on the session
			if (session->VisitEachPartition(visitor))
				return true;
		}
	}
	return false;
}

// SessionWithID
/*!	\brief Returns the session on the device, that has a certain ID.
	\param id The ID of the session to be returned.
	\return The session with ID \a id, or \c NULL, if a session with that
			ID does not exist on this device.
*/
BSession *
BDiskDevice::SessionWithID(int32 id)
{
	IDFinderVisitor visitor(id);
	return VisitEachSession(&visitor);
}

// PartitionWithID
/*!	\brief Returns the partition on the device, that has a certain ID.
	\param id The ID of the partition to be returned.
	\return The partition with ID \a id, or \c NULL, if a partition with that
			ID does not exist on this device.
*/
BPartition *
BDiskDevice::PartitionWithID(int32 id)
{
	IDFinderVisitor visitor(id);
	return VisitEachPartition(&visitor);
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
BDiskDevice::_Unarchive(BMessage *archive)
{
//printf("BDiskDevice::_Unarchive()\n");
//archive->PrintToStream();
	Unset();
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// ID and change counter
		if (error == B_OK)
			error = archive->FindInt32("id", &fUniqueID);
		if (error == B_OK)
			error = archive->FindInt32("change_counter", &fChangeCounter);
		// geometry
		if (error == B_OK)
			error = archive->FindInt64("size", &fSize);
		if (error == B_OK)
			error = archive->FindInt32("block_size", &fBlockSize);
		if (error == B_OK)
			error = archive->FindInt8("type", (int8*)&fType);
		if (error == B_OK)
			error = archive->FindBool("removable", &fRemovable);
		if (error == B_OK)
			error = archive->FindBool("read_only", &fReadOnly);
		// other data
		if (error == B_OK)
			error = find_string(archive, "path", fPath);
		if (error == B_OK)
			error = archive->FindInt32("media_status", &fMediaStatus);
		// sessions
		type_code fieldType;
		int32 count = 0;
		if (error == B_OK) {
			if (archive->GetInfo("sessions", &fieldType, &count) != B_OK)
				count = 0;
		}
		for (int32 i = 0; error == B_OK && i < count; i++) {
			// get the archived session
			BMessage sessionArchive;
			error = archive->FindMessage("sessions", i, &sessionArchive);
			// allocate a session object
			BSession *session = NULL;
			if (error == B_OK) {
				session = new(nothrow) BSession;
				if (!session)
					error = B_NO_MEMORY;
			}
			// unarchive the session
			if (error == B_OK)
				error = session->_Unarchive(&sessionArchive);
			// add the session
			if (error == B_OK && !_AddSession(session))
				error = B_NO_MEMORY;
			// cleanup on error
			if (error != B_OK && session)
				delete session;
		}
	}
	// cleanup on error
	if (error != B_OK)
		Unset();
//printf("BDiskDevice::_Unarchive() done: %s\n", strerror(error));
	return error;
}

// _Update
status_t
BDiskDevice::_Update(BMessage *archive)
{
	status_t error = (archive ? B_OK : B_BAD_VALUE);
	bool upToDate = false;
	if (error == B_OK) {
		// ID and change counter
		int32 id;
		if (error == B_OK)
			error = archive->FindInt32("id", &id);
		if (error == B_OK && id != fUniqueID)
			error = B_ERROR;
		int32 changeCounter;
		if (error == B_OK)
			error = archive->FindInt32("change_counter", &changeCounter);
		upToDate = (fChangeCounter == changeCounter);
		fChangeCounter = changeCounter;
	}
	if (error == B_OK && !upToDate) {
		// geometry
		if (error == B_OK)
			error = archive->FindInt64("size", &fSize);
		if (error == B_OK)
			error = archive->FindInt32("block_size", &fBlockSize);
		if (error == B_OK)
			error = archive->FindInt8("type", (int8*)&fType);
		if (error == B_OK)
			error = archive->FindBool("removable", &fRemovable);
		if (error == B_OK)
			error = archive->FindBool("read_only", &fReadOnly);
		// other data
		if (error == B_OK)
			error = find_string(archive, "path", fPath);
		if (error == B_OK)
			error = archive->FindInt32("media_status", &fMediaStatus);
		// sessions
		// copy old session list and empty it
		BObjectList<BSession> sessions;
		for (int32 i = 0; BSession *session = fSessions.ItemAt(i); i++)
			sessions.AddItem(session);
		for (int32 i = fSessions.CountItems() - 1; i >= 0; i--)
			fSessions.RemoveItemAt(i);
		// get the session count
		type_code fieldType;
		int32 count = 0;
		if (error == B_OK) {
			if (archive->GetInfo("sessions", &fieldType, &count) != B_OK)
				count = 0;
		}
		for (int32 i = 0; error == B_OK && i < count; i++) {
			// get the archived session
			BMessage sessionArchive;
			error = archive->FindMessage("sessions", i, &sessionArchive);
			// check, whether we do already know that session
			int32 sessionID;
			if (error == B_OK)
				error = sessionArchive.FindInt32("id", &sessionID);
			BSession *session = NULL;
			for (int32 k = 0; k < sessions.CountItems(); k++) {
				BSession *oldSession = sessions.ItemAt(i);
				if (oldSession->UniqueID() == sessionID) {
					session = oldSession;
					break;
				}
			}
			if (error == B_OK) {
				if (session) {
					// the session is known: just update it
					error = session->_Update(&sessionArchive);
				} else {
					// the session is unknown
					// allocate a session object
					session = new(nothrow) BSession;
					if (!session)
						error = B_NO_MEMORY;
					// unarchive the session
					if (error == B_OK)
						error = session->_Unarchive(&sessionArchive);
				}
			}
			// add the session
			if (error == B_OK && !_AddSession(session))
				error = B_NO_MEMORY;
			// cleanup on error
			if (error != B_OK && session)
				delete session;
		}
		// delete all obsolete sessions
		for (int32 i = sessions.CountItems() - 1; i >= 0; i--)
			delete sessions.RemoveItemAt(i);
	}
	// cleanup on error
	if (error != B_OK)
		Unset();
	return error;
}

// _AddSession
bool
BDiskDevice::_AddSession(BSession *session)
{
	bool result = fSessions.AddItem(session);
	if (result)
		session->_SetDevice(this);
	return result;
}

#endif	//0
