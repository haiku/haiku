/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "TestBootDrive.h"


TestBootDrive::TestBootDrive()
{
}


TestBootDrive::~TestBootDrive()
{
}


bool
TestBootDrive::IsBootMenuInstalled(BMessage* settings)
{
	return false;
}


status_t
TestBootDrive::ReadPartitions(BMessage *settings)
{
	settings->AddString("disk", "bootcylinder");

	BMessage partition;

	partition.AddBool("show", true);
	partition.AddString("name", "TEST Haiku");
	partition.AddString("type", "bfs");
	partition.AddString("path", "/dev/disk/ide/ata/0/master/0/0_0");
	partition.AddInt64("size", 3 * (int64)1024 * 1024 * 1024);
	settings->AddMessage("partition", &partition);


	partition.MakeEmpty();
	partition.AddBool("show", true);
	partition.AddString("name", "TEST no name");
	partition.AddString("type", "dos");
	partition.AddString("path", "/dev/disk/ide/ata/0/slave/0/0_0");
	partition.AddInt64("size", 10 * (int64)1024 * 1024 * 1024);
	settings->AddMessage("partition", &partition);

	partition.MakeEmpty();
	partition.AddBool("show", true);
	partition.AddString("name", "TEST Data");
	partition.AddString("type", "unknown");
	partition.AddString("path", "/dev/disk/ide/ata/0/slave/0/0_1");
	partition.AddInt64("size", 250 * (int64)1024 * 1024 * 1024);
	settings->AddMessage("partition", &partition);

	return B_OK;
}


status_t
TestBootDrive::WriteBootMenu(BMessage *settings)
{
	printf("WriteBootMenu:\n");
	settings->PrintToStream();

	BMessage partition;
	int32 index = 0;
	for (; settings->FindMessage("partition", index, &partition) == B_OK;
			index++) {
		printf("Partition %d:\n", (int)index);
		partition.PrintToStream();
	}

	return B_OK;
}


status_t
TestBootDrive::SaveMasterBootRecord(BMessage* settings, BFile* file)
{
	return B_OK;
}


status_t
TestBootDrive::RestoreMasterBootRecord(BMessage* settings, BFile* file)
{
	return B_OK;
}


status_t
TestBootDrive::GetDisplayText(const char* text, BString& displayText)
{
	displayText = text;
}
