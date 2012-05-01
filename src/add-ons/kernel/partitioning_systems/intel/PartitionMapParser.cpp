/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
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
#include "PartitionMapParser.h"


//#define TRACE_ENABLED
#ifdef TRACE_ENABLED
#	ifdef _USER_MODE
#		define TRACE(x) printf x
#	else
#		define TRACE(x) dprintf x
#	endif
#else
#	define TRACE(x) ;
#endif

#ifdef _USER_MODE
#	define ERROR(x) printf x
#else
#	define ERROR(x) dprintf x
#endif

using std::nothrow;

// Maximal number of logical partitions per extended partition we allow.
static const int32 kMaxLogicalPartitionCount = 128;

// Constants used to verify if a disk uses a GPT
static const int32 kGPTSignatureSize = 8;
static const char kGPTSignature[8] = { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T' };


// constructor
PartitionMapParser::PartitionMapParser(int deviceFD, off_t sessionOffset,
		off_t sessionSize, uint32 blockSize)
	:
	fDeviceFD(deviceFD),
	fBlockSize(blockSize),
	fSessionOffset(sessionOffset),
	fSessionSize(sessionSize),
	fPartitionTable(NULL),
	fMap(NULL)
{
}


// destructor
PartitionMapParser::~PartitionMapParser()
{
}


// Parse
status_t
PartitionMapParser::Parse(const uint8* block, PartitionMap* map)
{
	if (map == NULL)
		return B_BAD_VALUE;

	status_t error;
	bool hadToReFitSize = false;

	fMap = map;
	fMap->Unset();

	if (block) {
		const partition_table* table = (const partition_table*)block;
		error = _ParsePrimary(table, hadToReFitSize);
	} else {
		partition_table table;
		error = _ReadPartitionTable(0, &table);
		if (error == B_OK) {
			error = _ParsePrimary(&table, hadToReFitSize);

			if (fBlockSize != 512 && (hadToReFitSize
					|| !fMap->Check(fSessionSize))) {
				// This might be a fixed 512 byte MBR on a non-512 medium.
				// We do that for the anyboot images for example. so retry
				// with a fixed 512 block size and see if we get better
				// results
				int32 previousPartitionCount = fMap->CountNonEmptyPartitions();
				uint32 previousBlockSize = fBlockSize;
				TRACE(("intel: Parse(): trying with a fixed 512 block size\n"));

				fBlockSize = 512;
				fMap->Unset();
				error = _ParsePrimary(&table, hadToReFitSize);

				if (fMap->CountNonEmptyPartitions() < previousPartitionCount
					|| error != B_OK || hadToReFitSize
					|| !fMap->Check(fSessionSize)) {
					// That didn't improve anything, let's revert.
					TRACE(("intel: Parse(): try failed, reverting\n"));
					fBlockSize = previousBlockSize;
					fMap->Unset();
					error = _ParsePrimary(&table, hadToReFitSize);
				}
			}
		}
	}

	if (error == B_OK && !fMap->Check(fSessionSize))
		error = B_BAD_DATA;

	fMap = NULL;

	return error;
}


// _ParsePrimary
status_t
PartitionMapParser::_ParsePrimary(const partition_table* table,
	bool& hadToReFitSize)
{
	if (table == NULL)
		return B_BAD_VALUE;

	// check the signature
	if (table->signature != kPartitionTableSectorSignature) {
		TRACE(("intel: _ParsePrimary(): invalid PartitionTable signature: %lx\n",
			(uint32)table->signature));
		return B_BAD_DATA;
	}

	hadToReFitSize = false;

	// examine the table
	for (int32 i = 0; i < 4; i++) {
		const partition_descriptor* descriptor = &table->table[i];
		PrimaryPartition* partition = fMap->PrimaryPartitionAt(i);
		partition->SetTo(descriptor, 0, fBlockSize);

		// work-around potential BIOS/OS problems
		hadToReFitSize |= partition->FitSizeToSession(fSessionSize);

		// ignore, if location is bad
		if (!partition->CheckLocation(fSessionSize)) {
			TRACE(("intel: _ParsePrimary(): partition %ld: bad location, "
				"ignoring\n", i));
			partition->Unset();
		}
	}

	// allocate a partition_table buffer
	fPartitionTable = new(nothrow) partition_table;
	if (fPartitionTable == NULL)
		return B_NO_MEMORY;

	// parse extended partitions
	status_t error = B_OK;
	for (int32 i = 0; error == B_OK && i < 4; i++) {
		PrimaryPartition* primary = fMap->PrimaryPartitionAt(i);
		if (primary->IsExtended())
			error = _ParseExtended(primary, primary->Offset());
	}

	// cleanup
	delete fPartitionTable;
	fPartitionTable = NULL;

	return error;
}


// _ParseExtended
status_t
PartitionMapParser::_ParseExtended(PrimaryPartition* primary, off_t offset)
{
	status_t error = B_OK;
	int32 partitionCount = 0;
	while (error == B_OK) {
		// check for cycles
		if (++partitionCount > kMaxLogicalPartitionCount) {
			TRACE(("intel: _ParseExtended(): Maximal number of logical "
				   "partitions for extended partition reached. Cycle?\n"));
			error = B_BAD_DATA;
		}

		// read the partition table
		if (error == B_OK)
			error = _ReadPartitionTable(offset);

		// check the signature
		if (error == B_OK
			&& fPartitionTable->signature != kPartitionTableSectorSignature) {
			TRACE(("intel: _ParseExtended(): invalid partition table signature: "
				"%lx\n", (uint32)fPartitionTable->signature));
			error = B_BAD_DATA;
		}

		// ignore the partition table, if any error occured till now
		if (error != B_OK) {
			TRACE(("intel: _ParseExtended(): ignoring this partition table\n"));
			error = B_OK;
			break;
		}

		// Examine the table, there is exactly one extended and one
		// non-extended logical partition. All four table entries are
		// examined though. If there is no inner extended partition,
		// the end of the linked list is reached.
		// The first partition table describing both an "inner extended" parition
		// and a "data" partition (non extended and not empty) is the start
		// sector of the primary extended partition. The next partition table in
		// the linked list is the start sector of the inner extended partition
		// described in this partition table.
		LogicalPartition extended;
		LogicalPartition nonExtended;
		for (int32 i = 0; error == B_OK && i < 4; i++) {
			const partition_descriptor* descriptor = &fPartitionTable->table[i];
			if (descriptor->is_empty())
				continue;

			LogicalPartition* partition = NULL;
			if (descriptor->is_extended()) {
				if (extended.IsEmpty()) {
					extended.SetTo(descriptor, offset, primary);
					partition = &extended;
				} else {
					// only one extended partition allowed
					error = B_BAD_DATA;
					TRACE(("intel: _ParseExtended(): "
						   "only one extended partition allowed\n"));
				}
			} else {
				if (nonExtended.IsEmpty()) {
					nonExtended.SetTo(descriptor, offset, primary);
					partition = &nonExtended;
				} else {
					// only one non-extended partition allowed
					error = B_BAD_DATA;
					TRACE(("intel: _ParseExtended(): only one "
						   "non-extended partition allowed\n"));
				}
			}
			if (partition == NULL)
				break;

			// work-around potential BIOS/OS problems
			partition->FitSizeToSession(fSessionSize);

			// check the partition's location
			if (!partition->CheckLocation(fSessionSize)) {
				error = B_BAD_DATA;
				TRACE(("intel: _ParseExtended(): Invalid partition "
					"location: pts: %lld, offset: %lld, size: %lld\n",
					partition->PartitionTableOffset(), partition->Offset(),
					partition->Size()));
			}
		}

		// add non-extended partition to list
		if (error == B_OK && !nonExtended.IsEmpty()) {
			LogicalPartition* partition
				= new(nothrow) LogicalPartition(nonExtended);
			if (partition)
				primary->AddLogicalPartition(partition);
			else
				error = B_NO_MEMORY;
		}

		// prepare to parse next extended/non-extended partition pair
		if (error == B_OK && !extended.IsEmpty())
			offset = extended.Offset();
		else
			break;
	}

	return error;
}


// _ReadPartitionTable
status_t
PartitionMapParser::_ReadPartitionTable(off_t offset, partition_table* table)
{
	int32 toRead = sizeof(partition_table);

	// check the offset
	if (offset < 0 || offset + toRead > fSessionSize) {
		TRACE(("intel: _ReadPartitionTable(): bad offset: %Ld\n", offset));
		return B_BAD_VALUE;
	}

	if (table == NULL)
		table = fPartitionTable;

	// Read the partition table from the device into the table structure
	ssize_t bytesRead = read_pos(fDeviceFD, fSessionOffset + offset,
		table, toRead);
	if (bytesRead != (ssize_t)toRead) {
		TRACE(("intel: _ReadPartitionTable(): reading the partition "
			"table failed: %lx\n", errno));
		return bytesRead < 0 ? errno : B_IO_ERROR;
	}

	// check for GPT signature "EFI PART"
	// located in the 8bytes following the mbr
	toRead = kGPTSignatureSize;
	char gptSignature[8];
	bytesRead = read_pos(fDeviceFD, fSessionOffset + offset
		+ sizeof(partition_table), &gptSignature, toRead);
	if (bytesRead != (ssize_t)toRead) {
		TRACE(("intel: _ReadPartitionTable(): checking for GPT "
			"signature failed: %lx\n", errno));
		return bytesRead < 0 ? errno : B_IO_ERROR;
	}
	if (memcmp(gptSignature, kGPTSignature, kGPTSignatureSize) == 0) {
		ERROR(("Error: Disk is GPT-formatted: "
			"GPT disks are currently unsupported.\n"));
		return B_BAD_DATA;
	}

	return B_OK;
}

