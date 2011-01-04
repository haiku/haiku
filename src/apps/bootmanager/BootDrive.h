/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef BOOT_DRIVE_H
#define BOOT_DRIVE_H


#include <File.h>
#include <Message.h>


/*	Setting BMessage Format:

	"disk" String (path to boot disk)

	"partition" array of BMessage:
		"show" bool (flag if entry should be added to boot menu)
		"name" String (the name as shown in boot menu)
		"type" String (short name of file system: bfs, dos)
		"path" String (path to partition in /dev/...)
		"size" long (size of partition in bytes)
*/


enum {
	// Not enough space free before first partition for boot loader
	kErrorBootSectorTooSmall = B_ERRORS_END + 1,
};


class BootDrive {
public:
								BootDrive() {}
	virtual						~BootDrive() {}

	virtual	bool				IsBootMenuInstalled(BMessage* settings) = 0;
	virtual	status_t			ReadPartitions(BMessage* settings) = 0;
	virtual	status_t			WriteBootMenu(BMessage* settings) = 0;
	virtual	status_t			SaveMasterBootRecord(BMessage* settings,
									BFile* file) = 0;
	virtual	status_t			RestoreMasterBootRecord(BMessage* settings,
									BFile* file) = 0;

	// Converts the specified text into a text as it will be shown
	// in the boot menu.
	virtual	status_t			GetDisplayText(const char* text,
									BString& displayText) = 0;
};


#endif	// BOOT_DRIVE_H
