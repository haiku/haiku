//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file intel.cpp
	\brief disk_scanner partition module for "intel" style partitions.
*/

#include <errno.h>
#include <new.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <disk_scanner.h>
#include <KernelExport.h>
#include <disk_scanner/partition.h>

#include "intel_parameters.h"
#include "intel_partition_map.h"

#define TRACE(x) ;
//#define TRACE(x) dprintf x

#define INTEL_PARTITION_MODULE_NAME "disk_scanner/partition/intel/v1"

// partition module identifier
static const char *const kShortModuleName = "intel";

// Maximal number of logical partitions per extended partition we allow.
static const int32 kMaxLogicalPartitionCount = 128;


// PartitionMapParser

class PartitionMapParser {
public:
	PartitionMapParser(int deviceFD, off_t sessionOffset, off_t sessionSize,
					   int32 blockSize);
	~PartitionMapParser();

	status_t Parse(const uint8 *block, PartitionMap *map);

	int32 CountPartitions() const;
	const Partition *PartitionAt(int32 index) const;

private:
	status_t _ParsePrimary(const partition_table_sector *pts);
	status_t _ParseExtended(PrimaryPartition *primary, off_t offset);
	status_t _ReadPTS(off_t offset, partition_table_sector *pts = NULL);

private:
	int						fDeviceFD;
	off_t					fSessionOffset;
	off_t					fSessionSize;
	int32					fBlockSize;
	partition_table_sector	*fPTS;	// while parsing
	PartitionMap			*fMap;	//
};

// constructor
PartitionMapParser::PartitionMapParser(int deviceFD, off_t sessionOffset,
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
PartitionMapParser::~PartitionMapParser()
{
}

// Parse
status_t
PartitionMapParser::Parse(const uint8 *block, PartitionMap *map)
{
	status_t error = (block && map ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fMap = map;
		fMap->Unset();
		if (block) {
			const partition_table_sector *pts
				= (const partition_table_sector*)block;
			error = _ParsePrimary(pts);
		} else {
			partition_table_sector pts;
			error = _ReadPTS(0, &pts);
			if (error == B_OK)
				error = _ParsePrimary(&pts);
		}
		if (error == B_OK && !fMap->Check(fSessionSize, fBlockSize))
			error = B_BAD_DATA;
		fMap = NULL;
	}
	return error;
}

// _ParsePrimary
status_t
PartitionMapParser::_ParsePrimary(const partition_table_sector *pts)
{
	status_t error = (pts ? B_OK : B_BAD_VALUE);
	// check the signature
	if (error == B_OK && pts->signature != kPartitionTableSectorSignature) {
		TRACE(("intel: _ParsePrimary(): invalid PTS signature\n"));
		error = B_BAD_DATA;
	}
	// examine the table
	if (error == B_OK) {
		for (int32 i = 0; i < 4; i++) {
			const partition_descriptor *descriptor = &pts->table[i];
			PrimaryPartition *partition = fMap->PrimaryPartitionAt(i);
			partition->SetTo(descriptor, 0, fBlockSize);
			// fail, if location is bad
			if (!partition->CheckLocation(fSessionSize, fBlockSize)) {
				error = B_BAD_DATA;
				break;
			}
		}
	}
	// allocate a PTS buffer
	if (error == B_OK) {
		fPTS = new(nothrow) partition_table_sector;
		if (!fPTS)
			error = B_NO_MEMORY;
	}
	// parse extended partitions
	if (error == B_OK) {
		for (int32 i = 0; error == B_OK && i < 4; i++) {
			PrimaryPartition *primary = fMap->PrimaryPartitionAt(i);
			if (primary->IsExtended())
				error = _ParseExtended(primary, primary->Offset());
		}
	}
	// cleanup
	if (fPTS) {
		delete fPTS;
		fPTS = NULL;
	}
	return error;
}

// _ParseExtended
status_t
PartitionMapParser::_ParseExtended(PrimaryPartition *primary, off_t offset)
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
		// read the PTS
		if (error == B_OK)
			error = _ReadPTS(offset);
		// check the signature
		if (error == B_OK
			&& fPTS->signature != kPartitionTableSectorSignature) {
			TRACE(("intel: _ParseExtended(): invalid PTS signature\n"));
			error = B_BAD_DATA;
		}
		// ignore the PTS, if any error occured till now
		if (error != B_OK) {
			TRACE(("intel: _ParseExtended(): ignoring this PTS\n"));
			error = B_OK;
			break;
		}
		// examine the table
		LogicalPartition extended;
		LogicalPartition nonExtended;
		if (error == B_OK) {
			for (int32 i = 0; error == B_OK && i < 4; i++) {
				const partition_descriptor *descriptor = &fPTS->table[i];
				LogicalPartition *partition = NULL;
				if (!descriptor->is_empty()) {
					if (descriptor->is_extended()) {
						if (extended.IsEmpty()) {
							extended.SetTo(descriptor, offset, fBlockSize,
										   primary);
							partition = &extended;
						} else {
							// only one extended partition allowed
							error = B_BAD_DATA;
							TRACE(("intel: _ParseExtended(): "
								   "only one extended partition allowed\n"));
						}
					} else {
						if (nonExtended.IsEmpty()) {
							nonExtended.SetTo(descriptor, offset, fBlockSize,
											  primary);
							partition = &nonExtended;
						} else {
							// only one non-extended partition allowed
							error = B_BAD_DATA;
							TRACE(("intel: _ParseExtended(): only one "
								   "non-extended partition allowed\n"));
						}
					}
					// check the partition's location
					if (partition && !partition->CheckLocation(fSessionSize,
															   fBlockSize)) {
						error = B_BAD_DATA;
					}
				}
			}
		}
		// add non-extended partition to list
		if (error == B_OK && !nonExtended.IsEmpty()) {
			LogicalPartition *partition
				= new(nothrow) LogicalPartition(nonExtended);
			if (partition)
				primary->AddLogicalPartition(partition);
			else
				error = B_NO_MEMORY;
		}
		// prepare to parse next extended partition
		if (error == B_OK && !extended.IsEmpty())
			offset = extended.Offset();
		else
			break;
	}
	return error;
}

