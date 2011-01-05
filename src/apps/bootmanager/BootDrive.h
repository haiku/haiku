/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_DRIVE_H
#define BOOT_DRIVE_H


#include <File.h>


class BootDrive {
public:
								BootDrive();
	virtual						~BootDrive();

			bool				IsBootMenuInstalled() const;
			bool				CanBootMenuBeInstalled() const;

			bool				IsBootDrive() const;

			status_t			GetStream(BPositionIO* stream);
			status_t			GetBIOSDrive(uint8* drive);

private:
			BPath				fPath;
};


#endif	// BOOT_DRIVE_H
