/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LEGACY_BOOT_DRIVE_H
#define LEGACY_BOOT_DRIVE_H

#include "BootDrive.h"

class LegacyBootDrive : public BootDrive
{
public:
	LegacyBootDrive();
	virtual ~LegacyBootDrive();

	virtual bool IsBootMenuInstalled();
	virtual status_t ReadPartitions(BMessage *message);
	virtual status_t WriteBootMenu(BMessage *message);
	virtual status_t SaveMasterBootRecord(BFile *file);
	virtual status_t RestoreMasterBootRecord(BFile *file);
};

#endif	// LEGACY_BOOT_DRIVE_H
