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


Header::Header(int fd, off_t block, uint32 blockSize)
	:
	fBlock(block),
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL)
{
	// TODO: check the correctness of the protective MBR

	// read and check the partition table header

	ssize_t bytesRead = read_pos(fd, block * blockSize, &fHeader,
		sizeof(efi_table_header));
	if (bytesRead != (ssize_t)sizeof(efi_table_header)) {
		if (bytesRead < B_OK)
			fStatus = bytesRead;
		else
			fStatus = B_IO_ERROR;

		return;
	}

	if (memcmp(fHeader.header, EFI_PARTITION_HEADER, sizeof(fHeader.header))
		|| !_ValidateHeaderCRC()
		|| fHeader.AbsoluteBlock() != fBlock) {
		// TODO: check that partition counts are in valid bounds
		fStatus = B_BAD_DATA;
		return;
	}

	// allocate, read, and check partition entry array

	fEntries = new (std::nothrow) uint8[_EntryArraySize()];
	if (fEntries == NULL) {
		// TODO: if there cannot be allocated enough (ie. the boot loader's
		//	heap is limited), try a smaller size before failing
		fStatus = B_NO_MEMORY;
		return;
	}

	bytesRead = read_pos(fd, fHeader.EntriesBlock() * blockSize,
		fEntries, _EntryArraySize());
	if (bytesRead != (ssize_t)_EntryArraySize()) {
		if (bytesRead < B_OK)
			fStatus = bytesRead;
		else
			fStatus = B_IO_ERROR;

		return;
	}

	if (!_ValidateEntriesCRC()) {
		// TODO: check overlapping or out of range partitions
		fStatus = B_BAD_DATA;
		return;
	}

#ifdef TRACE_EFI_GPT
	_Dump();
	_DumpPartitions();
#endif

	fStatus = B_OK;
}


#ifndef _BOOT_MODE
Header::Header(off_t block, off_t lastBlock, uint32 blockSize)
	:
	fBlock(block),
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL)
{
	// initialize to an empty header
	memcpy(fHeader.header, EFI_PARTITION_HEADER, sizeof(fHeader.header));
	fHeader.SetRevision(EFI_TABLE_REVISION);
	fHeader.SetHeaderSize(sizeof(fHeader));
	fHeader.SetHeaderCRC(0);
	fHeader.SetAbsoluteBlock(fBlock);
	fHeader.SetAlternateBlock(0); // TODO
	// TODO: set disk guid
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

#ifdef TRACE_EFI_GPT
	_Dump();
	_DumpPartitions();
	dprintf("GPT: HERE I AM!\n");
#else
	dprintf("GPT: Nope!\n");
#endif

	fStatus = B_OK;
}
#endif // !_BOOT_MODE


Header::~Header()
{
	delete[] fEntries;
}


status_t
Header::InitCheck() const
{
	return fStatus;
}


#ifndef _BOOT_MODE
status_t
Header::WriteEntry(int fd, uint32 entryIndex)
{
	// Determine block to write
	off_t block = fHeader.EntriesBlock()
		+ entryIndex * fHeader.EntrySize() / fBlockSize;
	uint32 entryOffset = entryIndex * fHeader.EntrySize() % fBlockSize;

	status_t status = _Write(fd, block * fBlockSize, fEntries + entryOffset,
		fBlockSize);
	if (status != B_OK)
		return status;

	// TODO: write mirror at the end

	// Update header, too -- the entries CRC changed
	return Write(fd);
}


status_t
Header::Write(int fd)
{
	_UpdateCRC();

	status_t status = _Write(fd, fHeader.AbsoluteBlock() * fBlockSize,
		&fHeader, sizeof(efi_table_header));
	if (status != B_OK)
		return status;

	// TODO: write mirror at the end

	return B_OK;
}
#endif // !_BOOT_MODE


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
	fHeader.SetEntriesCRC(crc32(fEntries, _EntryArraySize()));
	fHeader.SetHeaderCRC(0);
	fHeader.SetHeaderCRC(crc32((uint8*)&fHeader, sizeof(efi_table_header)));
}


bool
Header::_ValidateHeaderCRC()
{
	uint32 originalCRC = fHeader.HeaderCRC();
	fHeader.SetHeaderCRC(0);

	bool matches = originalCRC == crc32((const uint8*)&fHeader,
		sizeof(efi_table_header));
dprintf("GPT: MATCHES %d!\n", matches);

	fHeader.SetHeaderCRC(originalCRC);
	return matches;
}


bool
Header::_ValidateEntriesCRC() const
{
	return fHeader.EntriesCRC() == crc32(fEntries, _EntryArraySize());
}


#ifdef TRACE_EFI_GPT
const char *
Header::_PrintGUID(const guid_t &id)
{
	static char guid[48];
	snprintf(guid, sizeof(guid),
		"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		B_LENDIAN_TO_HOST_INT32(id.data1), B_LENDIAN_TO_HOST_INT16(id.data2),
		B_LENDIAN_TO_HOST_INT16(id.data3), id.data4[0], id.data4[1],
		id.data4[2], id.data4[3], id.data4[4], id.data4[5], id.data4[6],
		id.data4[7]);
	return guid;
}


void
Header::_Dump()
{
	dprintf("EFI header: %.8s\n", fHeader.header);
	dprintf("EFI revision: %ld\n", fHeader.Revision());
	dprintf("header size: %ld\n", fHeader.HeaderSize());
	dprintf("header CRC: %ld\n", fHeader.HeaderCRC());
	dprintf("absolute block: %Ld\n", fHeader.AbsoluteBlock());
	dprintf("alternate block: %Ld\n", fHeader.AlternateBlock());
	dprintf("first usable block: %Ld\n", fHeader.FirstUsableBlock());
	dprintf("last usable block: %Ld\n", fHeader.LastUsableBlock());
	dprintf("disk GUID: %s\n", _PrintGUID(fHeader.disk_guid));
	dprintf("entries block: %Ld\n", fHeader.EntriesBlock());
	dprintf("entry size:  %ld\n", fHeader.EntrySize());
	dprintf("entry count: %ld\n", fHeader.EntryCount());
	dprintf("entries CRC: %ld\n", fHeader.EntriesCRC());
}


void
Header::_DumpPartitions()
{
	for (uint32 i = 0; i < EntryCount(); i++) {
		const efi_partition_entry &entry = EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		dprintf("[%3ld] partition type: %s\n", i,
			_PrintGUID(entry.partition_type));
		dprintf("      unique id: %s\n", _PrintGUID(entry.unique_guid));
		dprintf("      start block: %Ld\n", entry.StartBlock());
		dprintf("      end block: %Ld\n", entry.EndBlock());
		dprintf("      size: %g MB\n", (entry.EndBlock() - entry.StartBlock())
			* 512 / 1024.0 / 1024.0);
		dprintf("      attributes: %Lx\n", entry.Attributes());

		char name[64];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		dprintf("      name: %s\n", name);
	}
}
#endif	// TRACE_EFI_GPT


}	// namespace EFI
