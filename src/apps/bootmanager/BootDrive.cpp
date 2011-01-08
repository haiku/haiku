/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BootDrive.h"

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Volume.h>
#include <VolumeRoster.h>


BootDrive::BootDrive(const char* path)
	:
	fPath(path)
{
}


BootDrive::~BootDrive()
{
}


BootMenu*
BootDrive::InstalledMenu(const BootMenuList& menus) const
{
	for (int32 i = 0; i < menus.CountItems(); i++) {
		BootMenu* menu = menus.ItemAt(i);
		if (menu->IsInstalled(*this))
			return menu;
	}
	return NULL;
}


status_t
BootDrive::CanMenuBeInstalled(const BootMenuList& menus) const
{
	status_t status = B_ERROR;

	for (int32 i = 0; i < menus.CountItems(); i++) {
		status = menus.ItemAt(i)->CanBeInstalled(*this);
		if (status == B_OK)
			return status;
	}
	return status;
}


/*!	Adds all boot menus from the list \a from that support the drive to \a to.
*/
void
BootDrive::AddSupportedMenus(const BootMenuList& from, BootMenuList& to)
{
	for (int32 i = 0; i < from.CountItems(); i++) {
		BootMenu* menu = from.ItemAt(i);
		if (menu->CanBeInstalled(*this))
			to.AddItem(menu);
	}
}


const char*
BootDrive::Path() const
{
	return fPath.Path();
}


bool
BootDrive::IsBootDrive() const
{
	BVolumeRoster volumeRoster;
	BVolume volume;
	if (volumeRoster.GetBootVolume(&volume) != B_OK)
		return false;

	BDiskDeviceRoster roster;
	BDiskDevice device;
	if (roster.FindPartitionByVolume(volume, &device, NULL) == B_OK) {
		BPath path;
		if (device.GetPath(&path) == B_OK && path == fPath)
			return true;
	}

	return false;
}


status_t
BootDrive::GetDiskDevice(BDiskDevice& device) const
{
	BDiskDeviceRoster roster;
	return roster.GetDeviceForPath(fPath.Path(), &device);
}
