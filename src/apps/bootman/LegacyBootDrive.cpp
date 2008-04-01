/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "LegacyBootDrive.h"


#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <Partition.h>
#include <Path.h>
#include <String.h>

#include <stdio.h>


class PartitionRecorder : public BDiskDeviceVisitor
{
public:
	PartitionRecorder(BMessage* settings);

	virtual bool Visit(BDiskDevice* device);	
	virtual bool Visit(BPartition* partition, int32 level);

private:
	bool _Record(BPartition* partition);

	BMessage* fSettings;
	int32 fIndex;
};


PartitionRecorder::PartitionRecorder(BMessage* settings)
	: fSettings(settings)
{
}


bool
PartitionRecorder::Visit(BDiskDevice* device)
{
	return false;
}


bool
PartitionRecorder::Visit(BPartition* partition, int32 level)
{
	return _Record(partition);
}


bool
PartitionRecorder::_Record(BPartition* partition)
{
	BPath partitionPath;
	partition->GetPath(&partitionPath);

	BString buffer;
	const char* name = partition->ContentName();
	if (name == NULL) {
		fIndex ++;
		buffer << "Unnamed " << fIndex; 
		name = buffer.String();
	}
	
	const char* type = partition->Type();
	if (type == NULL)
		type = "Unknown";
		
	BMessage message;
	message.AddBool("show", true);
	message.AddString("name", name);
	message.AddString("type", type);
	message.AddString("path", partitionPath.Path());
	message.AddInt64("size", partition->ContentSize());	
	fSettings->AddMessage("partition", &message);	

	return false;
}


LegacyBootDrive::LegacyBootDrive()
{
}


LegacyBootDrive::~LegacyBootDrive()
{
}


bool
LegacyBootDrive::IsBootMenuInstalled()
{
	return false;
}


status_t
LegacyBootDrive::ReadPartitions(BMessage *message)
{
	BDiskDeviceRoster diskDeviceRoster;
	BDiskDevice device;
	// first disk only
	if (diskDeviceRoster.GetNextDevice(&device) == B_OK) {
		BPath path;
		if (device.GetPath(&path) != B_OK)
			return B_ERROR;
			
		message->AddString("disk", path.Path());


		PartitionRecorder recorder(message);
		device.VisitEachDescendant(&recorder);
	}	
	return B_OK;
}


status_t
LegacyBootDrive::WriteBootMenu(BMessage *message)
{
	// TODO
	printf("WriteBootMenu:\n");
	message->PrintToStream();
	
	BMessage partition;
	int32 index = 0;
	for (; message->FindMessage("partition", index, &partition) == B_OK; index ++) {
		printf("Partition %d:\n", (int)index);
		partition.PrintToStream();
	}
	
	return B_OK;
}


status_t
LegacyBootDrive::SaveMasterBootRecord(BFile *file)
{
	// TODO implement
	return B_OK;
}


status_t
LegacyBootDrive::RestoreMasterBootRecord(BFile *file)
{
	// TODO implement
	return B_OK;
}
