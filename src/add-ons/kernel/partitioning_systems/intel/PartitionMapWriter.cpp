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
/*!
	\brief Creates the writer.

	\param deviceFD File descriptor.
	\param sessionOffset Disk offset of the partition with partitioning system.
	\param sessionSize Size of the partition with partitioning system.
*/
PartitionMapWriter::PartitionMapWriter(int deviceFD, off_t sessionOffset,
		off_t sessionSize)
	: fDeviceFD(deviceFD),
	  fSessionOffset(sessionOffset),
	  fSessionSize(sessionSize),
	  fPTS(NULL),
	  fMap(NULL)
{
}

// destructor
PartitionMapWriter::~PartitionMapWriter()
{
}

// WriteMBR
/*!
	\brief Writes Master Boot Record to the first sector of the disk.

	If a \a block is not specified, the sector is firstly read from the disk
	and after changing relevant items it is written back to the disk.
	This allows to keep code area in MBR intact.
	\param pts Pointer to \c partition_table_sector.
	\param map Pointer to the PartitionMap structure describing disk partitions.
*/
status_t
PartitionMapWriter::WriteMBR(const PartitionMap *map, bool clearSectors)
{
	if (!map)
		return B_BAD_VALUE;

	fMap = map;

	uint8 sector[SECTOR_SIZE];	
	partition_table_sector* pts = (partition_table_sector*)sector;

	// If we shall not clear the first two sectors, we need to read the first
	// sector in, first.
	status_t error = B_OK;
	if (clearSectors)
		memset(sector, 0, SECTOR_SIZE);
	else
		error = _ReadPTS(0, pts);

	if (error == B_OK) {
		error = _WritePrimary(pts);
		if (error == B_OK)
			error = _WriteSector(0, sector);
	}

	// Clear the second sector, if desired. We do that to make the partition
	// unrecognizable by BFS.
	if (error == B_OK && clearSectors) {
		memset(sector, 0, SECTOR_SIZE);
		error = _WriteSector(SECTOR_SIZE, sector);
	}

	fMap = NULL;

	return error;
}

// WriteLogical
/*!
	\brief Writes Partition Table Sector of the logical \a partition to the
		disk.

	This function ensures that the connection of the following linked list
	of logical partitions will be correct. It do nothing with the connection of
	previous logical partitions (call this function on previous logical
	partition to ensure it).

	\param pts Pointer to \c partition_table_sector.
	\param partition Pointer to the logical partition.
*/
status_t
PartitionMapWriter::WriteLogical(partition_table_sector *pts,
	const LogicalPartition *partition)
{
	status_t error = (partition ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (pts) {
			error = _WriteExtended(pts, partition, partition->Next());
			if (error == B_OK)
				error = _WriteSector(partition->PTSOffset(), pts);
		} else {
			partition_table_sector _pts;
			pts = &_pts;
			error = _ReadPTS(partition->PTSOffset(), pts);
			if (error == B_OK) {
				error = _WriteExtended(pts, partition, partition->Next());
				if (error == B_OK)
					error = _WriteSector(partition->PTSOffset(), pts);
			}
		}
	}
	return error;
}

// WriteExtendedHead
/*!
	\brief Writes Extended Boot Record to the first sector of Extended
		Partition.

	Writes the head of linked list describing logical partitions.

	If the \a first_partition is not specified, it only initializes EBR and the
	linked list contains no logical partitions.

	\param pts Pointer to \c partition_table_sector.
	\param first_partition Pointer to the first logical partition.
*/
status_t
PartitionMapWriter::WriteExtendedHead(partition_table_sector *pts,
	const LogicalPartition *first_partition)
{
	LogicalPartition partition;
	if (first_partition)
		partition.SetPrimaryPartition(first_partition->GetPrimaryPartition());
	status_t error = B_OK;
	if (pts) {
		error = _WriteExtended(pts, &partition, first_partition);
		if (error == B_OK)
			error = _WriteSector(0, pts);
	} else {
		partition_table_sector _pts;
		pts = &_pts;
		error = _ReadPTS(0, pts);
		if (error == B_OK) {
			error = _WriteExtended(pts, &partition, first_partition);
			if (error == B_OK)
				error = _WriteSector(0, pts);
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
		if (!partition->CheckLocation(fSessionSize)) {
			TRACE(("intel: _WritePrimary(): partition %ld: bad location, "
				"ignoring\n", i));
			return B_BAD_DATA;
		}

		partition->GetPartitionDescriptor(descriptor, 0);
	}

	return B_OK;
}

// _WriteExtended
status_t
PartitionMapWriter::_WriteExtended(partition_table_sector *pts,
	const LogicalPartition *partition, const LogicalPartition *next)
{
	if (!pts || !partition)
		return B_BAD_VALUE;

	// write the signature
	pts->signature = kPartitionTableSectorSignature;

	// check the partition's location
	if (!partition->CheckLocation(fSessionSize)) {
		TRACE(("intel: _WriteExtended(): Invalid partition "
			"location: pts: %lld, offset: %lld, size: %lld, "
			"fSessionSize: %lld\n",
			partition->PTSOffset(), partition->Offset(),
			partition->Size(), fSessionSize));
		return B_BAD_DATA;
	}

	// write the table
	partition_descriptor *descriptor = &(pts->table[0]);
	partition->GetPartitionDescriptor(descriptor, partition->PTSOffset());

	// setting offset and size of the next partition in the linked list
	descriptor = &(pts->table[1]);
	LogicalPartition extended;
	if (next) {
		extended.SetPTSOffset(partition->PTSOffset());
		extended.SetOffset(next->PTSOffset());
		extended.SetSize(next->Size() + next->Offset() - next->PTSOffset());
		extended.SetType(partition->GetPrimaryPartition()->Type());
		extended.GetPartitionDescriptor(descriptor, 0);
		extended.Unset();
	} else
		extended.GetPartitionDescriptor(descriptor, 0);

	// last two descriptors are empty
	for (int32 i = 2; i < 4; i++) {
		descriptor = &(pts->table[i]);
		extended.GetPartitionDescriptor(descriptor, 0);
	}

	return B_OK;
}

// _ReadPTS
/*! \brief Reads the sector from the disk.
*/
status_t
PartitionMapWriter::_ReadPTS(off_t offset, partition_table_sector *pts)
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

// _WriteSector
/*! \brief Writes the sector to the disk.
*/
status_t
PartitionMapWriter::_WriteSector(off_t offset, const void* pts)
{
	status_t error = B_OK;

	int32 toWrite = SECTOR_SIZE;

	// check the offset
	if (offset < 0 || offset + toWrite > fSessionSize) {
		error = B_BAD_VALUE;
		TRACE(("intel: _WriteSector(): bad offset: %Ld\n", offset));
	// write
	} else if (write_pos(fDeviceFD, fSessionOffset + offset, pts, toWrite)
						!= toWrite) {
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;

		TRACE(("intel: _WriteSector(): writing the PTS failed: %lx\n", error));
	}
	return error;
}

