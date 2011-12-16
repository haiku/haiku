/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include <DiskDeviceRoster.h>

#include <new>

#include <Directory.h>
#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskSystem.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Partition.h>
#include <Path.h>
#include <Volume.h>

#include <MessengerPrivate.h>

#include <syscalls.h>
#include <ddm_userland_interface_defs.h>


/*!	\class BDiskDeviceRoster
	\brief An interface for iterating through the disk devices known to the
		   system and for a notification mechanism provided to listen to their
		   changes.
*/

/*!	\brief find_directory constants of the add-on dirs to be searched. */
static const directory_which kAddOnDirs[] = {
	B_USER_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_SYSTEM_ADDONS_DIRECTORY,
};
/*!	\brief Size of the kAddOnDirs array. */
static const int32 kAddOnDirCount
	= sizeof(kAddOnDirs) / sizeof(directory_which);


/*!	\brief Creates a BDiskDeviceRoster object.

	The object is ready to be used after construction.
*/
BDiskDeviceRoster::BDiskDeviceRoster()
	: fDeviceCookie(0),
	  fDiskSystemCookie(0),
	  fJobCookie(0)
//	  fPartitionAddOnDir(NULL),
//	  fFSAddOnDir(NULL),
//	  fPartitionAddOnDirIndex(0),
//	  fFSAddOnDirIndex(0)
{
}


/*!	\brief Frees all resources associated with the object.
*/
BDiskDeviceRoster::~BDiskDeviceRoster()
{
//	if (fPartitionAddOnDir)
//		delete fPartitionAddOnDir;
//	if (fFSAddOnDir)
//		delete fFSAddOnDir;
}


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
BDiskDeviceRoster::GetNextDevice(BDiskDevice* device)
{
	if (!device)
		return B_BAD_VALUE;

	size_t neededSize = 0;
	partition_id id = _kern_get_next_disk_device_id(&fDeviceCookie,
		&neededSize);
	if (id < 0)
		return id;

	return device->_SetTo(id, true, neededSize);
}


