/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bryce Groff, brycegroff@gmail.com
 */

#include "PartitionMapWriter.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <debug.h>


#ifndef _USER_MODE
#	include <KernelExport.h>
#endif

#include "PartitionMap.h"

using std::nothrow;


#define TRACE_ENABLED
#ifdef TRACE_ENABLED
#	ifdef _USER_MODE
#		define TRACE(x) printf x
#	else
#		define TRACE(x) dprintf x
#	endif
#endif


// compiled form of hybrid FreeBSD pmbr and mbr loader done by André Braga
static const uint8 kBootCode[] = {
	0xfc, 0xe8, 0x05, 0x01, 0xbc, 0x00, 0x0e, 0xbe, 0x15, 0x7c, 0xbf, 0x15,
	0x06, 0xb9, 0xeb, 0x01, 0xf3, 0xa4, 0xe9, 0x00, 0x8a, 0xe8, 0xfc, 0x00,
	0xbb, 0x00, 0x0e, 0xbe, 0xb0, 0x07, 0xb9, 0x01, 0x00, 0xe8, 0x03, 0x01,
	0x66, 0x81, 0x3e, 0x00, 0x0e, 0x45, 0x46, 0x49, 0x20, 0x75, 0x5c, 0x66,
	0x81, 0x3e, 0x04, 0x0e, 0x50, 0x41, 0x52, 0x54, 0x75, 0x51, 0xbe, 0x48,
	0x0e, 0xbb, 0x00, 0x10, 0xb9, 0x01, 0x00, 0xe8, 0xe1, 0x00, 0x89, 0xde,
	0xbf, 0xa0, 0x07, 0xb1, 0x10, 0xf3, 0xa6, 0x75, 0x1b, 0x89, 0xdf, 0x8d,
	0x75, 0x20, 0xbb, 0xc0, 0x07, 0x8e, 0xc3, 0x31, 0xdb, 0x56, 0x8a, 0x0e,
	0x9f, 0x07, 0xe8, 0xc2, 0x00, 0x31, 0xc0, 0x8e, 0xc0, 0xe9, 0x94, 0x75,
	0x66, 0xff, 0x0e, 0x50, 0x0e, 0x74, 0x18, 0xa1, 0x54, 0x0e, 0x01, 0xc3,
	0x81, 0xfb, 0x00, 0x12, 0x72, 0xc8, 0x66, 0xff, 0x06, 0x48, 0x0e, 0x66,
	0x83, 0x16, 0x4c, 0x0e, 0x00, 0xeb, 0xaf, 0xe8, 0x7b, 0x00, 0xbc, 0x00,
	0x7c, 0x31, 0xf6, 0xbb, 0xbe, 0x07, 0xb1, 0x04, 0x38, 0x2f, 0x74, 0x0c,
	0x0f, 0x8f, 0xa2, 0x00, 0x85, 0xf6, 0x0f, 0x85, 0x9c, 0x00, 0x89, 0xde,
	0x80, 0xc3, 0x10, 0xe2, 0xeb, 0x85, 0xf6, 0x75, 0x02, 0xcd, 0x18, 0xe8,
	0x5e, 0x00, 0x89, 0xe7, 0x8a, 0x74, 0x01, 0x8b, 0x4c, 0x02, 0xbb, 0x00,
	0x7c, 0xf6, 0x06, 0x9e, 0x07, 0x80, 0x74, 0x2d, 0x51, 0x53, 0xbb, 0xaa,
	0x55, 0xb4, 0x41, 0xcd, 0x13, 0x72, 0x20, 0x81, 0xfb, 0x55, 0xaa, 0x75,
	0x1a, 0xf6, 0xc1, 0x01, 0x74, 0x15, 0x5b, 0x66, 0x6a, 0x00, 0x66, 0xff,
	0x74, 0x08, 0x06, 0x53, 0x6a, 0x01, 0x6a, 0x10, 0x89, 0xe6, 0xb8, 0x00,
	0x42, 0xeb, 0x05, 0x5b, 0x59, 0xb8, 0x01, 0x02, 0xcd, 0x13, 0x89, 0xfc,
	0x72, 0x49, 0x81, 0xbf, 0xfe, 0x01, 0x55, 0xaa, 0x75, 0x46, 0xe9, 0x5c,
	0xff, 0x31, 0xc9, 0x31, 0xc0, 0x8e, 0xc0, 0x8e, 0xd8, 0x8e, 0xd0, 0xc3,
	0x80, 0xfa, 0x80, 0x72, 0x0b, 0x8a, 0x36, 0x75, 0x04, 0x80, 0xc6, 0x80,
	0x38, 0xf2, 0x72, 0x02, 0xb2, 0x80, 0xc3, 0x66, 0xff, 0x74, 0x04, 0x66,
	0xff, 0x34, 0x06, 0x53, 0x51, 0x6a, 0x10, 0x89, 0xe6, 0xb8, 0x00, 0x42,
	0xcd, 0x13, 0x83, 0xc4, 0x10, 0x0f, 0x82, 0x4a, 0xff, 0xc3, 0xbe, 0x60,
	0x07, 0xeb, 0x11, 0xbe, 0x74, 0x07, 0xeb, 0x0c, 0xbe, 0x7d, 0x07, 0xeb,
	0x07, 0xbb, 0x07, 0x00, 0xb4, 0x0e, 0xcd, 0x10, 0xac, 0x84, 0xc0, 0x75,
	0xf4, 0xf4, 0xeb, 0xfd, 0x42, 0x61, 0x64, 0x20, 0x50, 0x61, 0x72, 0x74,
	0x69, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x61, 0x62, 0x6c, 0x65, 0x00,
	0x49, 0x4f, 0x20, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x00, 0x4d, 0x69, 0x73,
	0x73, 0x69, 0x6e, 0x67, 0x20, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20,
	0x42, 0x6f, 0x6f, 0x74, 0x6c, 0x6f, 0x61, 0x64, 0x65, 0x72, 0x00, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x02, 0x31, 0x53, 0x46, 0x42,
	0xa3, 0x3b, 0xf1, 0x10, 0x80, 0x2a, 0x48, 0x61, 0x69, 0x6b, 0x75, 0x21
};


