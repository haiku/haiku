/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EFI_GPT_H
#define EFI_GPT_H


#include "guid.h"

#include <SupportDefs.h>
#include <ByteOrder.h>


struct gpt_table_header {
	char	header[8];
	uint32	revision;
	uint32	header_size;
	uint32	header_crc;
	uint32	_reserved1;

	uint64	absolute_block;
	uint64	alternate_block;
	uint64	first_usable_block;
	uint64	last_usable_block;
	guid_t	disk_guid;
	uint64	entries_block;
	uint32	entry_count;
	uint32	entry_size;
	uint32	entries_crc;

	// the rest of the block is reserved

	void SetRevision(uint32 newRevision)
		{ revision = B_HOST_TO_LENDIAN_INT32(newRevision); }
	uint32 Revision() const
		{ return B_LENDIAN_TO_HOST_INT32(revision); }
	void SetHeaderSize(uint32 size)
		{ header_size = B_HOST_TO_LENDIAN_INT32(size); }
	uint32 HeaderSize() const
		{ return B_LENDIAN_TO_HOST_INT32(header_size); }
	void SetHeaderCRC(uint32 crc)
		{ header_crc = B_HOST_TO_LENDIAN_INT32(crc); }
	uint32 HeaderCRC() const
		{ return B_LENDIAN_TO_HOST_INT32(header_crc); }

	void SetAbsoluteBlock(uint64 block)
		{ absolute_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 AbsoluteBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(absolute_block); }
	void SetAlternateBlock(uint64 block)
		{ alternate_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 AlternateBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(alternate_block); }
	void SetFirstUsableBlock(uint64 block)
		{ first_usable_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 FirstUsableBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(first_usable_block); }
	void SetLastUsableBlock(uint64 block)
		{ last_usable_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 LastUsableBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(last_usable_block); }

	void SetEntriesBlock(uint64 block)
		{ entries_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 EntriesBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(entries_block); }
	void SetEntryCount(uint32 count)
		{ entry_count = B_HOST_TO_LENDIAN_INT32(count); }
	uint32 EntryCount() const
		{ return B_LENDIAN_TO_HOST_INT32(entry_count); }
	void SetEntrySize(uint32 size)
		{ entry_size = B_HOST_TO_LENDIAN_INT32(size); }
	uint32 EntrySize() const
		{ return B_LENDIAN_TO_HOST_INT32(entry_size); }
	void SetEntriesCRC(uint32 crc)
		{ entries_crc = B_HOST_TO_LENDIAN_INT32(crc); }
	uint32 EntriesCRC() const
		{ return B_LENDIAN_TO_HOST_INT32(entries_crc); }
} _PACKED;

#define EFI_PARTITION_HEADER			"EFI PART"
#define EFI_HEADER_LOCATION				1
#define EFI_TABLE_REVISION				0x00010000

#define EFI_PARTITION_NAME_LENGTH		36
#define EFI_PARTITION_ENTRIES_BLOCK		2
#define EFI_PARTITION_ENTRY_COUNT		128
#define EFI_PARTITION_ENTRY_SIZE		128

struct gpt_partition_entry {
	guid_t	partition_type;
	guid_t	unique_guid;
	uint64	start_block;
	uint64	end_block;
	uint64	attributes;
	uint16	name[EFI_PARTITION_NAME_LENGTH];

	void SetStartBlock(uint64 block)
		{ start_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 StartBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(start_block); }
	void SetEndBlock(uint64 block)
		{ end_block = B_HOST_TO_LENDIAN_INT64(block); }
	uint64 EndBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(end_block); }
	void SetAttributes(uint64 _attributes)
		{ attributes = B_HOST_TO_LENDIAN_INT64(_attributes); }
	uint64 Attributes() const
		{ return B_LENDIAN_TO_HOST_INT64(attributes); }

	// convenience functions
	void SetBlockCount(uint64 blockCount)
		{ SetEndBlock(StartBlock() + blockCount - 1); }
	uint64 BlockCount() const
		{ return EndBlock() - StartBlock() + 1; }
} _PACKED;

#endif	/* EFI_GPT_H */