/*!	\brief Rewinds the device list iterator.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::RewindDevices()
{
	fDeviceCookie = 0;
	return B_OK;
}


status_t
BDiskDeviceRoster::GetNextDiskSystem(BDiskSystem* system)
{
	if (!system)
		return B_BAD_VALUE;
	user_disk_system_info info;
	status_t error = _kern_get_next_disk_system_info(&fDiskSystemCookie,
		&info);
	if (error == B_OK)
		error = system->_SetTo(&info);
	return error;
}


status_t
BDiskDeviceRoster::RewindDiskSystems()
{
	fDiskSystemCookie = 0;
	return B_OK;
}


status_t
BDiskDeviceRoster::GetDiskSystem(BDiskSystem* system, const char* name)
{
	if (!system)
		return B_BAD_VALUE;

	int32 cookie = 0;
	user_disk_system_info info;
	while (_kern_get_next_disk_system_info(&cookie, &info) == B_OK) {
		if (!strcmp(name, info.name)
			|| !strcmp(name, info.short_name)
			|| !strcmp(name, info.pretty_name))
			return system->_SetTo(&info);
	}

	return B_ENTRY_NOT_FOUND;
}


partition_id
BDiskDeviceRoster::RegisterFileDevice(const char* filename)
{
	if (!filename)
		return B_BAD_VALUE;
	return _kern_register_file_device(filename);
}


status_t
BDiskDeviceRoster::UnregisterFileDevice(const char* filename)
{
	if (!filename)
		return B_BAD_VALUE;
	return _kern_unregister_file_device(-1, filename);
}


status_t
BDiskDeviceRoster::UnregisterFileDevice(partition_id device)
{
	if (device < 0)
		return B_BAD_VALUE;
	return _kern_unregister_file_device(device, NULL);
}


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
BDiskDeviceRoster::VisitEachDevice(BDiskDeviceVisitor* visitor,
	BDiskDevice* device)
{
	bool terminatedEarly = false;
	if (visitor) {
		int32 oldCookie = fDeviceCookie;
		fDeviceCookie = 0;
		BDiskDevice deviceOnStack;
		BDiskDevice* useDevice = device ? device : &deviceOnStack;
		while (!terminatedEarly && GetNextDevice(useDevice) == B_OK)
			terminatedEarly = visitor->Visit(useDevice);
		fDeviceCookie = oldCookie;
		if (!terminatedEarly)
			useDevice->Unset();
	}
	return terminatedEarly;
}


/*!	\brief Pre-order traverses the trees spanned by the BDiskDevices and their
		   subobjects.

	The supplied visitor's Visit(BDiskDevice*) method is invoked for each
	disk device and Visit(BPartition*) for each (non-disk device) partition.
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
BDiskDeviceRoster::VisitEachPartition(BDiskDeviceVisitor* visitor,
	BDiskDevice* device, BPartition** partition)
{
	bool terminatedEarly = false;
	if (visitor) {
		int32 oldCookie = fDeviceCookie;
		fDeviceCookie = 0;
		BDiskDevice deviceOnStack;
		BDiskDevice* useDevice = device ? device : &deviceOnStack;
		BPartition* foundPartition = NULL;
		while (GetNextDevice(useDevice) == B_OK) {
			foundPartition = useDevice->VisitEachDescendant(visitor);
			if (foundPartition) {
				terminatedEarly = true;
				break;
			}
		}
		fDeviceCookie = oldCookie;
		if (!terminatedEarly)
			useDevice->Unset();
		else if (device && partition)
			*partition = foundPartition;
	}
	return terminatedEarly;
}


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
BDiskDeviceRoster::VisitEachMountedPartition(BDiskDeviceVisitor* visitor,
	BDiskDevice* device, BPartition** partition)
{
	bool terminatedEarly = false;
	if (visitor) {
		struct MountedPartitionFilter : public PartitionFilter {
			virtual bool Filter(BPartition *partition, int32)
				{ return partition->IsMounted(); }
		} filter;
		PartitionFilterVisitor filterVisitor(visitor, &filter);
		terminatedEarly
			= VisitEachPartition(&filterVisitor, device, partition);
	}
	return terminatedEarly;
}


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
BDiskDeviceRoster::VisitEachMountablePartition(BDiskDeviceVisitor* visitor,
	BDiskDevice* device, BPartition** partition)
{
	bool terminatedEarly = false;
	if (visitor) {
		struct MountablePartitionFilter : public PartitionFilter {
			virtual bool Filter(BPartition *partition, int32)
				{ return partition->ContainsFileSystem(); }
		} filter;
		PartitionFilterVisitor filterVisitor(visitor, &filter);
		terminatedEarly
			= VisitEachPartition(&filterVisitor, device, partition);
	}
	return terminatedEarly;
}


/*!	\brief Finds a BPartition by BVolume.
*/
status_t
BDiskDeviceRoster::FindPartitionByVolume(const BVolume& volume,
	BDiskDevice* device, BPartition** _partition)
{
	class FindPartitionVisitor : public BDiskDeviceVisitor {
	public:
		FindPartitionVisitor(dev_t volume)
			:
			fVolume(volume)
		{
		}

		virtual bool Visit(BDiskDevice* device)
		{
			return Visit(device, 0);
		}

		virtual bool Visit(BPartition* partition, int32 level)
		{
			BVolume volume;
			return partition->GetVolume(&volume) == B_OK
				&& volume.Device() == fVolume;
		}

	private:
		dev_t	fVolume;
	} visitor(volume.Device());

	if (VisitEachMountedPartition(&visitor, device, _partition))
		return B_OK;

	return B_ENTRY_NOT_FOUND;
}


