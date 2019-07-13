/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "LegacyBootMenu.h"

#include <new>

#include <errno.h>
#include <stdio.h>

#include <Catalog.h>
#include <DataIO.h>
#include <DiskDevice.h>
#include <DiskDeviceTypes.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <Drivers.h>
#include <File.h>
#include <Partition.h>
#include <Path.h>
#include <String.h>
#include <UTF8.h>

#include "BootDrive.h"
#include "BootLoader.h"


/*
	Note: for testing, the best way is to create a small file image, and
	register it, for example via the "diskimage" tool (mountvolume might also
	work).
	You can then create partitions on it via DriveSetup, or simply copy an
	existing drive to it, for example via:
		$ dd if=/dev/disk/ata/0/master/raw of=test.image bs=1M count=10

	It will automatically appear in BootManager once it is registered, and,
	depending on its parition layout, you can then install the boot menu on
	it, and directly test it via qemu.
*/


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LegacyBootMenu"


struct MasterBootRecord {
	uint8 bootLoader[440];
	uint8 diskSignature[4];
	uint8 reserved[2];
	uint8 partition[64];
	uint8 signature[2];
};


class LittleEndianMallocIO : public BMallocIO {
public:
			bool				WriteInt8(int8 value);
			bool				WriteInt16(int16 value);
			bool				WriteInt32(int32 value);
			bool				WriteInt64(int64 value);
			bool				WriteString(const char* value);
			bool				Align(int16 alignment);
			bool				Fill(int16 size, int8 fillByte);
};


class PartitionVisitor : public BDiskDeviceVisitor {
public:
								PartitionVisitor();

	virtual	bool				Visit(BDiskDevice* device);
	virtual	bool				Visit(BPartition* partition, int32 level);

			bool				HasPartitions() const;
			off_t				FirstOffset() const;

private:
			off_t				fFirstOffset;
};


class PartitionRecorder : public BDiskDeviceVisitor {
public:
								PartitionRecorder(BMessage& settings,
									int8 biosDrive);

	virtual	bool				Visit(BDiskDevice* device);
	virtual	bool				Visit(BPartition* partition, int32 level);

			bool				FoundPartitions() const;

private:
			BMessage&			fSettings;
			int32				fUnnamedIndex;
			int8				fBIOSDrive;
			bool				fFound;
};


static const uint32 kBlockSize = 512;
static const uint32 kNumberOfBootLoaderBlocks = 4;
	// The number of blocks required to store the
	// MBR including the Haiku boot loader.

static const uint32 kMBRSignature = 0xAA55;

static const int32 kMaxBootMenuItemLength = 70;


bool
LittleEndianMallocIO::WriteInt8(int8 value)
{
	return Write(&value, sizeof(value)) == sizeof(value);
}


bool
LittleEndianMallocIO::WriteInt16(int16 value)
{
	return WriteInt8(value & 0xff)
		&& WriteInt8(value >> 8);
}


bool
LittleEndianMallocIO::WriteInt32(int32 value)
{
	return WriteInt8(value & 0xff)
		&& WriteInt8(value >> 8)
		&& WriteInt8(value >> 16)
		&& WriteInt8(value >> 24);
}


bool
LittleEndianMallocIO::WriteInt64(int64 value)
{
	return WriteInt32(value) && WriteInt32(value >> 32);
}


bool
LittleEndianMallocIO::WriteString(const char* value)
{
	int len = strlen(value) + 1;
	return WriteInt8(len)
		&& Write(value, len) == len;
}


bool
LittleEndianMallocIO::Align(int16 alignment)
{
	if ((Position() % alignment) == 0)
		return true;
	return Fill(alignment - (Position() % alignment), 0);
}


bool
LittleEndianMallocIO::Fill(int16 size, int8 fillByte)
{
	for (int i = 0; i < size; i ++) {
		if (!WriteInt8(fillByte))
			return false;
	}
	return true;
}


// #pragma mark -


PartitionVisitor::PartitionVisitor()
	:
	fFirstOffset(LONGLONG_MAX)
{
}


bool
PartitionVisitor::Visit(BDiskDevice* device)
{
	return false;
}


bool
PartitionVisitor::Visit(BPartition* partition, int32 level)
{
	if (partition->Offset() < fFirstOffset)
		fFirstOffset = partition->Offset();

	return false;
}


bool
PartitionVisitor::HasPartitions() const
{
	return fFirstOffset != LONGLONG_MAX;
}


off_t
PartitionVisitor::FirstOffset() const
{
	return fFirstOffset;
}


// #pragma mark -


