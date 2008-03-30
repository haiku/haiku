/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_BOOT_DRIVE_H
#define TEST_BOOT_DRIVER_H

#include "BootDrive.h"

class TestBootDrive : public BootDrive
{
public:
	TestBootDrive();
	virtual ~TestBootDrive();

	virtual bool IsBootMenuInstalled();
	virtual status_t ReadPartitions(BMessage *message);
	virtual status_t WriteBootMenu(BMessage *message);
	virtual status_t SaveMasterBootRecord(BFile *file);
	virtual status_t RestoreMasterBootRecord(BFile *file);
};

#endif	// TEST_BOOT_DRIVER_H