/*!	\brief Finds a BPartition by mount path.
*/
status_t
BDiskDeviceRoster::FindPartitionByMountPoint(const char* mountPoint,
	BDiskDevice* device, BPartition** _partition)
{
	BVolume volume(dev_for_path(mountPoint));
	if (volume.InitCheck() == B_OK
		&& FindPartitionByVolume(volume, device, _partition))
		return B_OK;

	return B_ENTRY_NOT_FOUND;
}


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
BDiskDeviceRoster::GetDeviceWithID(int32 id, BDiskDevice* device) const
{
	if (!device)
		return B_BAD_VALUE;
	return device->_SetTo(id, true, 0);
}


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
BDiskDeviceRoster::GetPartitionWithID(int32 id, BDiskDevice* device,
	BPartition** partition) const
{
	if (!device || !partition)
		return B_BAD_VALUE;

	// retrieve the device data
	status_t error = device->_SetTo(id, false, 0);
	if (error != B_OK)
		return error;

	// find the partition object
	*partition = device->FindDescendant(id);
	if (!*partition)	// should never happen!
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


status_t
BDiskDeviceRoster::GetDeviceForPath(const char* filename, BDiskDevice* device)
{
	if (!filename || !device)
		return B_BAD_VALUE;

	// get the device ID
	size_t neededSize = 0;
	partition_id id = _kern_find_disk_device(filename, &neededSize);
	if (id < 0)
		return id;

	// retrieve the device data
	return device->_SetTo(id, true, neededSize);
}


status_t
BDiskDeviceRoster::GetPartitionForPath(const char* filename,
	BDiskDevice* device, BPartition** partition)
{
	if (!filename || !device || !partition)
		return B_BAD_VALUE;

	// get the partition ID
	size_t neededSize = 0;
	partition_id id = _kern_find_partition(filename, &neededSize);
	if (id < 0)
		return id;

	// retrieve the device data
	status_t error = device->_SetTo(id, false, neededSize);
	if (error != B_OK)
		return error;

	// find the partition object
	*partition = device->FindDescendant(id);
	if (!*partition)	// should never happen!
		return B_ENTRY_NOT_FOUND;
	return B_OK;
}


status_t
BDiskDeviceRoster::GetFileDeviceForPath(const char* filename,
	BDiskDevice* device)
{
	if (!filename || !device)
		return B_BAD_VALUE;

	// get the device ID
	size_t neededSize = 0;
	partition_id id = _kern_find_file_disk_device(filename, &neededSize);
	if (id < 0)
		return id;

	// retrieve the device data
	return device->_SetTo(id, true, neededSize);
}


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
	if (eventMask == 0)
		return B_BAD_VALUE;

	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();

	return _kern_start_watching_disks(eventMask, port, token);
}


/*!	\brief Remove a target from the list of targets to be notified on disk
		   device events.
	\param target A BMessenger identifying the target to which notfication
		   message shall not longer be sent.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::StopWatching(BMessenger target)
{
	BMessenger::Private messengerPrivate(target);
	port_id port = messengerPrivate.Port();
	int32 token = messengerPrivate.Token();

	return _kern_stop_watching_disks(port, token);
}

#if 0

/*!	\brief Returns the next partitioning system capable of partitioning.

	The returned \a shortName can be passed to BSession::Partition().

	\param shortName Pointer to a pre-allocation char buffer, of size
		   \c B_FILE_NAME_LENGTH or larger into which the short name of the
		   partitioning system shall be written.
	\param longName Pointer to a pre-allocation char buffer, of size
		   \c B_FILE_NAME_LENGTH or larger into which the long name of the
		   partitioning system shall be written. May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a shortName.
	- \c B_ENTRY_NOT_FOUND: End of the list has been reached.
	- other error codes
*/
status_t
BDiskDeviceRoster::GetNextPartitioningSystem(char *shortName, char *longName)
{
	status_t error = (shortName ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// search until an add-on has been found or the end of all directories
		// has been reached
		bool found = false;
		do {
			// get the next add-on in the current dir
			AddOnImage image;
			error = _GetNextAddOn(fPartitionAddOnDir, &image);
			if (error == B_OK) {
				// add-on loaded: get the function that creates an add-on
				// object
				BDiskScannerPartitionAddOn *(*create_add_on)();
				if (get_image_symbol(image.ID(), "create_ds_partition_add_on",
									 B_SYMBOL_TYPE_TEXT,
									 (void**)&create_add_on) == B_OK) {
					// create the add-on object and copy the requested data
					if (BDiskScannerPartitionAddOn *addOn
						= (*create_add_on)()) {
						const char *addOnShortName = addOn->ShortName();
						const char *addOnLongName = addOn->LongName();
						if (addOnShortName && addOnLongName) {
							strcpy(shortName, addOnShortName);
							if (longName)
								strcpy(longName, addOnLongName);
							found = true;
						}
						delete addOn;
					}
				}
			} else if (error == B_ENTRY_NOT_FOUND) {
				// end of the current directory has been reached, try next dir
				error = _GetNextAddOnDir(&fPartitionAddOnDir,
										 &fPartitionAddOnDirIndex,
										 "partition");
			}
		} while (error == B_OK && !found);
	}
	return error;
}


