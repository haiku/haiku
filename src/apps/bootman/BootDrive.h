/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef BOOT_DRIVE_H
#define BOOT_DRIVE_H

#include <File.h>
#include <Message.h>

/*
	Setting BMessage Format:

	"disk" String (path to boot disk)

	"partition" array of BMessage:
		"show" bool (flag if entry should be added to boot menu)
		"name" String (the name as shown in boot menu)
		"type" String (short name of file system: bfs, dos)
		"path" String (path to partition in /dev/...)
		"size" long (size of partition in bytes)
 */
class BootDrive 
{
public:
	BootDrive() {}
	virtual ~BootDrive() {}

	virtual bool IsBootMenuInstalled() = 0;
	virtual status_t ReadPartitions(BMessage *message) = 0;
	virtual status_t WriteBootMenu(BMessage *message) = 0;
	virtual status_t SaveMasterBootRecord(BFile *file) = 0;
	virtual status_t RestoreMasterBootRecord(BFile *file) = 0;
};

#endif	// BOOT_DRIVE_H