// _ReadPTS
status_t
PartitionMapParser::_ReadPTS(off_t offset, partition_table_sector *pts)
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
		error = errno;
		if (error == B_OK)
			error = B_IO_ERROR;
		TRACE(("intel: _ReadPTS(): reading the PTS failed: %s\n",
			   strerror(error)));
	}
	return error;
}


// std_ops
static
status_t
std_ops(int32 op, ...)
{
	TRACE(("intel: std_ops(0x%lx)\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// read_partition_map
static
status_t
read_partition_map(int deviceFD, const session_info *sessionInfo,
				   const uchar *block, PartitionMap *map)
{
	bool result = true;
	off_t sessionOffset = sessionInfo->offset;
	off_t sessionSize = sessionInfo->size;
	int32 blockSize = sessionInfo->logical_block_size;
	TRACE(("intel: read_partition_map(%d, %lld, %lld, %p, %ld)\n", deviceFD,
		   sessionOffset, sessionSize, block, blockSize));
	// check block size
	if (result) {
		result = ((uint32)blockSize >= sizeof(partition_table_sector));
		if (!result) {
			TRACE(("intel: read_partition_map: bad block size: %ld, should be "
				   ">= %ld\n", blockSize, sizeof(partition_table_sector)));
		}
	}
	// read the partition structure
	if (result) {
		PartitionMapParser parser(deviceFD, sessionOffset, sessionSize,
								  blockSize);
		result = (parser.Parse(block, map) == B_OK);
	}
	return result;
}

// intel_identify
static
bool
intel_identify(int deviceFD, const session_info *sessionInfo,
			   const uchar *block)
{
	TRACE(("intel: identify(%d, %lld, %lld, %p, %ld)\n", deviceFD,
		   sessionInfo->offset, sessionInfo->size, block,
		   sessionInfo->logical_block_size));
	PartitionMap map;
	return read_partition_map(deviceFD, sessionInfo, block, &map);
}

// intel_get_nth_info
static
status_t
intel_get_nth_info(int deviceFD, const session_info *sessionInfo,
				   const uchar *block, int32 index,
				   extended_partition_info *partitionInfo)
{
	status_t error = B_OK;
	off_t sessionOffset = sessionInfo->offset;
	TRACE(("intel: get_nth_info(%d, %lld, %lld, %p, %ld, %ld)\n", deviceFD,
		   sessionOffset, sessionInfo->size, block,
		   sessionInfo->logical_block_size, index));
	PartitionMap map;
	if (read_partition_map(deviceFD, sessionInfo, block, &map)) {
		if (Partition *partition = map.PartitionAt(index)) {
			if (partition->IsEmpty()) {
				// empty partition
				partitionInfo->info.offset = sessionOffset;
				partitionInfo->info.size = 0;
				partitionInfo->flags
					= B_HIDDEN_PARTITION | B_EMPTY_PARTITION;
			} else {
				// non-empty partition
				partitionInfo->info.offset
					= partition->Offset() + sessionOffset;
				partitionInfo->info.size = partition->Size();
				if (partition->IsExtended())
					partitionInfo->flags = B_HIDDEN_PARTITION;
				else
					partitionInfo->flags = 0;
			}
			partitionInfo->partition_name[0] = '\0';
			partition->GetTypeString(partitionInfo->partition_type);
		} else
			error = B_ENTRY_NOT_FOUND;
	} else	// couldn't read partition map -- we shouldn't be in get_nth_info()
		error = B_BAD_DATA;
	return error;
}

// intel_get_partitioning_params
static
status_t
intel_get_partitioning_params(int deviceFD,
							  const struct session_info *sessionInfo,
							  char *buffer, size_t bufferSize,
							  size_t *actualSize)
{
	status_t error = B_OK;
	PartitionMap map;
	if (!read_partition_map(deviceFD, sessionInfo, NULL, &map)) {
		// couldn't read partition map, set up a default one:
		// four empty primary partitions
		map.Unset();
	}
	// get the parameter length
	size_t length = 0;
	if (error == B_OK) {
		ParameterUnparser unparser;
		error = unparser.GetParameterLength(&map, &length);
	}
	// write the parameters
	if (error == B_OK && length <= bufferSize) {
		ParameterUnparser unparser;
		error = unparser.Unparse(&map, buffer, bufferSize);
	}
	// set the results
	if (error == B_OK)
		*actualSize = length;
	return error;
}

// intel_partition
static
status_t
intel_partition(int deviceFD, const struct session_info *sessionInfo,
				const char *parameters)
{
	// not yet supported
	return B_UNSUPPORTED;
}


static partition_module_info intel_partition_module = 
{
	// module_info
	{
		INTEL_PARTITION_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	kShortModuleName,

	intel_identify,
	intel_get_nth_info,
	intel_get_partitioning_params,
	intel_partition,
};

extern "C" partition_module_info *modules[];
_EXPORT partition_module_info *modules[] =
{
	&intel_partition_module,
	NULL
};