/*!	\brief Returns the next file system capable of initializing.

	The returned \a shortName can be passed to BPartition::Initialize().

	\param shortName Pointer to a pre-allocation char buffer, of size
		   \c B_FILE_NAME_LENGTH or larger into which the short name of the
		   file system shall be written.
	\param longName Pointer to a pre-allocation char buffer, of size
		   \c B_FILE_NAME_LENGTH or larger into which the long name of the
		   file system shall be written. May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a shortName.
	- \c B_ENTRY_NOT_FOUND: End of the list has been reached.
	- other error codes
*/
status_t
BDiskDeviceRoster::GetNextFileSystem(char *shortName, char *longName)
{
	status_t error = (shortName ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// search until an add-on has been found or the end of all directories
		// has been reached
		bool found = false;
		do {
			// get the next add-on in the current dir
			AddOnImage image;
			error = _GetNextAddOn(fFSAddOnDir, &image);
			if (error == B_OK) {
				// add-on loaded: get the function that creates an add-on
				// object
				BDiskScannerFSAddOn *(*create_add_on)();
				if (get_image_symbol(image.ID(), "create_ds_fs_add_on",
									 B_SYMBOL_TYPE_TEXT,
									 (void**)&create_add_on) == B_OK) {
					// create the add-on object and copy the requested data
					if (BDiskScannerFSAddOn *addOn = (*create_add_on)()) {
						const char *addOnShortName = addOn->ShortName();
						const char *addOnLongName = addOn->LongName();
						if (addOnShortName && addOnLongName) {
							strcpy(shortName, addOnShortName);
							if (longName)
								strcpy(longName, addOnLongName);
							found = true;
						}
						delete addOn;
					}
				}
			} else if (error == B_ENTRY_NOT_FOUND) {
				// end of the current directory has been reached, try next dir
				error = _GetNextAddOnDir(&fFSAddOnDir, &fFSAddOnDirIndex,
										 "fs");
			}
		} while (error == B_OK && !found);
	}
	return error;
}


/*!	\brief Rewinds the partitioning system list iterator.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::RewindPartitiningSystems()
{
	if (fPartitionAddOnDir) {
		delete fPartitionAddOnDir;
		fPartitionAddOnDir = NULL;
	}
	fPartitionAddOnDirIndex = 0;
	return B_OK;
}


/*!	\brief Rewinds the file system list iterator.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskDeviceRoster::RewindFileSystems()
{
	if (fFSAddOnDir) {
		delete fFSAddOnDir;
		fFSAddOnDir = NULL;
	}
	fFSAddOnDirIndex = 0;
	return B_OK;
}


/*!	\brief Returns a BDiskDevice for a given device, session or partition ID.

	The supplied \a device is initialized to the device the object identified
	by \a id belongs to.

	\param fieldName "device_id", "sesison_id" or "partition_id" according to
		   the type of object the device shall be retrieved for.
	\param id The ID of the device, session or partition to be retrieved.
	\param device Pointer to a pre-allocated BDiskDevice to be initialized
		   to the device to be retrieved.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: A device, session or partition respectively with
		 ID \a id could not be found.
	- other error codes
*/
status_t
BDiskDeviceRoster::_GetObjectWithID(const char *fieldName, int32 id,
	BDiskDevice *device) const
{
	status_t error = (device ? B_OK : B_BAD_VALUE);
	// compose request message
	BMessage request(B_REG_GET_DISK_DEVICE);
	if (error == B_OK)
		error = request.AddInt32(fieldName, id);
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
		// device
		BMessage archive;
		if (error == B_OK)
			error = reply.FindMessage("device", &archive);
		if (error == B_OK)
			error = device->_Unarchive(&archive);
	}
	return error;
}


/*!	\brief Finds and loads the next add-on of an add-on subdirectory.
	\param directory The add-on directory.
	\param image Pointer to an image_id into which the image ID of the loaded
		   add-on shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: End of directory.
	- other error codes
*/
status_t
BDiskDeviceRoster::_GetNextAddOn(BDirectory **directory, int32 *index,
	const char *subdir, AddOnImage *image)
{
	status_t error = (directory && index && subdir && image
					  ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// search until an add-on has been found or the end of all directories
		// has been reached
		bool found = false;
		do {
			// get the next add-on in the current dir
			error = _GetNextAddOn(*directory, image);
			if (error == B_OK) {
				found = true;
			} else if (error == B_ENTRY_NOT_FOUND) {
				// end of the current directory has been reached, try next dir
				error = _GetNextAddOnDir(directory, index, subdir);
			}
		} while (error == B_OK && !found);
	}
	return error;
}


