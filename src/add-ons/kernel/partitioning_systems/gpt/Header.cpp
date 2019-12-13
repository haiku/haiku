/*
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "Header.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <KernelExport.h>

#ifdef _KERNEL_MODE
#	include <util/kernel_cpp.h>
#else
#	include <new>
#endif

#ifndef _BOOT_MODE
#include "PartitionMap.h"
#include "PartitionMapWriter.h"
#endif

#if !defined(_BOOT_MODE) && !defined(_USER_MODE)
#include "uuid.h"
#endif

#include "crc32.h"
#include "utility.h"


#define TRACE_EFI_GPT
#ifdef TRACE_EFI_GPT
#	ifndef _KERNEL_MODE
#		define dprintf printf
#	endif
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


namespace EFI {


Header::Header(int fd, uint64 lastBlock, uint32 blockSize)
	:
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL),
	fDirty(false)
{
	// TODO: check the correctness of the protective MBR and warn if invalid

	// Read and check the partition table header

	fStatus = _Read(fd, (uint64)EFI_HEADER_LOCATION * blockSize,
		&fHeader, sizeof(gpt_table_header));
	if (fStatus == B_OK) {
		if (!_IsHeaderValid(fHeader, EFI_HEADER_LOCATION))
			fStatus = B_BAD_DATA;
	}

	if (fStatus == B_OK && lastBlock != fHeader.AlternateBlock()) {
		dprintf("gpt: alternate header not in last block (%" B_PRIu64 " vs. %"
			B_PRIu64 ")\n", fHeader.AlternateBlock(), lastBlock);
		lastBlock = fHeader.AlternateBlock();
	}

	// Read backup header, too
	status_t status = _Read(fd, lastBlock * blockSize, &fBackupHeader,
		sizeof(gpt_table_header));
	if (status == B_OK) {
		if (!_IsHeaderValid(fBackupHeader, lastBlock))
			status = B_BAD_DATA;
	}

	// If both headers are invalid, bail out -- this is probably not a GPT disk
	if (status != B_OK && fStatus != B_OK)
		return;

	if (fStatus != B_OK) {
		// Recreate primary header from the backup
		fHeader = fBackupHeader;
		fHeader.SetAbsoluteBlock(EFI_HEADER_LOCATION);
		fHeader.SetEntriesBlock(EFI_PARTITION_ENTRIES_BLOCK);
		fHeader.SetAlternateBlock(lastBlock);
		fDirty = true;
	} else if (status != B_OK) {
		// Recreate backup header from primary
		_SetBackupHeaderFromPrimary(lastBlock);
	}

	// allocate, read, and check partition entry array

	fEntries = new (std::nothrow) uint8[_EntryArraySize()];
	if (fEntries == NULL) {
		// TODO: if there cannot be allocated enough (ie. the boot loader's
		//	heap is limited), try a smaller size before failing
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = _Read(fd, fHeader.EntriesBlock() * blockSize,
		fEntries, _EntryArraySize());
	if (fStatus != B_OK || !_ValidateEntriesCRC()) {
		// Read backup entries instead
		fStatus = _Read(fd, fBackupHeader.EntriesBlock() * blockSize,
			fEntries, _EntryArraySize());
		if (fStatus != B_OK)
			return;

		if (!_ValidateEntriesCRC()) {
			fStatus = B_BAD_DATA;
			return;
		}
	}

	// TODO: check overlapping or out of range partitions

#ifdef TRACE_EFI_GPT
	_Dump(fHeader);
	_Dump(fBackupHeader);
	_DumpPartitions();
#endif

	fStatus = B_OK;
}


#if !defined(_BOOT_MODE) && !defined(_USER_MODE)
Header::Header(uint64 lastBlock, uint32 blockSize)
	:
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL),
	fDirty(true)
{
	TRACE(("EFI::Header: Initialize GPT, block size %" B_PRIu32 "\n",
		blockSize));

	// Initialize to an empty header
	memcpy(fHeader.header, EFI_PARTITION_HEADER, sizeof(fHeader.header));
	fHeader.SetRevision(EFI_TABLE_REVISION);
	fHeader.SetHeaderSize(sizeof(fHeader));
	fHeader.SetHeaderCRC(0);
	fHeader.SetAbsoluteBlock(EFI_HEADER_LOCATION);
	fHeader.SetAlternateBlock(lastBlock);
	uuid_t uuid;
	uuid_generate_random(uuid);
	memcpy((uint8*)&fHeader.disk_guid, uuid, sizeof(guid_t));
	fHeader.SetEntriesBlock(EFI_PARTITION_ENTRIES_BLOCK);
	fHeader.SetEntryCount(EFI_PARTITION_ENTRY_COUNT);
	fHeader.SetEntrySize(EFI_PARTITION_ENTRY_SIZE);
	fHeader.SetEntriesCRC(0);

	size_t arraySize = _EntryArraySize();
	fEntries = new (std::nothrow) uint8[arraySize];
	if (fEntries == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	memset(fEntries, 0, arraySize);
		// TODO: initialize the entry guids

	uint32 entryBlocks = (arraySize + fBlockSize - 1) / fBlockSize;
	fHeader.SetFirstUsableBlock(EFI_PARTITION_ENTRIES_BLOCK + entryBlocks);
	fHeader.SetLastUsableBlock(lastBlock - 1 - entryBlocks);

	_SetBackupHeaderFromPrimary(lastBlock);

#ifdef TRACE_EFI_GPT
	_Dump(fHeader);
	_DumpPartitions();
#endif

	fStatus = B_OK;
}
#endif // !_BOOT_MODE && !_USER_MODE


Header::~Header()
{
	delete[] fEntries;
}


status_t
Header::InitCheck() const
{
	return fStatus;
}


bool
Header::IsDirty() const
{
	return fDirty;
}


#ifndef _BOOT_MODE
status_t
Header::WriteEntry(int fd, uint32 entryIndex)
{
	off_t entryOffset = entryIndex * fHeader.EntrySize();

	status_t status = _Write(fd,
		fHeader.EntriesBlock() * fBlockSize + entryOffset,
		fEntries + entryOffset, fHeader.EntrySize());
	if (status != B_OK)
		return status;

	// Update header, too -- the entries CRC changed
	status = _WriteHeader(fd);

	// Write backup
	status_t backupStatus = _Write(fd,
		fBackupHeader.EntriesBlock() * fBlockSize + entryOffset,
		fEntries + entryOffset, fHeader.EntrySize());

	if (status == B_OK && backupStatus == B_OK)
		fDirty = false;
	return status == B_OK ? backupStatus : status;
}


status_t
Header::Write(int fd)
{
	// Try to write the protective MBR
	PartitionMap partitionMap;
	PrimaryPartition *partition = NULL;
	uint32 index = 0;
	while ((partition = partitionMap.PrimaryPartitionAt(index)) != NULL) {
		if (index == 0) {
			uint64 deviceSize = fHeader.AlternateBlock() * fBlockSize;
			partition->SetTo(fBlockSize, deviceSize, 0xEE, false, fBlockSize);
		} else
			partition->Unset();
		++index;
	}
	PartitionMapWriter writer(fd, fBlockSize);
	writer.WriteMBR(&partitionMap, true);
		// We also write the bootcode, so we can boot GPT disks from BIOS

	status_t status = _Write(fd, fHeader.EntriesBlock() * fBlockSize, fEntries,
		_EntryArraySize());
	if (status != B_OK)
		return status;

	// First write the header, so that we have at least one completely correct
	// data set
	status = _WriteHeader(fd);


	// Write backup entries
	status_t backupStatus = _Write(fd,
		fBackupHeader.EntriesBlock() * fBlockSize, fEntries, _EntryArraySize());

	if (status == B_OK && backupStatus == B_OK)
		fDirty = false;
	return status == B_OK ? backupStatus : status;
}


status_t
Header::_WriteHeader(int fd)
{
	_UpdateCRC();

	status_t status = _Write(fd, fHeader.AbsoluteBlock() * fBlockSize,
		&fHeader, sizeof(gpt_table_header));
	if (status != B_OK)
		return status;

	return _Write(fd, fBackupHeader.AbsoluteBlock() * fBlockSize,
		&fBackupHeader, sizeof(gpt_table_header));
}


status_t
Header::_Write(int fd, off_t offset, const void* data, size_t size) const
{
	ssize_t bytesWritten = write_pos(fd, offset, data, size);
	if (bytesWritten < 0)
		return bytesWritten;
	if (bytesWritten != (ssize_t)size)
		return B_IO_ERROR;

	return B_OK;
}


void
Header::_UpdateCRC()
{
	_UpdateCRC(fHeader);
	_UpdateCRC(fBackupHeader);
}


void
Header::_UpdateCRC(gpt_table_header& header)
{
	header.SetEntriesCRC(crc32(fEntries, _EntryArraySize()));
	header.SetHeaderCRC(0);
	header.SetHeaderCRC(crc32((uint8*)&header, sizeof(gpt_table_header)));
}
#endif // !_BOOT_MODE


status_t
Header::_Read(int fd, off_t offset, void* data, size_t size) const
{
	ssize_t bytesRead = read_pos(fd, offset, data, size);
	if (bytesRead < 0)
		return bytesRead;
	if (bytesRead != (ssize_t)size)
		return B_IO_ERROR;

	return B_OK;
}


bool
Header::_IsHeaderValid(gpt_table_header& header, uint64 block)
{
	return !memcmp(header.header, EFI_PARTITION_HEADER, sizeof(header.header))
		&& _ValidateHeaderCRC(header)
		&& header.AbsoluteBlock() == block;
}


bool
Header::_ValidateHeaderCRC(gpt_table_header& header)
{
	uint32 originalCRC = header.HeaderCRC();
	header.SetHeaderCRC(0);

	bool matches = originalCRC == crc32((const uint8*)&header,
		sizeof(gpt_table_header));

	header.SetHeaderCRC(originalCRC);
	return matches;
}


bool
Header::_ValidateEntriesCRC() const
{
	return fHeader.EntriesCRC() == crc32(fEntries, _EntryArraySize());
}


void
Header::_SetBackupHeaderFromPrimary(uint64 lastBlock)
{
	fBackupHeader = fHeader;
	fBackupHeader.SetAbsoluteBlock(lastBlock);
	fBackupHeader.SetEntriesBlock(
		lastBlock - _EntryArraySize() / fBlockSize);
	fBackupHeader.SetAlternateBlock(1);
}


#ifdef TRACE_EFI_GPT
const char *
Header::_PrintGUID(const guid_t &id)
{
	static char guid[48];
	snprintf(guid, sizeof(guid),
		"%08" B_PRIx32 "-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		B_LENDIAN_TO_HOST_INT32(id.data1), B_LENDIAN_TO_HOST_INT16(id.data2),
		B_LENDIAN_TO_HOST_INT16(id.data3), id.data4[0], id.data4[1],
		id.data4[2], id.data4[3], id.data4[4], id.data4[5], id.data4[6],
		id.data4[7]);
	return guid;
}


void
Header::_Dump(const gpt_table_header& header)
{
	dprintf("EFI header: %.8s\n", header.header);
	dprintf("EFI revision: %" B_PRIx32 "\n", header.Revision());
	dprintf("header size: %" B_PRId32 "\n", header.HeaderSize());
	dprintf("header CRC: %" B_PRIx32 "\n", header.HeaderCRC());
	dprintf("absolute block: %" B_PRIu64 "\n", header.AbsoluteBlock());
	dprintf("alternate block: %" B_PRIu64 "\n", header.AlternateBlock());
	dprintf("first usable block: %" B_PRIu64 "\n", header.FirstUsableBlock());
	dprintf("last usable block: %" B_PRIu64 "\n", header.LastUsableBlock());
	dprintf("disk GUID: %s\n", _PrintGUID(header.disk_guid));
	dprintf("entries block: %" B_PRIu64 "\n", header.EntriesBlock());
	dprintf("entry size:  %" B_PRIu32 "\n", header.EntrySize());
	dprintf("entry count: %" B_PRIu32 "\n", header.EntryCount());
	dprintf("entries CRC: %" B_PRIx32 "\n", header.EntriesCRC());
}


void
Header::_DumpPartitions()
{
	for (uint32 i = 0; i < EntryCount(); i++) {
		const gpt_partition_entry &entry = EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		dprintf("[%3" B_PRIu32 "] partition type: %s\n", i,
			_PrintGUID(entry.partition_type));
		dprintf("      unique id: %s\n", _PrintGUID(entry.unique_guid));
		dprintf("      start block: %" B_PRIu64 "\n", entry.StartBlock());
		dprintf("      end block: %" B_PRIu64 "\n", entry.EndBlock());
		dprintf("      size: %g MB\n", (entry.EndBlock() - entry.StartBlock())
			* 512 / 1024.0 / 1024.0);
		dprintf("      attributes: %" B_PRIx64 "\n", entry.Attributes());

		char name[64];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		dprintf("      name: %s\n", name);
	}
}
#endif	// TRACE_EFI_GPT


}	// namespace EFI
