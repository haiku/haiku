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
	~TestBootDrive();

	bool IsBootMenuInstalled(BMessage* settings);
	status_t ReadPartitions(BMessage* settings);
	status_t WriteBootMenu(BMessage* settings);
	status_t SaveMasterBootRecord(BMessage* settings, BFile* file);
	status_t RestoreMasterBootRecord(BMessage* settings, BFile* file);
	status_t GetDisplayText(const char* text, BString& displayText);
};

#endif	// TEST_BOOT_DRIVE_H
