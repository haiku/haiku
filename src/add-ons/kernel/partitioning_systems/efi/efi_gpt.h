/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EFI_GPT_H
#define EFI_GPT_H


#include "guid.h"

#include <SupportDefs.h>
#include <ByteOrder.h>


struct efi_table_header {
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

	uint32 Revision() const
		{ return B_LENDIAN_TO_HOST_INT32(revision); }
	uint32 HeaderSize() const
		{ return B_LENDIAN_TO_HOST_INT32(header_size); }
	uint32 HeaderCRC() const
		{ return B_LENDIAN_TO_HOST_INT32(header_crc); }

	uint64 AbsoluteBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(absolute_block); }
	uint64 AlternateBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(alternate_block); }
	uint64 FirstUsableBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(first_usable_block); }
	uint64 LastUsableBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(last_usable_block); }

	uint64 EntriesBlock() const
		{ return B_LENDIAN_TO_HOST_INT64(entries_block); }
	uint32 EntryCount() const
		{ return B_LENDIAN_TO_HOST_INT32(entry_count); }
	uint32 EntrySize() const
		{ return B_LENDIAN_TO_HOST_INT32(entry_size); }
	uint32 EntriesCRC() const
		{ return B_LENDIAN_TO_HOST_INT32(entries_crc); }
};

#define EFI_PARTITION_HEADER		"EFI PART"
#define EFI_HEADER_LOCATION			1

#define EFI_PARTITION_NAME_LENGTH	36

struct efi_partition_entry {
	guid_t	partition_type;
	guid_t	unique_guid;
	uint64	start_lba;
	uint64	end_lba;
	uint64	attributes;
	uint16	name[EFI_PARTITION_NAME_LENGTH];
};

#endif	/* EFI_GPT_H */