PartitionRecorder::PartitionRecorder(BMessage& settings, int8 biosDrive)
	:
	fSettings(settings),
	fUnnamedIndex(0),
	fBIOSDrive(biosDrive),
	fFound(false)
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
	if (partition->ContainsPartitioningSystem())
		return false;

	BPath partitionPath;
	partition->GetPath(&partitionPath);

	BString buffer;
	const char* name = partition->ContentName();
	if (name == NULL) {
		BString number;
		number << ++fUnnamedIndex;
		buffer << B_TRANSLATE_COMMENT("Unnamed %d",
			"Default name of a partition whose name could not be read from "
			"disk; characters in codepage 437 are allowed only");
		buffer.ReplaceFirst("%d", number);
		name = buffer.String();
	}

	const char* type = partition->Type();
	if (type == NULL)
		type = B_TRANSLATE_COMMENT("Unknown", "Text is shown for an unknown "
			"partition type");

	BMessage message;
	// Data as required by BootLoader.h
	message.AddBool("show", true);
	message.AddString("name", name);
	message.AddString("type", type);
	message.AddString("path", partitionPath.Path());
	if (fBIOSDrive != 0)
		message.AddInt8("drive", fBIOSDrive);
	message.AddInt64("size", partition->Size());
	message.AddInt64("offset", partition->Offset());

	fSettings.AddMessage("partition", &message);
	fFound = true;

	return false;
}


bool
PartitionRecorder::FoundPartitions() const
{
	return fFound;
}


// #pragma mark -


LegacyBootMenu::LegacyBootMenu()
{
}


LegacyBootMenu::~LegacyBootMenu()
{
}


bool
LegacyBootMenu::IsInstalled(const BootDrive& drive)
{
	// TODO: detect bootman
	return false;
}


status_t
LegacyBootMenu::CanBeInstalled(const BootDrive& drive)
{
	BDiskDevice device;
	status_t status = drive.GetDiskDevice(device);
	if (status != B_OK)
		return status;

	PartitionVisitor visitor;
	device.VisitEachDescendant(&visitor);

	if (!visitor.HasPartitions()
			|| strcmp(device.ContentType(), kPartitionTypeIntel) != 0)
		return B_ENTRY_NOT_FOUND;

	// Enough space to write boot menu to drive?
	if (visitor.FirstOffset() < (int)sizeof(kBootLoader))
		return B_PARTITION_TOO_SMALL;

	if (device.IsReadOnlyMedia())
		return B_READ_ONLY_DEVICE;

	return B_OK;
}


status_t
LegacyBootMenu::CollectPartitions(const BootDrive& drive, BMessage& settings)
{
	status_t status = B_ERROR;

	// Remove previous partitions, if any
	settings.RemoveName("partition");

	BDiskDeviceRoster diskDeviceRoster;
	BDiskDevice device;
	bool partitionsFound = false;

	while (diskDeviceRoster.GetNextDevice(&device) == B_OK) {
		BPath path;
		status_t status = device.GetPath(&path);
		if (status != B_OK)
			continue;

		// Skip not from BIOS bootable drives that are not the target disk
		int8 biosDrive = 0;
		if (path != drive.Path()
			&& _GetBIOSDrive(path.Path(), biosDrive) != B_OK)
			continue;

		PartitionRecorder recorder(settings, biosDrive);
		device.VisitEachDescendant(&recorder);

		partitionsFound |= recorder.FoundPartitions();
	}

	return partitionsFound ? B_OK : status;
}


status_t
LegacyBootMenu::Install(const BootDrive& drive, BMessage& settings)
{
	int32 defaultPartitionIndex;
	if (settings.FindInt32("defaultPartition", &defaultPartitionIndex) != B_OK)
		return B_BAD_VALUE;

	int32 timeout;
	if (settings.FindInt32("timeout", &timeout) != B_OK)
		return B_BAD_VALUE;

	int fd = open(drive.Path(), O_RDWR);
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

	LittleEndianMallocIO newBootLoader;
	ssize_t size = sizeof(kBootLoader);
	if (newBootLoader.Write(kBootLoader, size) != size) {
		close(fd);
		return B_NO_MEMORY;
	}

	MasterBootRecord* newMBR = (MasterBootRecord*)newBootLoader.Buffer();
	_CopyPartitionTable(newMBR, &oldMBR);

	int menuEntries = 0;
	int defaultMenuEntry = 0;
	BMessage partition;
	int32 index;
	for (index = 0; settings.FindMessage("partition", index,
			&partition) == B_OK; index ++) {
		bool show;
		partition.FindBool("show", &show);
		if (!show)
			continue;
		if (index == defaultPartitionIndex)
			defaultMenuEntry = menuEntries;

		menuEntries ++;
	}
	newBootLoader.WriteInt16(menuEntries);
	newBootLoader.WriteInt16(defaultMenuEntry);
	newBootLoader.WriteInt16(timeout);

	for (index = 0; settings.FindMessage("partition", index,
			&partition) == B_OK; index ++) {
		bool show;
		BString name;
		BString path;
		int64 offset;
		int8 drive;
		partition.FindBool("show", &show);
		partition.FindString("name", &name);
		partition.FindString("path", &path);
		// LegacyBootMenu specific data
		partition.FindInt64("offset", &offset);
		partition.FindInt8("drive", &drive);
		if (!show)
			continue;

		BString biosName;
		_ConvertToBIOSText(name.String(), biosName);

		newBootLoader.WriteString(biosName.String());
		newBootLoader.WriteInt8(drive);
		newBootLoader.WriteInt64(offset / kBlockSize);
	}

	if (!newBootLoader.Align(kBlockSize)) {
		close(fd);
		return B_ERROR;
	}

	lseek(fd, 0, SEEK_SET);
	const uint8* buffer = (const uint8*)newBootLoader.Buffer();
	status_t status = _WriteBlocks(fd, buffer, newBootLoader.Position());
	close(fd);
	return status;
}


