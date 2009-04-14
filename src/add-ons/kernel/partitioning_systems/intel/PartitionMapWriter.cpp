/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */

#include "PartitionMapWriter.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <new>

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


// constructor
/*!	\brief Creates the writer.

	\param deviceFD File descriptor.
	\param sessionOffset Disk offset of the partition with partitioning system.
	\param sessionSize Size of the partition with partitioning system.
*/
PartitionMapWriter::PartitionMapWriter(int deviceFD, off_t sessionOffset,
		off_t sessionSize)
	:
	fDeviceFD(deviceFD),
	fSessionOffset(sessionOffset),
	fSessionSize(sessionSize),
	fMap(NULL)
{
}


// destructor
PartitionMapWriter::~PartitionMapWriter()
{
}


// WriteMBR
/*!	\brief Writes Master Boot Record to the first sector of the disk.

	If a \a block is not specified, the sector is firstly read from the disk
	and after changing relevant items it is written back to the disk.
	This allows to keep code area in MBR intact.
	\param pts Pointer to \c partition_table.
	\param map Pointer to the PartitionMap structure describing disk partitions.
*/
status_t
PartitionMapWriter::WriteMBR(const PartitionMap *map, bool clearSectors)
{
	if (!map)
		return B_BAD_VALUE;

	fMap = map;

	uint8 sector[SECTOR_SIZE];
	partition_table* pts = (partition_table*)sector;

	// If we shall not clear the first two sectors, we need to read the first
	// sector in, first.
	status_t error = B_OK;
	if (clearSectors)
		memset(sector, 0, SECTOR_SIZE);
	else
		error = _ReadSector(0, pts);

	if (error == B_OK) {
		error = _WritePrimary(pts);
		if (error == B_OK)
			error = _WriteSector(0, pts);
	}

	// Clear the second sector, if desired. We do that to make the partition
	// unrecognizable by BFS.
	if (error == B_OK && clearSectors) {
		memset(sector, 0, SECTOR_SIZE);
		error = _WriteSector(SECTOR_SIZE, pts);
	}

	fMap = NULL;

	return error;
}


// WriteLogical
/*!	\brief Writes Partition Table Sector of the logical \a partition to the
		disk.

	This function ensures that the connection of the following linked list
	of logical partitions will be correct. It does nothing with the connection
	of previous logical partitions (call this function on previous logical
	partition to ensure it).

	\param pts Pointer to \c partition_table.
	\param partition Pointer to the logical partition.
*/
status_t
PartitionMapWriter::WriteLogical(partition_table* pts,
	const LogicalPartition* partition)
{
	if (partition == NULL)
		return B_BAD_VALUE;

	partition_table _pts;
	if (pts == NULL) {
		// no partition table given, use stack based partition table and read
		// from disk first
		pts = &_pts;
		status_t error = _ReadSector(partition->PartitionTableOffset(), pts);
		if (error != B_OK)
			return error;
	}

	status_t error = _WriteExtended(pts, partition, partition->Next());
	if (error != B_OK)
		return error;

	return _WriteSector(partition->PartitionTableOffset(), pts);
}


// WriteExtendedHead
/*!	\brief Writes Extended Boot Record to the first sector of Extended
		Partition.

	Writes the head of linked list describing logical partitions.

	If the \a firstPartition is not specified, it only initializes EBR and the
	linked list contains no logical partitions.

	\param pts Pointer to \c partition_table.
	\param firstPartition Pointer to the first logical partition.
*/
status_t
PartitionMapWriter::WriteExtendedHead(partition_table* pts,
	const LogicalPartition* firstPartition)
{
	LogicalPartition partition;
	if (firstPartition != NULL)
		partition.SetPrimaryPartition(firstPartition->GetPrimaryPartition());

	partition_table _pts;
	if (pts == NULL) {
		// no partition table given, use stack based partition table and read
		// from disk first
		pts = &_pts;
		status_t error = _ReadSector(0, pts);
		if (error != B_OK)
			return error;
	}

	status_t error = _WriteExtended(pts, &partition, firstPartition);
	if (error != B_OK)
		return error;

	return _WriteSector(0, pts);
}


// #pragma mark - fill a partition table in memory


// _WritePrimary
status_t
PartitionMapWriter::_WritePrimary(partition_table* pts)
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
			// TODO: Should this be fSessionOffset?!
	}

	return B_OK;
}


