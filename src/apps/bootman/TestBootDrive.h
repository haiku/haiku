/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef TEST_BOOT_DRIVE_H
#define TEST_BOOT_DRIVE_H

#include "BootDrive.h"

class TestBootDrive : public BootDrive
{
public:
	TestBootDrive();
	virtual ~TestBootDrive();

	virtual bool IsBootMenuInstalled(BMessage* settings);
	virtual status_t ReadPartitions(BMessage* settings);
	virtual status_t WriteBootMenu(BMessage* settings);
	virtual status_t SaveMasterBootRecord(BMessage* settings, BFile* file);
	virtual status_t RestoreMasterBootRecord(BMessage* settings, BFile* file);
};

#endif	// TEST_BOOT_DRIVE_H