status_t
LegacyBootMenu::SaveMasterBootRecord(BMessage* settings, BFile* file)
{
	BString path;

	if (settings->FindString("disk", &path) != B_OK)
		return B_BAD_VALUE;

	int fd = open(path.String(), O_RDONLY);
	if (fd < 0)
		return B_IO_ERROR;

	ssize_t size = kBlockSize * kNumberOfBootLoaderBlocks;
	uint8* buffer = new(std::nothrow) uint8[size];
	if (buffer == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	status_t status = _ReadBlocks(fd, buffer, size);
	if (status != B_OK) {
		close(fd);
		delete[] buffer;
		return B_IO_ERROR;
	}

	MasterBootRecord* mbr = (MasterBootRecord*)buffer;
	if (!_IsValid(mbr)) {
		close(fd);
		delete[] buffer;
		return B_BAD_VALUE;
	}

	if (file->Write(buffer, size) != size)
		status = B_IO_ERROR;
	delete[] buffer;
	close(fd);
	return status;
}


status_t
LegacyBootMenu::RestoreMasterBootRecord(BMessage* settings, BFile* file)
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
	uint8* buffer = new(std::nothrow) uint8[size];
	if (buffer == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	if (file->Read(buffer, size) != (ssize_t)size) {
		close(fd);
		delete[] buffer;
		return B_IO_ERROR;
	}

	MasterBootRecord* newMBR = (MasterBootRecord*)buffer;
	if (!_IsValid(newMBR)) {
		close(fd);
		delete[] buffer;
		return B_BAD_VALUE;
	}

	_CopyPartitionTable(newMBR, &oldMBR);

	status_t status = _WriteBlocks(fd, buffer, size);
	delete[] buffer;
	close(fd);
	return status;
}


status_t
LegacyBootMenu::GetDisplayText(const char* text, BString& displayText)
{
	BString biosText;
	if (!_ConvertToBIOSText(text, biosText)) {
		displayText = "???";
		return B_ERROR;
	}

	// convert back to UTF-8
	int32 biosTextLength = biosText.Length();
	int32 bufferLength = strlen(text);
	char* buffer = displayText.LockBuffer(bufferLength + 1);
	int32 state = 0;
	if (convert_to_utf8(B_MS_DOS_CONVERSION,
		biosText.String(), &biosTextLength,
		buffer, &bufferLength, &state) != B_OK) {
		displayText.UnlockBuffer(0);
		displayText = "???";
		return B_ERROR;
	}

	buffer[bufferLength] = '\0';
	displayText.UnlockBuffer(bufferLength);
	return B_OK;
}


bool
LegacyBootMenu::_ConvertToBIOSText(const char* text, BString& biosText)
{
	// convert text in UTF-8 to 'code page 437'
	int32 textLength = strlen(text);

	int32 biosTextLength = textLength;
	char* buffer = biosText.LockBuffer(biosTextLength + 1);
	if (buffer == NULL) {
		biosText.UnlockBuffer(0);
		return false;
	}

	int32 state = 0;
	if (convert_from_utf8(B_MS_DOS_CONVERSION, text, &textLength,
		buffer, &biosTextLength, &state) != B_OK) {
		biosText.UnlockBuffer(0);
		return false;
	}

	buffer[biosTextLength] = '\0';
	biosText.UnlockBuffer(biosTextLength);
	return biosTextLength < kMaxBootMenuItemLength;
}


status_t
LegacyBootMenu::_GetBIOSDrive(const char* device, int8& drive)
{
	int fd = open(device, O_RDONLY);
	if (fd < 0)
		return errno;

	status_t status = ioctl(fd, B_GET_BIOS_DRIVE_ID, &drive, 1);
	close(fd);
	return status;
}


status_t
LegacyBootMenu::_ReadBlocks(int fd, uint8* buffer, size_t size)
{
	if (size % kBlockSize != 0) {
		fprintf(stderr, "_ReadBlocks buffer size must be a multiple of %d\n",
			(int)kBlockSize);
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
LegacyBootMenu::_WriteBlocks(int fd, const uint8* buffer, size_t size)
{
	if (size % kBlockSize != 0) {
		fprintf(stderr, "_WriteBlocks buffer size must be a multiple of %d\n",
			(int)kBlockSize);
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
LegacyBootMenu::_CopyPartitionTable(MasterBootRecord* destination,
		const MasterBootRecord* source)
{
	memcpy(destination->diskSignature, source->diskSignature,
		sizeof(source->diskSignature) + sizeof(source->reserved)
			+ sizeof(source->partition));
}


bool
LegacyBootMenu::_IsValid(const MasterBootRecord* mbr)
{
	return mbr->signature[0] == (kMBRSignature & 0xff)
		&& mbr->signature[1] == (kMBRSignature >> 8);
}
