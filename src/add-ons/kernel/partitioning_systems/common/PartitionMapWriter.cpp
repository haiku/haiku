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

#ifndef _USER_MODE
#include <debug.h>
#endif

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


#if defined(__i386__) || defined(__x86_64__)
#	ifndef _USER_MODE
#		define MBR_HEADER "MBR.h"
#		include MBR_HEADER
#	endif
#endif


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
#ifdef MBR_HEADER
	if (writeBootCode) {
		// the boot code must be small enough to fit in the code area
		STATIC_ASSERT(kMBRSize <= sizeof(partitionTable.code_area));
		partitionTable.clear_code_area();
		partitionTable.fill_code_area(kMBR, kMBRSize);
	}
#endif

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

