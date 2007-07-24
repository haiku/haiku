/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */

#ifndef _USER_MODE
#	include <KernelExport.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <new>

#include "PartitionMap.h"
#include "PartitionMapWriter.h"

#define TRACE_ENABLED

#ifdef TRACE_ENABLED
#	ifdef _USER_MODE
#		define TRACE(x) printf x
#	else
#		define TRACE(x) dprintf x
#	endif
#endif

using std::nothrow;

// constructor
PartitionMapWriter::PartitionMapWriter(int deviceFD, off_t sessionOffset,
									   off_t sessionSize, int32 blockSize)
	: fDeviceFD(deviceFD),
	  fSessionOffset(sessionOffset),
	  fSessionSize(sessionSize),
	  fBlockSize(blockSize),
	  fPTS(NULL),
	  fMap(NULL)
{
}

// destructor
PartitionMapWriter::~PartitionMapWriter()
{
}

// WriteMBR
status_t
PartitionMapWriter::WriteMBR(uint8 *block, const PartitionMap *map)
{
	status_t error = (map ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fMap = map;
		if (block) {
			partition_table_sector *pts
				= (partition_table_sector*)block;
			error = _WritePrimary(pts);
			if (error == B_OK)
				error = _WritePTS(0, pts);
		} else {
			partition_table_sector pts;
			error = _ReadPTS(0, &pts);
			if (error == B_OK) {
				error = _WritePrimary(&pts);
				if (error == B_OK)
					error = _WritePTS(0, &pts);
			}
		}

		fMap = NULL;
	}
	return error;
}

// WriteLogical
status_t
PartitionMapWriter::WriteLogical(uint8 *block, const LogicalPartition *partition)
{
	status_t error = (partition ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (block) {
			partition_table_sector *pts
					= (partition_table_sector*)block;
			error = _WriteExtended(pts, partition, partition->Next());
			if (error == B_OK)
				error = _WritePTS(partition->PTSOffset(), pts);
		} else {
			partition_table_sector pts;
			error = _ReadPTS(partition->PTSOffset(), &pts);
			if (error == B_OK) {
				error = _WriteExtended(&pts, partition, partition->Next());
				if (error == B_OK)
					error = _WritePTS(partition->PTSOffset(), &pts);
			}
		}
	}
	return error;
}

// WriteExtendedHead
status_t
PartitionMapWriter::WriteExtendedHead(uint8 *block,
									  const LogicalPartition *first_partition)
{
	LogicalPartition partition;
	if (first_partition)
		partition.SetPrimaryPartition(first_partition->GetPrimaryPartition());
	status_t error = B_OK;
	if (block) {
		partition_table_sector *pts
				= (partition_table_sector*)block;
		error = _WriteExtended(pts, &partition, first_partition);
		if (error == B_OK)
			error = _WritePTS(0, pts);
	} else {
		partition_table_sector pts;
		error = _ReadPTS(0, &pts);
		if (error == B_OK) {
			error = _WriteExtended(&pts, &partition, first_partition);
			if (error == B_OK)
				error = _WritePTS(0, &pts);
		}
	}
	return error;
}

// _WritePrimary
status_t
PartitionMapWriter::_WritePrimary(partition_table_sector *pts)
{
	if (pts == NULL)
		return B_BAD_VALUE;

	// write the signature
	pts->signature = kPartitionTableSectorSignature;

	// write the table
	for (int32 i = 0; i < 4; i++) {
		partition_descriptor *descriptor = &pts->table[i];
		const PrimaryPartition *partition = fMap->PrimaryPartitionAt(i);

		// ignore, if location is bad
		if (!partition->CheckLocation(fSessionSize, fBlockSize)) {
			TRACE(("intel: _WritePrimary(): partition %ld: bad location, "
				"ignoring\n", i));
			return B_BAD_DATA;
		}

		partition->GetPartitionDescriptor(descriptor, 0, fBlockSize);
	}

	return B_OK;
}

// _WriteExtended
status_t
PartitionMapWriter::_WriteExtended(partition_table_sector *pts,
								   const LogicalPartition *partition,
								   const LogicalPartition *next)
{
	if (!pts || !partition)
		return B_BAD_VALUE;

	// write the signature
	pts->signature = kPartitionTableSectorSignature;

	// check the partition's location
	if (!partition->CheckLocation(fSessionSize, fBlockSize)) {
		TRACE(("intel: _WriteExtended(): Invalid partition "
			"location: pts: %lld, offset: %lld, size: %lld, fSessionSize: %lld\n",
			partition->PTSOffset(), partition->Offset(),
			partition->Size(), fSessionSize));
		return B_BAD_DATA;
	}

	// write the table
	partition_descriptor *descriptor = &(pts->table[0]);
	partition->GetPartitionDescriptor(descriptor, partition->PTSOffset(), fBlockSize);

	// setting offset and size of the next partition in the linked list
	descriptor = &(pts->table[1]);
	LogicalPartition extended;
	if (next) {
		extended.SetPTSOffset(partition->PTSOffset());
		extended.SetOffset(next->PTSOffset());
		extended.SetSize(next->Size() + next->Offset() - next->PTSOffset());
		extended.SetType(partition->GetPrimaryPartition()->Type());
		extended.GetPartitionDescriptor(descriptor, 0, fBlockSize);
		extended.Unset();
	} else
		extended.GetPartitionDescriptor(descriptor, 0, fBlockSize);

	// last two descriptors are empty
	for (int32 i = 2; i < 4; i++) {
		descriptor = &(pts->table[i]);
		extended.GetPartitionDescriptor(descriptor, 0, fBlockSize);
	}

	return B_OK;
}

// _ReadPTS
status_t
PartitionMapWriter::_ReadPTS(off_t offset, partition_table_sector *pts)
// reads the sector from the disk
{
	status_t error = B_OK;
	if (!pts)
		pts = fPTS;
	int32 toRead = sizeof(partition_table_sector);
	// check the offset
	if (offset < 0 || offset + toRead > fSessionSize) {
		error = B_BAD_VALUE;
		TRACE(("intel: _ReadPTS(): bad offset: %Ld\n", offset));
	// read
	} else if (read_pos(fDeviceFD, fSessionOffset + offset, pts, toRead)
						!= toRead) {
#ifndef _BOOT_MODE
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
#else
		error = B_IO_ERROR;
#endif
		TRACE(("intel: _ReadPTS(): reading the PTS failed: %lx\n", error));
	}
	return error;
}

// _WritePTS
status_t
PartitionMapWriter::_WritePTS(off_t offset, const partition_table_sector *pts)
// writes the sector to the disk
{
	status_t error = B_OK;
	if (!pts)
		pts = fPTS;
	int32 toWrite = sizeof(partition_table_sector);
	// check the offset
	if (offset < 0 || offset + toWrite > fSessionSize) {
		error = B_BAD_VALUE;
		TRACE(("intel: _WritePTS(): bad offset: %Ld\n", offset));
	// write
	} else if (write_pos(fDeviceFD, fSessionOffset + offset, pts, toWrite)
						!= toWrite) {
#ifndef _BOOT_MODE
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
#else
		error = B_IO_ERROR;
#endif
		TRACE(("intel: _WritePTS(): writing the PTS failed: %lx\n", error));
	}
	return error;
}

