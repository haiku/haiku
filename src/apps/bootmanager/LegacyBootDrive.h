/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef LEGACY_BOOT_DRIVE_H
#define LEGACY_BOOT_DRIVE_H


#include "BootDrive.h"

#include <SupportDefs.h>


struct MasterBootRecord;


class LegacyBootDrive : public BootDrive {
public:
								LegacyBootDrive();
	virtual						~LegacyBootDrive();

	virtual	bool				IsBootMenuInstalled(BMessage* settings);
	virtual	status_t			ReadPartitions(BMessage* settings);
	virtual	status_t			WriteBootMenu(BMessage* settings);
	virtual	status_t			SaveMasterBootRecord(BMessage* settings,
									BFile* file);
	virtual	status_t			RestoreMasterBootRecord(BMessage* settings,
									BFile* file);
	virtual	status_t			GetDisplayText(const char* text,
									BString& displayText);

private:
			bool				_ConvertToBIOSText(const char* text,
									BString& biosText);
			bool				_GetBiosDrive(const char* device, int8* drive);
			status_t			_ReadBlocks(int fd, uint8* buffer, size_t size);
			status_t			_WriteBlocks(int fd, const uint8* buffer,
									size_t size);
			void				_CopyPartitionTable(
									MasterBootRecord* destination,
									const MasterBootRecord* source);
			bool				_IsValid(const MasterBootRecord* mbr);
};


#endif	// LEGACY_BOOT_DRIVE_H
