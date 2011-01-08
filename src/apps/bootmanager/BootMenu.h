/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef BOOT_MENU_H
#define BOOT_MENU_H


#include <File.h>
#include <Message.h>
#include <ObjectList.h>


class BootDrive;


/*	Setting BMessage Format:

	"partition" array of BMessage:
		"show" bool (flag if entry should be added to boot menu)
		"name" String (the name as shown in boot menu)
		"type" String (short name of file system: bfs, dos)
		"path" String (path to partition in /dev/...)
		"size" long (size of partition in bytes)
*/


class BootMenu {
public:
								BootMenu() {}
	virtual						~BootMenu() {}

	virtual	bool				IsInstalled(const BootDrive& drive) = 0;
	virtual	status_t			CanBeInstalled(const BootDrive& drive) = 0;

	virtual	status_t			CollectPartitions(const BootDrive& drive,
									BMessage& settings) = 0;

	virtual	status_t			Install(const BootDrive& drive,
									BMessage& settings) = 0;
	virtual	status_t			SaveMasterBootRecord(BMessage* settings,
									BFile* file) = 0;
	virtual	status_t			RestoreMasterBootRecord(BMessage* settings,
									BFile* file) = 0;

	// Converts the specified text into a text as it will be shown
	// in the boot menu.
	virtual	status_t			GetDisplayText(const char* text,
									BString& displayText) = 0;
};

typedef BObjectList<BootMenu> BootMenuList;


#endif	// BOOT_MENU_H
