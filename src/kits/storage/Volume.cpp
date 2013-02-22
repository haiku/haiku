/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold
 */


#include <errno.h>
#include <string.h>

#include <Bitmap.h>
#include <Directory.h>
#include <fs_info.h>
#include <Node.h>
#include <Path.h>
#include <Volume.h>

#include <storage_support.h>
#include <syscalls.h>

#include <fs_interface.h>


// Creates an uninitialized BVolume object.
BVolume::BVolume()
	: fDevice((dev_t)-1),
	  fCStatus(B_NO_INIT)
{
}


// Creates a BVolume and initializes it to the volume specified by the
// supplied device ID.
BVolume::BVolume(dev_t device)
	: fDevice((dev_t)-1),
	  fCStatus(B_NO_INIT)
{
	SetTo(device);
}


// Creates a copy of the supplied BVolume object.
BVolume::BVolume(const BVolume &volume)
	: fDevice(volume.fDevice),
	  fCStatus(volume.fCStatus)
{
}


// Destroys the object and frees all associated resources.
BVolume::~BVolume()
{
}


// Returns the initialization status.
status_t
BVolume::InitCheck(void) const
{
	return fCStatus;
}


// Initializes the object to refer to the volume specified by the supplied
// device ID.
status_t
BVolume::SetTo(dev_t device)
{
	// uninitialize
	Unset();
	// check the parameter
	status_t error = (device >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fs_info info;
		if (fs_stat_dev(device, &info) != 0)
			error = errno;
	}
	// set the new value
	if (error == B_OK)
		fDevice = device;
	// set the init status variable
	fCStatus = error;
	return fCStatus;
}


// Brings the BVolume object to an uninitialized state.
void
BVolume::Unset()
{
	fDevice = (dev_t)-1;
	fCStatus = B_NO_INIT;
}


// Returns the device ID of the volume the object refers to.
dev_t
BVolume::Device() const
{
	return fDevice;
}


// Writes the root directory of the volume referred to by this object into
// directory.
status_t
BVolume::GetRootDirectory(BDirectory *directory) const
{
	// check parameter and initialization
	status_t error = (directory && InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	// init the directory
	if (error == B_OK) {
		node_ref ref;
		ref.device = info.dev;
		ref.node = info.root;
		error = directory->SetTo(&ref);
	}
	return error;
}


// Returns the total storage capacity of the volume.
off_t
BVolume::Capacity() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK ? info.total_blocks * info.block_size : error);
}


// Returns the amount of unused space on the volume (in bytes).
off_t
BVolume::FreeBytes() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK ? info.free_blocks * info.block_size : error);
}


// Returns the size of one block (in bytes).
off_t
BVolume::BlockSize() const
{
	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// get FS stat
	fs_info info;
	if (fs_stat_dev(fDevice, &info) != 0)
		return errno;

	return info.block_size;
}


// Copies the name of the volume into the provided buffer.
status_t
BVolume::GetName(char *name) const
{
	// check parameter and initialization
	status_t error = (name && InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	// copy the name
	if (error == B_OK)
		strncpy(name, info.volume_name, B_FILE_NAME_LENGTH);
	return error;
}


// Sets the name of the volume.
status_t
BVolume::SetName(const char *name)
{
	// check initialization
	if (!name || InitCheck() != B_OK)
		return B_BAD_VALUE;
	if (strlen(name) >= B_FILE_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	// get the FS stat (including the old name) first
	fs_info oldInfo;
	if (fs_stat_dev(fDevice, &oldInfo) != 0)
		return errno;
	if (strcmp(name, oldInfo.volume_name) == 0)
		return B_OK;
	// set the volume name
	fs_info newInfo;
	strlcpy(newInfo.volume_name, name, sizeof(newInfo.volume_name));
	status_t error = _kern_write_fs_info(fDevice, &newInfo,
		FS_WRITE_FSINFO_NAME);
	if (error != B_OK)
		return error;

	// change the name of the mount point

	// R5 implementation checks if an entry with the volume's old name
	// exists in the root directory and renames that entry, if it is indeed
	// the mount point of the volume (or a link referring to it). In all other
	// cases, nothing is done (even if the mount point is named like the
	// volume, but lives in a different directory).
	// We follow suit for the time being.
	// NOTE: If the volume name itself is actually "boot", then this code
	// tries to rename /boot, but that is prevented in the kernel.

	BPath entryPath;
	BEntry entry;
	BEntry traversedEntry;
	node_ref entryNodeRef;
	if (BPrivate::Storage::check_entry_name(name) == B_OK
		&& BPrivate::Storage::check_entry_name(oldInfo.volume_name) == B_OK
		&& entryPath.SetTo("/", oldInfo.volume_name) == B_OK
		&& entry.SetTo(entryPath.Path(), false) == B_OK
		&& entry.Exists()
		&& traversedEntry.SetTo(entryPath.Path(), true) == B_OK
		&& traversedEntry.GetNodeRef(&entryNodeRef) == B_OK
		&& entryNodeRef.device == fDevice
		&& entryNodeRef.node == oldInfo.root) {
		entry.Rename(name, false);
	}
	return error;
}


// Writes the volume's icon into icon.
status_t
BVolume::GetIcon(BBitmap *icon, icon_size which) const
{
	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// get FS stat for the device name
	fs_info info;
	if (fs_stat_dev(fDevice, &info) != 0)
		return errno;

	// get the icon
	return get_device_icon(info.device_name, icon, which);
}


status_t
BVolume::GetIcon(uint8** _data, size_t* _size, type_code* _type) const
{
	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// get FS stat for the device name
	fs_info info;
	if (fs_stat_dev(fDevice, &info) != 0)
		return errno;

	// get the icon
	return get_device_icon(info.device_name, _data, _size, _type);
}


// Returns whether or not the volume is removable.
bool
BVolume::IsRemovable() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_REMOVABLE));
}


// Returns whether or not the volume is read-only.
bool
BVolume::IsReadOnly(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_READONLY));
}


// Returns whether or not the volume is persistent.
bool
BVolume::IsPersistent(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_PERSISTENT));
}


// Returns whether or not the volume is shared.
bool
BVolume::IsShared(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_SHARED));
}


// Returns whether or not the volume supports MIME-types.
bool
BVolume::KnowsMime(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_MIME));
}


// Returns whether or not the volume supports attributes.
bool
BVolume::KnowsAttr(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_ATTR));
}


// Returns whether or not the volume supports queries.
bool
BVolume::KnowsQuery(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_QUERY));
}


// Returns whether or not the supplied BVolume object is a equal
// to this object.
bool
BVolume::operator==(const BVolume &volume) const
{
	return ((InitCheck() != B_OK && volume.InitCheck() != B_OK)
			|| fDevice == volume.fDevice);
}

// Returns whether or not the supplied BVolume object is NOT equal
// to this object.
bool
BVolume::operator!=(const BVolume &volume) const
{
	return !(*this == volume);
}


// Assigns the supplied BVolume object to this volume.
BVolume&
BVolume::operator=(const BVolume &volume)
{
	if (&volume != this) {
		this->fDevice = volume.fDevice;
		this->fCStatus = volume.fCStatus;
	}
	return *this;
}


// FBC
void BVolume::_TurnUpTheVolume1() {}
void BVolume::_TurnUpTheVolume2() {}
void BVolume::_TurnUpTheVolume3() {}
void BVolume::_TurnUpTheVolume4() {}
void BVolume::_TurnUpTheVolume5() {}
void BVolume::_TurnUpTheVolume6() {}
void BVolume::_TurnUpTheVolume7() {}
void BVolume::_TurnUpTheVolume8() {}
