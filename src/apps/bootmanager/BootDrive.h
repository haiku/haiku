/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_DRIVE_H
#define BOOT_DRIVE_H


#include <Path.h>

#include "BootMenu.h"


class BDiskDevice;


class BootDrive {
public:
								BootDrive(const char* path);
	virtual						~BootDrive();

			BootMenu*			InstalledMenu(
									const BootMenuList& menus) const;
			status_t			CanMenuBeInstalled(
									const BootMenuList& menus) const;
			void				AddSupportedMenus(const BootMenuList& from,
									BootMenuList& to);

			const char*			Path() const;
			bool				IsBootDrive() const;

			status_t			GetDiskDevice(BDiskDevice& device) const;

private:
			BPath				fPath;
};


#endif	// BOOT_DRIVE_H
