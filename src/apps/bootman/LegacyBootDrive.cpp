/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "LegacyBootDrive.h"


#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <Partition.h>
#include <Path.h>
#include <String.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <DataIO.h>
#include <File.h>
#include <String.h>

#include "BootLoader.h"


#define USE_SECOND_DISK 0


class Buffer : public BMallocIO 
{
public:
	Buffer() : BMallocIO() {}
	bool WriteInt8(int8 value);
	bool WriteInt16(int16 value);
	bool WriteInt32(int32 value);
	bool WriteInt64(int64 value);
	bool WriteString(const char* value);
	bool Align(int16 alignment);
	bool Fill(int16 size, int8 fillByte);
};


bool
Buffer::WriteInt8(int8 value) 
{
	return Write(&value, sizeof(value)) == sizeof(value);
}


bool
Buffer::WriteInt16(int16 value)
{
	return WriteInt8(value & 0xff) && WriteInt8(value >> 8);
}


bool
Buffer::WriteInt32(int32 value)
{
	return WriteInt8(value & 0xff) && WriteInt8(value >> 8) &&
		WriteInt8(value >> 16) && WriteInt8(value >> 24);
}


bool
Buffer::WriteInt64(int64 value)
{
	return WriteInt32(value) && WriteInt32(value >> 32);
}


bool
Buffer::WriteString(const char* value)
{
	int len = strlen(value) + 1;
	return WriteInt8(len) && Write(value, len) == len;
}


bool
Buffer::Align(int16 alignment)
{
	if ((Position() % alignment) == 0)
		return true;
	return Fill(alignment - (Position() % alignment), 0);
}


bool
Buffer::Fill(int16 size, int8 fillByte)
{
	for (int i = 0; i < size; i ++) {
		if (!WriteInt8(fillByte))
			return false;
	}
	return true;
}


class PartitionRecorder : public BDiskDeviceVisitor
{
public:
	PartitionRecorder(BMessage* settings);

	virtual bool Visit(BDiskDevice* device);	
	virtual bool Visit(BPartition* partition, int32 level);

	bool HasPartitions() const;	
	off_t FirstOffset() const;
	
private:
	bool _Record(BPartition* partition);

	BMessage* fSettings;
	int32 fIndex;
	off_t fFirstOffset;
};


PartitionRecorder::PartitionRecorder(BMessage* settings)
	: fSettings(settings)
	, fIndex(0)
	, fFirstOffset((off_t)-1)
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
PartitionRecorder::HasPartitions() const
{
	return fFirstOffset != (off_t)-1;
}


off_t
PartitionRecorder::FirstOffset() const
{
	return fFirstOffset;
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
	// Data as required by BootLoader.h
	message.AddBool("show", true);
	message.AddString("name", name);
	message.AddString("type", type);
	message.AddString("path", partitionPath.Path());
	message.AddInt64("size", partition->ContentSize());	
	// Specific data
	message.AddInt64("offset", partition->Offset());
	
	fSettings->AddMessage("partition", &message);

	if (partition->Offset() < fFirstOffset)
		fFirstOffset = partition->Offset();
	
	return false;
}


LegacyBootDrive::LegacyBootDrive()
{
}


LegacyBootDrive::~LegacyBootDrive()
{
}


bool
LegacyBootDrive::IsBootMenuInstalled(BMessage* settings)
{
	// TODO detect bootman
	return false;
}


status_t
LegacyBootDrive::ReadPartitions(BMessage *settings)
{
	BDiskDeviceRoster diskDeviceRoster;
	BDiskDevice device;
	bool diskFound = false;
	while (diskDeviceRoster.GetNextDevice(&device) == B_OK) {
		BPath path;
		if (device.GetPath(&path) != B_OK)
			return B_ERROR;
		
		PartitionRecorder recorder(settings);
		device.VisitEachDescendant(&recorder);

		if (!diskFound) {
			settings->AddString("disk", path.Path());
			diskFound = true;

			#if not USE_SECOND_DISK
				// ignored in test build
				off_t size = sizeof(kBootLoader);
				if (!recorder.HasPartitions() || recorder.FirstOffset() < size)
					return kErrorBootSectorTooSmall;
			#endif
		}
	}	

	#if USE_SECOND_DISK
		// for testing only write boot menu to second hdd
		settings->ReplaceString("disk", "/dev/disk/ata/1/master/raw");
	#endif
	
	if (diskFound)
		return B_OK;
	else
		return B_ERROR;
}