/*!	\brief Finds and loads the next add-on of an add-on subdirectory.
	\param directory The add-on directory.
	\param image Pointer to an image_id into which the image ID of the loaded
		   add-on shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: End of directory.
	- other error codes
*/
status_t
BDiskDeviceRoster::_GetNextAddOn(BDirectory *directory, AddOnImage *image)
{
	status_t error = (directory ? B_OK : B_ENTRY_NOT_FOUND);
	if (error == B_OK) {
		// iterate through the entry list and try to load the entries
		bool found = false;
		while (error == B_OK && !found) {
			BEntry entry;
			error = directory->GetNextEntry(&entry);
			BPath path;
			if (error == B_OK && entry.GetPath(&path) == B_OK)
				found = (image->Load(path.Path()) == B_OK);
		}
	}
	return error;
}


/*!	\brief Gets the next add-on directory path.
	\param path Pointer to a BPath to be set to the found directory.
	\param index Pointer to an index into the kAddOnDirs array indicating
		   which add-on dir shall be retrieved next.
	\param subdir Name of the subdirectory (in the "disk_scanner" subdirectory
		   of the add-on directory) \a directory shall be set to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: End of directory list.
	- other error codes
*/
status_t
BDiskDeviceRoster::_GetNextAddOnDir(BPath *path, int32 *index,
	const char *subdir)
{
	status_t error = (*index < kAddOnDirCount ? B_OK : B_ENTRY_NOT_FOUND);
	// get the add-on dir path
	if (error == B_OK) {
		error = find_directory(kAddOnDirs[*index], path);
		(*index)++;
	}
	// construct the subdirectory path
	if (error == B_OK) {
		error = path->Append("disk_scanner");
		if (error == B_OK)
			error = path->Append(subdir);
	}
if (error == B_OK)
printf("  next add-on dir: `%s'\n", path->Path());
	return error;
}


/*!	\brief Gets the next add-on directory.
	\param directory Pointer to a BDirectory* to be set to the found directory.
	\param index Pointer to an index into the kAddOnDirs array indicating
		   which add-on dir shall be retrieved next.
	\param subdir Name of the subdirectory (in the "disk_scanner" subdirectory
		   of the add-on directory) \a directory shall be set to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_ENTRY_NOT_FOUND: End of directory list.
	- other error codes
*/
status_t
BDiskDeviceRoster::_GetNextAddOnDir(BDirectory **directory, int32 *index,
	const char *subdir)
{
	BPath path;
	status_t error = _GetNextAddOnDir(&path, index, subdir);
	// create a BDirectory object, if there is none yet.
	if (error == B_OK && !*directory) {
		*directory = new BDirectory;
		if (!*directory)
			error = B_NO_MEMORY;
	}
	// init the directory
	if (error == B_OK)
		error = (*directory)->SetTo(path.Path());
	// cleanup on error
	if (error != B_OK && *directory) {
		delete *directory;
		*directory = NULL;
	}
	return error;
}


status_t
BDiskDeviceRoster::_LoadPartitionAddOn(const char *partitioningSystem,
	AddOnImage *image, BDiskScannerPartitionAddOn **_addOn)
{
	status_t error = partitioningSystem && image && _addOn
		? B_OK : B_BAD_VALUE;

	// load the image
	bool found = false;
	BPath path;
	BDirectory *directory = NULL;
	int32 index = 0;
	while (error == B_OK && !found) {
		error = _GetNextAddOn(&directory, &index, "partition", image);
		if (error == B_OK) {
			// add-on loaded: get the function that creates an add-on
			// object
			BDiskScannerPartitionAddOn *(*create_add_on)();
			if (get_image_symbol(image->ID(), "create_ds_partition_add_on",
								 B_SYMBOL_TYPE_TEXT,
								 (void**)&create_add_on) == B_OK) {
				// create the add-on object and copy the requested data
				if (BDiskScannerPartitionAddOn *addOn = (*create_add_on)()) {
					if (!strcmp(addOn->ShortName(), partitioningSystem)) {
						*_addOn = addOn;
						found = true;
					} else
						delete addOn;
				}
			}
		}
	}
	// cleanup
	if (directory)
		delete directory;
	if (error != B_OK && image)
		image->Unload();
	return error;
}

#endif	// 0
