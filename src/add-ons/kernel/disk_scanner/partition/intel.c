//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		intel.c
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	disk_scanner partition module for "intel" style partitions
//------------------------------------------------------------------------------

/*!
	\file intel.c
	disk_scanner partition module for "intel" style partitions
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_device.h>
#include <KernelExport.h>
#include <disk_scanner/partition.h>

#define TRACE(x)
//#define TRACE(x) dprintf x

#define INTEL_PARTITION_MODULE_NAME "disk_scanner/partition/intel/v1"

// the maximal number of partitions we support (size of some static arrays)
static const int32 MAX_PARTITION_COUNT = 64;

struct partition_type {
	uint8	type;
	char	*name;
};

static const struct partition_type gPartitionTypes[] = {
	// these entries must be sorted by type (currently not)
	{ 0x00, "empty" },
	{ 0x01, "FAT 12-bit" },
	{ 0x02, "Xenix root" },
	{ 0x03, "Xenix user" },
	{ 0x04, "FAT 16-bit (dos 3.0)" },
	{ 0x05, "Extended Partition" },
	{ 0x06, "FAT 16-bit (dos 3.31)" },
	{ 0x07, "OS/2 IFS, Windows NT, Advanced Unix" },
	{ 0x0b, "FAT 32-bit" },
	{ 0x0c, "FAT 32-bit, LBA-mapped" },
	{ 0x0d, "FAT 16-bit, LBA-mapped" },
	{ 0x0f, "Extended Partition, LBA-mapped" },
	{ 0x42, "Windows 2000 marker (switches to a proprietary partition table)" },
	{ 0x4d, "QNX 4" },
	{ 0x4e, "QNX 4 2nd part" },
	{ 0x4f, "QNX 4 3rd part" },
	{ 0x78, "XOSL boot loader" },
	{ 0x82, "Linux swapfile" },
	{ 0x83, "Linux native" },
	{ 0x85, "Linux extendend partition" },
	{ 0xa5, "FreeBSD" },
	{ 0xa6, "OpenBSD" },
	{ 0xa7, "NextSTEP" },
	{ 0xa8, "MacOS X" },
	{ 0xa9, "NetBSD" },
	{ 0xab, "MacOS X boot" },
	{ 0xbe, "Solaris 8 boot" },
	{ 0xeb, "BeOS" },
	{ 0, NULL }
};

// chs
struct chs {
	uint8	cylinder;
	uint16	head_sector;	// head[15:10], sector[9:0]
} _PACKED;
typedef struct chs chs;

// partition_descriptor
struct partition_descriptor {
	uint8	active;
	chs		begin;
	uint8	type;
	chs		end;
	uint32	start;
	uint32	size;
} _PACKED;
typedef struct partition_descriptor partition_descriptor;

// partition_table_sector
struct partition_table_sector {
	char					pad1[446];
	partition_descriptor	table[4];
	uint16					signature;
} _PACKED;
typedef struct partition_table_sector partition_table_sector;

static const uint16 kPartitionTableSectorSignature = 0xaa55;

// partition_spec
typedef struct partition_spec {
	struct partition_spec	*next;
	struct partition_spec	*primary;
	off_t					offset;
	off_t					size;
	off_t					extended_pts;
	uint8					type;
} partition_spec;

// partition_type_string
static
const char *
partition_type_string(uint8 type)
{
	int32 i;
	for (i = 0; gPartitionTypes[i].name ; i++)
	{
		if (type == gPartitionTypes[i].type)
			return gPartitionTypes[i].name;
	}
	return "unknown";
}

// new_partition_spec
static
partition_spec *
new_partition_spec(partition_spec *primary, off_t offset, off_t size,
				   off_t extended_pts, uint8 type)
{
	partition_spec *spec = (partition_spec*)malloc(sizeof(partition_spec));
	if (spec) {
		spec->next = NULL;
		spec->primary = primary;
		spec->offset = offset;
		spec->size = size;
		spec->extended_pts = extended_pts;
		spec->type = type;
	}
	return spec;
}

// is_empty_partition
static
bool
is_empty_partition(const partition_descriptor *descriptor)
{
	return (descriptor->size == 0);
}

// is_extended_type
static
bool
is_extended_type(uint8 type)
{
	return (type == 0x05 || type == 0x0f || type == 0x85);
}

// is_extended_partition
static
bool
is_extended_partition(const partition_descriptor *descriptor)
{
	return is_extended_type(descriptor->type);
}

// get_partition_dimension
static
void
get_partition_dimension(const partition_descriptor *descriptor,
						int32 blockSize, off_t baseOffset, off_t *offset,
						off_t *size)
{
	if (descriptor) {
		*offset = baseOffset + (off_t)descriptor->start * blockSize;
		*size = (off_t)descriptor->size * blockSize;
	}
}

// check_partition
static
status_t
check_partition(const partition_descriptor *descriptor, off_t primaryStart,
				off_t sessionSize)
{
	status_t error = B_OK;
	if (is_extended_partition(descriptor)) {
	} else {
	}
	return error;
}

// parse_extended_partition_map
static
status_t
parse_extended_partition_map(int deviceFD, off_t sessionOffset,
							 off_t sessionSize, partition_spec *primary,
							 off_t sectorStart, uint8 *buffer,
							 int32 blockSize, partition_spec **partitionList)
{
// TODO: Check for cycles, or at least set an upper bound for the number of
// partitions. Otherwise an ill-formated disk may bring down the whole
// system.
	status_t error = B_OK;
	const partition_table_sector *pts = (const partition_table_sector*)buffer;
	const partition_descriptor *extended = NULL;
	const partition_descriptor *nonExtended = NULL;
	off_t extendedStart = 0;
	off_t extendedSize = 0;
	off_t nonExtendedStart = 0;
	off_t nonExtendedSize = 0;
	// read partition table sector
	if (read_pos(deviceFD, sectorStart, buffer, blockSize) != blockSize) {
		error = errno;
		if (error == B_OK)
			error = B_ERROR;
		TRACE(("intel: parse_extended_partition_map: reading the PTS failed: "
			   "%s\n", strerror(error)));
		return B_OK;
	}
	// check the signature
	if (error == B_OK && pts->signature != kPartitionTableSectorSignature) {
		TRACE(("intel: parse_extended_partition_map: invalid PTS "
			   "signature\n"));
		return B_OK;
	}
	if (error == B_OK) {
		// check the partitions
		int32 i;
		for (i = 0; error == B_OK && i < 4; i++) {
			const partition_descriptor *descriptor = &pts->table[i];
			if (!is_empty_partition(descriptor)) {
				error = check_partition(descriptor, primary->offset,
										sessionSize);
				if (is_extended_partition(descriptor)) {
					if (extended) {
						// only one extended partition allowed
						error = B_ERROR;
						TRACE(("intel: parse_extended_partition_map: "
							   "only one extended partition allowed\n"));
					} else
						extended = descriptor;
				} else {
					if (nonExtended) {
						// only one non-ext. partition allowed
						error = B_ERROR;
						TRACE(("intel: parse_extended_partition_map: "
							   "only one non-extended partition allowed\n"));
					} else
						nonExtended = descriptor;
				}
				// simply ignore this PTS, if something is ill-formated
				if (error != B_OK)
					return B_OK;
			}
		}
		// get the partitions' dimensions
		if (error == B_OK) {
			get_partition_dimension(extended, blockSize, primary->offset,
									&extendedStart, &extendedSize);
			get_partition_dimension(nonExtended, blockSize, sectorStart,
									&nonExtendedStart, &nonExtendedSize);
		}
		// add a partition specification for the non-extended partition
		if (error == B_OK && nonExtended) {
			partition_spec *spec = new_partition_spec(primary,
				nonExtendedStart, nonExtendedSize, extendedStart,
				nonExtended->type);
			if (spec) {
				*partitionList = spec;
				partitionList = &spec->next;
			} else
				error = B_NO_MEMORY;
		}
		// parse the extended partition
		if (error == B_OK && extended) {
			error = parse_extended_partition_map(deviceFD,
				sessionOffset, sessionSize, primary, extendedStart,
				buffer, blockSize, partitionList);
		}
	}
	return error;
}

// parse_partition_map
static
status_t
parse_partition_map(int deviceFD, off_t sessionOffset, off_t sessionSize,
					const uint8 *block, int32 blockSize,
					partition_spec **partitionList)
{
	status_t error = B_OK;
	const partition_table_sector *pts = (const partition_table_sector*)block;
	uint8 *buffer = (uint8*)malloc(blockSize);
	partition_spec *primarySpecs[4];
	if (buffer) {
		// check the primary partitions
		int32 i;
		for (i = 0; error == B_OK && i < 4; i++) {
			const partition_descriptor *descriptor = &pts->table[i];
			if (!is_empty_partition(descriptor))
				error = check_partition(descriptor, 0, sessionSize);
			// add the partition specification
			if (error == B_OK) {
				off_t start = 0;
				off_t size = 0;
				partition_spec *spec;
				get_partition_dimension(descriptor, blockSize, 0, &start,
										&size);
				spec = new_partition_spec(0, start, size, 0, descriptor->type);
				if (spec) {
					*partitionList = spec;
					partitionList = &spec->next;
					primarySpecs[i] = spec;
				} else
					error = B_NO_MEMORY;
			}
		}
		// parse the extended partitions
		if (error == B_OK) {
			for (i = 0; error == B_OK && i < 4; i++) {
				partition_spec *spec = primarySpecs[i];
				if (is_extended_type(spec->type)) {
					error = parse_extended_partition_map(deviceFD,
						sessionOffset, sessionSize, spec, spec->offset,
						buffer, blockSize, partitionList);
				}
			}
		}
	} else
		error = B_NO_MEMORY;
	// cleanup
	if (buffer)
		free(buffer);
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

// intel_identify
static
bool
intel_identify(int deviceFD, const session_info *sessionInfo,
			   const uchar *block)
{
	const partition_table_sector *pts = (const partition_table_sector*)block;
	bool result = true;
	int32 blockSize = sessionInfo->logical_block_size;
	TRACE(("intel: identify(%d, %lld, %lld, %p, %ld)\n", deviceFD,
		   sessionInfo->offset, sessionInfo->size, block, blockSize));
	// check block size
	if (result) {
		result = ((uint32)blockSize >= sizeof(partition_table_sector));
		if (!result) {
			TRACE(("intel: identify: bad block size: %ld, should be >= %ld\n",
				   blockSize, sizeof(partition_table_sector)));
		}
	}
	// check PTS signature
	if (result) {
		result = (pts->signature == kPartitionTableSectorSignature);
		if (!result)
			TRACE(("intel: identify: bad signature: %x\n", pts->signature));
	}
	return result;
}

// intel_get_nth_info
static
status_t
intel_get_nth_info(int deviceFD, const session_info *sessionInfo,
				   const uchar *block, int32 index,
				   extended_partition_info *partitionInfo)
{
	status_t error = B_ENTRY_NOT_FOUND;
	off_t sessionOffset = sessionInfo->offset;
	off_t sessionSize = sessionInfo->size;
	int32 blockSize = sessionInfo->logical_block_size;
	TRACE(("intel: get_nth_info(%d, %lld, %lld, %p, %ld, %ld)\n", deviceFD,
		   sessionOffset, sessionSize, block, blockSize, index));
	if (intel_identify(deviceFD, sessionInfo, block)) {
		if (index >= 0) {
			partition_spec *partitionList = NULL;
			error = parse_partition_map(deviceFD, sessionOffset, sessionSize,
										block, blockSize, &partitionList);
			if (error == B_OK) {
				int32 i = 0;
				partition_spec *spec = partitionList;
				for (i = 0; spec && i < index; i++, spec = spec->next);
				if (spec) {
					if (spec->size == 0) {
						// empty partition
						partitionInfo->info.offset = sessionOffset;
						partitionInfo->info.size = 0;
						partitionInfo->flags
							= B_HIDDEN_PARTITION | B_EMPTY_PARTITION;
					} else {
						partitionInfo->info.offset = spec->offset;
						partitionInfo->info.size = spec->size;
						if (is_extended_type(spec->type))
							partitionInfo->flags = B_HIDDEN_PARTITION;
						else
							partitionInfo->flags = 0;
					}
					partitionInfo->partition_name[0] = '\0';
					strcpy(partitionInfo->partition_type,
						   partition_type_string(spec->type));
					partitionInfo->partition_code = spec->type;
				} else
					error = B_ENTRY_NOT_FOUND;
			}
			// free partition list
			if (partitionList) {
				partition_spec *spec = partitionList;
				while (spec) {
					partitionList = spec->next;
					free(spec);
					spec = partitionList;
				}
			}
		}
	}
	return error;
}


static partition_module_info intel_partition_module = 
{
	// module_info
	{
		INTEL_PARTITION_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	intel_identify,
	intel_get_nth_info,
};

_EXPORT partition_module_info *modules[] =
{
	&intel_partition_module,
	NULL
};