bool
check_logical_location(const LogicalPartition* child,
	const PrimaryPartition* parent)
{
	if (child->PartitionTableOffset() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - PartitionTableOffset: %lld not a "
			"multiple of media's block size: %ld\n",
			child->PartitionTableOffset(), child->BlockSize()));
		return false;
	}
	if (child->Offset() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - Parition offset: %lld "
			"is not a multiple of block size: %ld\n", child->Offset(),
			child->BlockSize()));
		return false;
	}
	if (child->Size() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - Size: (%lld) is not a multiple of"
			" block size: (%ld)\n", child->Size(), child->BlockSize()));
		return false;
	}
	if (child->PartitionTableOffset() < parent->Offset()
		|| child->PartitionTableOffset() >= parent->Offset()
		+ parent->Size()) {
		TRACE(("check_logical_location() - Partition table: (%lld) not within "
			"extended partition (start: %lld), (end: %lld)\n",
			child->PartitionTableOffset(), parent->Offset(), parent->Offset()
			+ parent->Size()));
		return false;
	}
	if (child->Offset() + child->Size() > parent->Offset() + parent->Size()) {
		TRACE(("check_logical_location() - logical paritition does not lie "
			"within extended partition\n"));
		return false;
	}
	return true;
}


PartitionMapWriter::PartitionMapWriter(int deviceFD, uint32 blockSize)
	:
	fDeviceFD(deviceFD),
	fBlockSize(blockSize)
{
}


PartitionMapWriter::~PartitionMapWriter()
{
}


status_t
PartitionMapWriter::WriteMBR(const PartitionMap* map, bool writeBootCode)
{
	if (map == NULL)
		return B_BAD_VALUE;

	partition_table partitionTable;
	status_t error = _ReadBlock(0, partitionTable);
	if (error != B_OK)
		return error;
	if (writeBootCode) {
		// the boot code must be small enough to fit in the code area
		STATIC_ASSERT(sizeof(kBootCode) <= sizeof(partitionTable.code_area));
		partitionTable.clear_code_area();
		partitionTable.fill_code_area(kBootCode, sizeof(kBootCode));
	}

	partitionTable.signature = kPartitionTableSectorSignature;

	for (int i = 0; i < 4; i++) {
		partition_descriptor* descriptor = &partitionTable.table[i];
		const PrimaryPartition* partition = map->PrimaryPartitionAt(i);

		partition->GetPartitionDescriptor(descriptor);
	}

	error = _WriteBlock(0, partitionTable);
	return error;
}


