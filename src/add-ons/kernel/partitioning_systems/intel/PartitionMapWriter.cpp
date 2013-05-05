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


// compiled mbr boot loader code
static const uint8 kBootCode[] = {
	// compiled form of //haiku/trunk/src/bin/writembr/mbr.S
	// yasm -f bin -O5 -o mbrcode.bin mbr.S -dMBR_CODE_ONLY=1
	// bin2h <mbrcode.bin 12
	0xfa, 0xfc, 0x31, 0xc0, 0x8e, 0xc0, 0x8e, 0xd8, 0x8e, 0xd0, 0xbc, 0x00,
	0x7c, 0xbf, 0x00, 0x08, 0xb9, 0x18, 0x00, 0xf3, 0xaa, 0xbe, 0x00, 0x7c,
	0x89, 0x36, 0x0c, 0x08, 0xc6, 0x06, 0x08, 0x08, 0x10, 0xc6, 0x06, 0x0a,
	0x08, 0x01, 0xbf, 0x00, 0x06, 0xb5, 0x01, 0xf3, 0xa5, 0xea, 0x32, 0x06,
	0x00, 0x00, 0xfb, 0xbe, 0xbe, 0x07, 0xb0, 0x04, 0x80, 0x3c, 0x80, 0x74,
	0x09, 0x83, 0xc6, 0x10, 0xfe, 0xc8, 0x75, 0xf4, 0xeb, 0x6d, 0x89, 0x36,
	0x04, 0x08, 0x66, 0x8b, 0x44, 0x08, 0x66, 0xa3, 0x10, 0x08, 0x66, 0x85,
	0xc0, 0x74, 0x27, 0xb4, 0x41, 0xbb, 0xaa, 0x55, 0xcd, 0x13, 0x72, 0x1e,
	0xbe, 0x08, 0x08, 0xb4, 0x42, 0xcd, 0x13, 0x72, 0x15, 0x81, 0x3e, 0xfe,
	0x7d, 0x55, 0xaa, 0x75, 0x42, 0xbe, 0xfb, 0x06, 0xe8, 0x52, 0x00, 0x8b,
	0x36, 0x04, 0x08, 0xe9, 0x82, 0x75, 0x8b, 0x36, 0x04, 0x08, 0x8a, 0x74,
	0x01, 0x8b, 0x4c, 0x02, 0x88, 0xc8, 0x24, 0x3f, 0x84, 0xc0, 0x74, 0x23,
	0x3c, 0x3f, 0x75, 0x14, 0x89, 0xc8, 0xc0, 0xec, 0x06, 0x3d, 0xff, 0x03,
	0x75, 0x0a, 0x80, 0xfe, 0xff, 0x75, 0x05, 0x80, 0xfe, 0xfe, 0x74, 0x0b,
	0xbb, 0x00, 0x7c, 0xb0, 0x01, 0xb4, 0x02, 0xcd, 0x13, 0x73, 0xb6, 0xbe,
	0xdf, 0x06, 0xe8, 0x10, 0x00, 0xe8, 0x08, 0x00, 0xbe, 0xd5, 0x06, 0xe8,
	0x07, 0x00, 0xcd, 0x18, 0xb4, 0x00, 0xcd, 0x16, 0xc3, 0x31, 0xdb, 0xac,
	0xb4, 0x0e, 0xcd, 0x10, 0x08, 0xc0, 0x75, 0xf7, 0xc3, 0x52, 0x4f, 0x4d,
	0x20, 0x42, 0x41, 0x53, 0x49, 0x43, 0x00, 0x4e, 0x6f, 0x20, 0x62, 0x6f,
	0x6f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x61, 0x63, 0x74, 0x69, 0x76,
	0x65, 0x20, 0x76, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x0d, 0x0a, 0x00, 0x4c,
	0x6f, 0x61, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65,
	0x6d, 0x0a, 0x0d, 0x00
};


bool
check_logical_location(const LogicalPartition* child,
	const PrimaryPartition* parent)
{
	if (child->PartitionTableOffset() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - PartitionTableOffset: %" B_PRId64 " "
			"not a multiple of media's block size: %" B_PRId32 "\n",
			child->PartitionTableOffset(), child->BlockSize()));
		return false;
	}
	if (child->Offset() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - Parition offset: %" B_PRId64 " "
			"is not a multiple of block size: %" B_PRId32 "\n", child->Offset(),
			child->BlockSize()));
		return false;
	}
	if (child->Size() % child->BlockSize() != 0) {
		TRACE(("check_logical_location() - Size: (%" B_PRId64 ") is not a "
			"multiple of block size: (%" B_PRId32 ")\n", child->Size(),
			child->BlockSize()));
		return false;
	}
	if (child->PartitionTableOffset() < parent->Offset()
		|| child->PartitionTableOffset() >= parent->Offset()
		+ parent->Size()) {
		TRACE(("check_logical_location() - Partition table: (%" B_PRId64 ") not"
			" within extended partition (start: %" B_PRId64 "), (end: "
			"%" B_PRId64 ")\n", child->PartitionTableOffset(), parent->Offset(),
			parent->Offset() + parent->Size()));
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