// _WriteExtended
status_t
PartitionMapWriter::_WriteExtended(partition_table *pts,
	const LogicalPartition *partition, const LogicalPartition *next)
{
	if (pts == NULL || partition == NULL)
		return B_BAD_VALUE;

	// write the signature
	pts->signature = kPartitionTableSectorSignature;

	// check the partition's location
	if (!partition->CheckLocation(fSessionSize)) {
		TRACE(("intel: _WriteExtended(): Invalid partition "
			"location: pts: %lld, offset: %lld, size: %lld, "
			"fSessionSize: %lld\n",
			partition->PartitionTableOffset(), partition->Offset(),
			partition->Size(), fSessionSize));
		return B_BAD_DATA;
	}

	// NOTE: The OS/2 boot manager needs the first entry to describe the
	// data partition, while the second entry should describe the "inner
	// extended" partition.

	// write the table
	partition_descriptor* descriptor = &(pts->table[0]);
	partition->GetPartitionDescriptor(descriptor,
		partition->PartitionTableOffset());
		// location is relative to this partition's table offset

	// Set offset and size of the next partition in the linked list.
	// This is done via a so called "inner extended" partition which is
	// only used to point to the next partition table location (start sector of
	// the inner extended partition).
	descriptor = &pts->table[1];
	LogicalPartition extended;
	if (next) {
		extended.SetPartitionTableOffset(partition->PartitionTableOffset());
		extended.SetOffset(next->PartitionTableOffset());

		// Strictly speaking, the size is not relevant and just needs to
		// be non-zero. But some operating systems check the size of
		// inner extended partitions and it needs to include the next data
		// partition. Therefor the size is the size of the next data partition
		// plus the offset between the next partition table and the data
		// partition start offset.
		// This assumes of course that the start offset is behind the partition
		// table offset, which is actually not dictated by a minimal
		// specification.
		extended.SetSize(next->Size()
			+ (next->Offset() - next->PartitionTableOffset()));

		// Use the same extended partition type as the primary extended
		// partition.
		extended.SetType(partition->GetPrimaryPartition()->Type());

		extended.GetPartitionDescriptor(descriptor, 0);

		// Unsetting to get an empty descriptor for the remaining slots.
		extended.Unset();
	} else
		extended.GetPartitionDescriptor(descriptor, 0);

	// last two descriptors are empty ("extended" is unset)
	for (int32 i = 2; i < 4; i++) {
		descriptor = &(pts->table[i]);
		extended.GetPartitionDescriptor(descriptor, 0);
	}

	return B_OK;
}


// #pragma mark - to/from disk


// _ReadSector
/*! \brief Reads the sector from the disk.
*/
status_t
PartitionMapWriter::_ReadSector(off_t offset, partition_table* pts)
{
	int32 toRead = sizeof(partition_table);
		// same as SECTOR_SIZE actually

	// check the offset
	if (offset < 0 || offset + toRead > fSessionSize) {
		TRACE(("intel: _ReadSector(): bad offset: %Ld\n", offset));
		return B_BAD_VALUE;
	}

	// read
	offset += fSessionOffset;
	if (read_pos(fDeviceFD, offset, pts, toRead) != toRead) {
#ifndef _BOOT_MODE
		status_t error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
#else
		status_t error = B_IO_ERROR;
#endif
		TRACE(("intel: _ReadSector(): reading the partition table failed: %lx\n",
			error));
		return error;
	}

	return B_OK;
}


// _WriteSector
/*! \brief Writes the sector to the disk.
*/
status_t
PartitionMapWriter::_WriteSector(off_t offset, const partition_table* pts)
{
	int32 toWrite = sizeof(partition_table);
		// same as SECTOR_SIZE actually

	// check the offset
	if (offset < 0 || offset + toWrite > fSessionSize) {
		TRACE(("intel: _WriteSector(): bad offset: %Ld\n", offset));
		return B_BAD_VALUE;
	}

	offset += fSessionOffset;

	// write
	if (write_pos(fDeviceFD, offset, pts, toWrite) != toWrite) {
		status_t error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;

		TRACE(("intel: _WriteSector(): writing the partition table failed: "
			"%lx\n", error));
		return error;
	}

	return B_OK;
}