status_t
PartitionMapWriter::WriteLogical(const LogicalPartition* logical,
	const PrimaryPartition* primary, bool clearCode)
{
	if (logical == NULL || primary == NULL)
		return B_BAD_VALUE;

	if (!check_logical_location(logical, primary))
		return B_BAD_DATA;

	partition_table partitionTable;
	if (clearCode) {
		partitionTable.clear_code_area();
	} else {
		status_t error = _ReadBlock(logical->PartitionTableOffset(),
			partitionTable);
		if (error != B_OK)
			return error;
	}

	partitionTable.signature = kPartitionTableSectorSignature;

	partition_descriptor* descriptor = &partitionTable.table[0];
	logical->GetPartitionDescriptor(descriptor);

	descriptor = &partitionTable.table[1];
	if (logical->Next() != NULL)
		logical->Next()->GetPartitionDescriptor(descriptor, true);
	else
		memset(descriptor, 0, sizeof(partition_descriptor));

	// last two descriptors are empty
	for (int32 i = 2; i < 4; i++) {
		descriptor = &partitionTable.table[i];
		memset(descriptor, 0, sizeof(partition_descriptor));
	}

	status_t error = _WriteBlock(logical->PartitionTableOffset(),
		partitionTable);
	return error;
}


status_t
PartitionMapWriter::WriteExtendedHead(const LogicalPartition* logical,
	const PrimaryPartition* primary, bool clearCode)
{
	if (primary == NULL)
		return B_BAD_VALUE;

	partition_table partitionTable;
	if (clearCode) {
		partitionTable.clear_code_area();
	} else {
		status_t error = _ReadBlock(primary->Offset(), partitionTable);
		if (error != B_OK)
			return error;
	}

	partitionTable.signature = kPartitionTableSectorSignature;
	partition_descriptor* descriptor;
	if (logical == NULL) {
		for (int32 i = 0; i < 4; i++) {
			descriptor = &partitionTable.table[i];
			memset(descriptor, 0, sizeof(partition_descriptor));
		}
	} else {
		LogicalPartition partition;
		partition.SetPartitionTableOffset(primary->Offset());
		partition.SetBlockSize(logical->BlockSize());
		partition.SetOffset(logical->Offset());
		partition.SetSize(logical->Size());
		partition.SetType(logical->Type());

		// set the logicals partition table to the correct location
		descriptor = &partitionTable.table[0];
		partition.GetPartitionDescriptor(descriptor);

		descriptor = &partitionTable.table[1];
		LogicalPartition* next = logical->Next();
		if (next != NULL)
			next->GetPartitionDescriptor(descriptor, true);
		else
			memset(descriptor, 0, sizeof(partition_descriptor));

		// last two descriptors are empty
		for (int32 i = 2; i < 4; i++) {
			descriptor = &partitionTable.table[i];
			memset(descriptor, 0, sizeof(partition_descriptor));
		}
	}

	status_t error = _WriteBlock(primary->Offset(), partitionTable);
	if (error != B_OK)
		return error;

	return B_OK;
}



status_t
PartitionMapWriter::ClearExtendedHead(const PrimaryPartition* primary)
{
	if (primary == NULL)
		return B_BAD_VALUE;

	partition_table partitionTable;
	partitionTable.clear_code_area();
	partitionTable.signature = kPartitionTableSectorSignature;

	partition_descriptor* descriptor;
	for (int32 i = 0; i < 4; i++) {
		descriptor = &partitionTable.table[i];
		memset(descriptor, 0, sizeof(partition_descriptor));
	}

	status_t error = _WriteBlock(primary->Offset(), partitionTable);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
PartitionMapWriter::_ReadBlock(off_t partitionOffset,
	partition_table& partitionTable)
{
	if (partitionOffset < 0)
		return B_BAD_VALUE;
	// TODO: If fBlockSize > sizeof(partition_table) then stop/read NULL after
	if (read_pos(fDeviceFD, partitionOffset, &partitionTable,
		sizeof(partitionTable)) != sizeof(partitionTable)) {
		status_t error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;

		return error;
	}

	return B_OK;
}


status_t
PartitionMapWriter::_WriteBlock(off_t partitionOffset,
	const partition_table& partitionTable)
{
	if (partitionOffset < 0)
		return B_BAD_VALUE;
	// TODO: maybe clear the rest of the block if
	// fBlockSize > sizeof(partition_table)?
	if (write_pos(fDeviceFD, partitionOffset, &partitionTable,
		sizeof(partitionTable)) != sizeof(partitionTable)) {
		status_t error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;

		return error;
	}

	return B_OK;
}