status_t
LegacyBootDrive::WriteBootMenu(BMessage *settings)
{
	printf("WriteBootMenu:\n");
	settings->PrintToStream();

	BString path;
	if (settings->FindString("disk", &path) != B_OK)
		return B_BAD_VALUE;
	
	int32 defaultPartitionIndex;
	if (settings->FindInt32("defaultPartition", &defaultPartitionIndex) != B_OK)
		return B_BAD_VALUE;
	
	int32 timeout;
	if (settings->FindInt32("timeout", &timeout) != B_OK)
		return B_BAD_VALUE;
	
	int fd = open(path.String(), O_RDWR);
	if (fd < 0)
		return B_IO_ERROR;
	
	MasterBootRecord oldMBR;
	if (read(fd, &oldMBR, sizeof(oldMBR)) != sizeof(oldMBR)) {
		close(fd);
		return B_IO_ERROR;
	}
	
	if (!_IsValid(&oldMBR)) {
		close(fd);
		return B_BAD_VALUE;
	}
	
	Buffer newBootLoader;
	ssize_t size = sizeof(kBootLoader);
	if (newBootLoader.Write(kBootLoader, size) != size) {
		close(fd);
		return B_NO_MEMORY;
	}
	
	MasterBootRecord* newMBR = (MasterBootRecord*)newBootLoader.BMallocIO::Buffer();
	_CopyPartitionTable(newMBR, &oldMBR);
	
	int menuEntries = 0;
	BMessage partition;
	int32 index;
	for (index = 0; settings->FindMessage("partition", index, &partition) == B_OK; index ++) {
		bool show;
		partition.FindBool("show", &show);
		if (!show)
			continue;
		
		menuEntries ++;
	}
	newBootLoader.WriteInt16(menuEntries);
	newBootLoader.WriteInt16(defaultPartitionIndex);
	newBootLoader.WriteInt16(timeout);
	
	
	for (index = 0; settings->FindMessage("partition", index, &partition) == B_OK; index ++) {
		bool show;
		BString name;
		BString path;
		int64 offset;
		partition.FindBool("show", &show);
		partition.FindString("name", &name);
		partition.FindString("path", &path);
		// LegacyBootDrive provides offset in message as well
		partition.FindInt64("offset", &offset);
		if (!show)
			continue;
		
		newBootLoader.WriteString(name.String());
		newBootLoader.WriteInt64(offset / kBlockSize);
	}	

	if (!newBootLoader.Align(kBlockSize)) {
		close(fd);
		return B_ERROR;
	}
	
	lseek(fd, 0, SEEK_SET);
	const uint8* buffer = (uint8*)newBootLoader.BMallocIO::Buffer();
	status_t status = _WriteBlocks(fd, buffer, newBootLoader.Position());
	close(fd);
	return status;
}


status_t
LegacyBootDrive::SaveMasterBootRecord(BMessage* settings, BFile* file)
{	
	BString path;

	if (settings->FindString("disk", &path) != B_OK)
		return B_BAD_VALUE;
	
	int fd = open(path.String(), O_RDONLY);
	if (fd < 0)
		return B_IO_ERROR;
	
	ssize_t size = kBlockSize * kNumberOfBootLoaderBlocks;
	uint8* buffer = new uint8[size];
	if (buffer == NULL) {
		close(fd);		
		return B_NO_MEMORY;
	}

	status_t status = _ReadBlocks(fd, buffer, size);
	if (status != B_OK) {
		close(fd);
		delete buffer;
		return B_IO_ERROR;
	}		

	MasterBootRecord* mbr = (MasterBootRecord*)buffer;
	if (!_IsValid(mbr)) {
		close(fd);
		delete buffer;
		return B_BAD_VALUE;
	}

	if (file->Write(buffer, size) != size)
		status = B_IO_ERROR;
	delete buffer;		
	close(fd);
	return status;
}


status_t
LegacyBootDrive::RestoreMasterBootRecord(BMessage* settings, BFile* file)
{
	BString path;
	if (settings->FindString("disk", &path) != B_OK)
		return B_BAD_VALUE;
	
	int fd = open(path.String(), O_RDWR);
	if (fd < 0)
		return B_IO_ERROR;
	
	MasterBootRecord oldMBR;
	if (read(fd, &oldMBR, sizeof(oldMBR)) != sizeof(oldMBR)) {
		close(fd);
		return B_IO_ERROR;	
	}
	if (!_IsValid(&oldMBR)) {
		close(fd);
		return B_BAD_VALUE;
	}
	
	lseek(fd, 0, SEEK_SET);	
	
	size_t size = kBlockSize * kNumberOfBootLoaderBlocks;
	uint8* buffer = new uint8[size];
	if (buffer == NULL) {
		close(fd);		
		return B_NO_MEMORY;
	}

	if (file->Read(buffer, size) != (ssize_t)size) {
		close(fd);
		delete buffer;
		return B_IO_ERROR;
	}		

	MasterBootRecord* newMBR = (MasterBootRecord*)buffer;
	if (!_IsValid(newMBR)) {
		close(fd);
		delete buffer;
		return B_BAD_VALUE;
	}
	
	_CopyPartitionTable(newMBR, &oldMBR);

	status_t status = _WriteBlocks(fd, buffer, size);
	delete buffer;		
	close(fd);
	return status;
}


status_t
LegacyBootDrive::_ReadBlocks(int fd, uint8* buffer, size_t size)
{
	if (size % kBlockSize != 0) {
		fprintf(stderr, "_ReadBlocks buffer size must be a multiple of %d\n", (int)kBlockSize);
		return B_BAD_VALUE;
	}
	const size_t blocks = size / kBlockSize;
	uint8* block = buffer;
	for (size_t i = 0; i < blocks; i ++, block += kBlockSize) {
		if (read(fd, block, kBlockSize) != (ssize_t)kBlockSize)
			return B_IO_ERROR;
	}
	return B_OK;
}


status_t
LegacyBootDrive::_WriteBlocks(int fd, const uint8* buffer, size_t size)
{
	if (size % kBlockSize != 0) {
		fprintf(stderr, "_WriteBlocks buffer size must be a multiple of %d\n", (int)kBlockSize);
		return B_BAD_VALUE;
	}
	const size_t blocks = size / kBlockSize;
	const uint8* block = buffer;
	for (size_t i = 0; i < blocks; i ++, block += kBlockSize) {
		if (write(fd, block, kBlockSize) != (ssize_t)kBlockSize)
			return B_IO_ERROR;
	}
	return B_OK;
}


void
LegacyBootDrive::_CopyPartitionTable(MasterBootRecord* destination,
		const MasterBootRecord* source)
{
	memcpy(destination->partition, source->partition, sizeof(source->partition));
}


bool
LegacyBootDrive::_IsValid(const MasterBootRecord* mbr)
{
	return  mbr->signature[0] == (kMBRSignature & 0xff) &&
		mbr->signature[1] == (kMBRSignature >> 8);
}
