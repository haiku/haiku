//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file intel.cpp
	disk_scanner partition module for "intel" style partitions
*/

#include <errno.h>
#include <new.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_device.h>
#include <KernelExport.h>
#include <disk_scanner/partition.h>

#define TRACE(x) ;
//#define TRACE(x) dprintf x

#define INTEL_PARTITION_MODULE_NAME "disk_scanner/partition/intel/v1"

// partition module identifier
static const char *const kShortModuleName = "intel";

// Maximal number of logical partitions per extended partition we allow.
static const int32 kMaxLogicalPartitionCount = 128;

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

// is_empty_type
static inline
bool
is_empty_type(uint8 type)
{
	return (type == 0x00);
}

// is_extended_type
static inline
bool
is_extended_type(uint8 type)
{
	return (type == 0x05 || type == 0x0f || type == 0x85);
}

// chs
struct chs {
	uint8	cylinder;
	uint16	head_sector;	// head[15:10], sector[9:0]
} _PACKED;

// partition_descriptor
struct partition_descriptor {
	uint8	active;
	chs		begin;
	uint8	type;
	chs		end;
	uint32	start;
	uint32	size;

	bool is_empty() const { return is_empty_type(type); }
	bool is_extended() const { return is_extended_type(type); }
} _PACKED;

// partition_table_sector
struct partition_table_sector {
	char					pad1[446];
	partition_descriptor	table[4];
	uint16					signature;
} _PACKED;

static const uint16 kPartitionTableSectorSignature = 0xaa55;

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


// Partition
class Partition {
public:
	Partition();
	Partition(const partition_descriptor *descriptor, off_t ptsOffset,
			  off_t baseOffset, int32 blockSize);

	void SetTo(const partition_descriptor *descriptor, off_t ptsOffset,
			   off_t baseOffset, int32 blockSize);
	void Unset();

	bool IsEmpty() const { return is_empty_type(fType); }
	bool IsExtended() const { return is_extended_type(fType); }

	off_t PTSOffset() const	{ return fPTSOffset; }
	off_t Offset() const	{ return fOffset; }
	off_t Size() const		{ return fSize; }
	uint8 Type() const		{ return fType; }
	const char *TypeString() const { return partition_type_string(fType); }

	bool CheckLocation(off_t sessionSize) const;

private:
	off_t	fPTSOffset;
	off_t	fOffset;	// relative to the start of the session
	off_t	fSize;
	uint8	fType;
};

// constructor
Partition::Partition()
	: fPTSOffset(0),
	  fOffset(0),
	  fSize(0),
	  fType(0)
{
}

// constructor
Partition::Partition(const partition_descriptor *descriptor,off_t ptsOffset,
					 off_t baseOffset, int32 blockSize)
	: fPTSOffset(0),
	  fOffset(0),
	  fSize(0),
	  fType(0)
{
	SetTo(descriptor, ptsOffset, baseOffset, blockSize);
}

// SetTo
void
Partition::SetTo(const partition_descriptor *descriptor, off_t ptsOffset,
				 off_t baseOffset, int32 blockSize)
{
	fPTSOffset = ptsOffset;
	fOffset = baseOffset + (off_t)descriptor->start * blockSize;
	fSize = (off_t)descriptor->size * blockSize;
	fType = descriptor->type;
	if (fSize == 0)
		Unset();
}

// Unset
void
Partition::Unset()
{
	fPTSOffset = 0;
	fOffset = 0;
	fSize = 0;
	fType = 0;
}

// CheckLocation
bool
Partition::CheckLocation(off_t sessionSize) const
{
	return (fOffset >= 0 && fOffset + fSize <= sessionSize);
}


class LogicalPartition;

// PrimaryPartition

class PrimaryPartition : public Partition {
public:
	PrimaryPartition();
	PrimaryPartition(const partition_descriptor *descriptor, off_t ptsOffset,
					 int32 blockSize);

	void SetTo(const partition_descriptor *descriptor, off_t ptsOffset,
			   int32 blockSize);
	void Unset();

	// only if extended
	int32 CountLogicalPartitions() const { return fLogicalPartitionCount; }
	LogicalPartition *LogicalPartitionAt(int32 index) const;
	void AddLogicalPartition(LogicalPartition *partition);

private:
	LogicalPartition	*fHead;
	LogicalPartition	*fTail;
	int32				fLogicalPartitionCount;
};


// LogicalPartition
class LogicalPartition : public Partition {
public:
	LogicalPartition();
	LogicalPartition(const partition_descriptor *descriptor, off_t ptsOffset,
					 int32 blockSize, PrimaryPartition *primary);

	void SetTo(const partition_descriptor *descriptor, off_t ptsOffset,
			   int32 blockSize, PrimaryPartition *primary);
	void Unset();

	PrimaryPartition *GetPrimaryPartition() const { return fPrimary; }

	void SetNext(LogicalPartition *next) { fNext = next; }
	LogicalPartition *Next() const { return fNext; }

private:
	PrimaryPartition	*fPrimary;
	LogicalPartition	*fNext;
};


// PrimaryPartition

// constructor
PrimaryPartition::PrimaryPartition()
	: Partition(),
	  fHead(NULL),
	  fTail(NULL),
	  fLogicalPartitionCount(0)
{
}

// constructor
PrimaryPartition::PrimaryPartition(const partition_descriptor *descriptor,
								   off_t ptsOffset, int32 blockSize)
	: Partition(),
	  fHead(NULL),
	  fTail(NULL),
	  fLogicalPartitionCount(0)
{
	SetTo(descriptor, ptsOffset, blockSize);
}

// SetTo
void
PrimaryPartition::SetTo(const partition_descriptor *descriptor,
						off_t ptsOffset, int32 blockSize)
{
	Unset();
	Partition::SetTo(descriptor, ptsOffset, 0, blockSize);
}

// Unset
void
PrimaryPartition::Unset()
{
	while (LogicalPartition *partition = fHead) {
		fHead = partition->Next();
		delete partition;
	}
	fHead = NULL;
	fTail = NULL;
	fLogicalPartitionCount = 0;
	Partition::Unset();
}

// LogicalPartitionAt
LogicalPartition *
PrimaryPartition::LogicalPartitionAt(int32 index) const
{
	LogicalPartition *partition = NULL;
	if (index >= 0 && index < fLogicalPartitionCount) {
		for (partition = fHead; index > 0; index--)
			partition = partition->Next();
	}
	return partition;
}

// AddLogicalPartition
void
PrimaryPartition::AddLogicalPartition(LogicalPartition *partition)
{
	if (partition) {
		if (fTail) {
			fTail->SetNext(partition);
			fTail = partition;
		} else
			fHead = fTail = partition;
		partition->SetNext(NULL);
		fLogicalPartitionCount++;
	}
}


// LogicalPartition

// constructor
LogicalPartition::LogicalPartition()
	: Partition(),
	  fPrimary(NULL),
	  fNext(NULL)
{
}

// constructor
LogicalPartition::LogicalPartition(const partition_descriptor *descriptor,
								   off_t ptsOffset, int32 blockSize,
								   PrimaryPartition *primary)
	: Partition(),
	  fPrimary(NULL),
	  fNext(NULL)
{
	SetTo(descriptor, ptsOffset, blockSize, primary);
}

// SetTo
void
LogicalPartition::SetTo(const partition_descriptor *descriptor,
						off_t ptsOffset, int32 blockSize,
						PrimaryPartition *primary)
{
	Unset();
	if (descriptor && primary) {
		off_t baseOffset = (descriptor->is_extended() ? primary->Offset()
													  : ptsOffset);
		Partition::SetTo(descriptor, ptsOffset, baseOffset, blockSize);
		fPrimary = primary;
	}
}

// Unset
void
LogicalPartition::Unset()
{
	fPrimary = NULL;
	fNext = NULL;
	Partition::Unset();
}


// PartitionMap

class PartitionMap {
public:
	PartitionMap();
	~PartitionMap();

	void Unset();

	PrimaryPartition *PrimaryPartitionAt(int32 index);

	int32 CountPartitions() const;
	Partition *PartitionAt(int32 index);
	const Partition *PartitionAt(int32 index) const;

	bool Check() const;

private:
	PrimaryPartition		fPrimaries[4];
};

// constructor
PartitionMap::PartitionMap()
{
}

// destructor
PartitionMap::~PartitionMap()
{
}

// Unset
void
PartitionMap::Unset()
{
	for (int32 i = 0; i < 4; i++)
		fPrimaries[i].Unset();
}

// PrimaryPartitionAt
PrimaryPartition *
PartitionMap::PrimaryPartitionAt(int32 index)
{
	PrimaryPartition *partition = NULL;
	if (index >= 0 && index < 4)
		partition = fPrimaries + index;
	return partition;
}

// CountPartitions
int32
PartitionMap::CountPartitions() const
{
	int32 count = 4;
	for (int32 i = 0; i < 4; i++)
		count += fPrimaries[i].CountLogicalPartitions();
	return count;
}

// PartitionAt
Partition *
PartitionMap::PartitionAt(int32 index)
{
	Partition *partition = NULL;
	int32 count = CountPartitions();
	if (index >= 0 && index < count) {
		if (index < 4)
			partition  = fPrimaries + index;
		else {
			index -= 4;
			int32 primary = 0;
			while (index >= fPrimaries[primary].CountLogicalPartitions()) {
				index -= fPrimaries[primary].CountLogicalPartitions();
				primary++;
			}
			partition = fPrimaries[primary].LogicalPartitionAt(index);
		}
	}
	return partition;
}

// PartitionAt
const Partition *
PartitionMap::PartitionAt(int32 index) const
{
	return const_cast<PartitionMap*>(this)->PartitionAt(index);
}

// cmp_partition_offset
static
int
cmp_partition_offset(const void *p1, const void *p2)
{
	const Partition *partition1 = *static_cast<const Partition **>(p1);
	const Partition *partition2 = *static_cast<const Partition **>(p2);
	if (partition1->Offset() < partition2->Offset())
		return -1;
	else if (partition1->Offset() > partition2->Offset())
		return 1;
	return 0;
}

// cmp_offset
static
int
cmp_offset(const void *o1, const void *o2)
{
	off_t offset1 = *static_cast<const off_t*>(o1);
	off_t offset2 = *static_cast<const off_t*>(o2);
	if (offset1 < offset2)
		return -1;
	else if (offset1 > offset2)
		return 1;
	return 0;
}

// is_inside_partitions
static
bool
is_inside_partitions(off_t location, const Partition **partitions, int32 count)
{
	bool result = false;
	if (count > 0) {
		// binary search
		int32 lower = 0;
		int32 upper = count - 1;
		while (lower < upper) {
			int32 mid = (lower + upper) / 2;
			const Partition *midPartition = partitions[mid];
			if (location >= midPartition->Offset() + midPartition->Size())
				lower = mid + 1;
			else
				upper = mid;
		}
		const Partition *partition = partitions[lower];
		result = (location >= partition->Offset() &&
				  location < partition->Offset() + partition->Size());
	}
	return result;
}

// Check
bool
PartitionMap::Check() const
{
	bool result = true;
	int32 partitionCount = CountPartitions();
	const Partition **byOffset = new(nothrow) const Partition*[partitionCount];
	off_t *ptsOffsets = new(nothrow) off_t[partitionCount - 3];
	if (byOffset && ptsOffsets) {
		// fill the arrays
		int32 byOffsetCount = 0;
		int32 ptsOffsetCount = 1;	// primary PTS
		ptsOffsets[0] = 0;			//
		for (int32 i = 0; i < partitionCount; i++) {
			const Partition *partition = PartitionAt(i);
			if (!partition->IsExtended())
				byOffset[byOffsetCount++] = partition;
			// add only logical partition PTS locations
			if (i >= 4)
				ptsOffsets[ptsOffsetCount++] = partition->PTSOffset();
		}
		// sort the arrays
		qsort(byOffset, byOffsetCount, sizeof(const Partition*),
			  cmp_partition_offset);
		qsort(ptsOffsets, ptsOffsetCount, sizeof(off_t), cmp_offset);
		// check for overlappings
		off_t nextOffset = 0;
		for (int32 i = 0; i < byOffsetCount; i++) {
			const Partition *partition = byOffset[i];
			if (partition->Offset() < nextOffset) {
				TRACE(("intel: PartitionMap::Check(): overlapping partitions!"
					   "\n"));
				result = false;
				break;
			}
			nextOffset = partition->Offset() + partition->Size();
		}
		// check uniqueness of PTS offsets and whether they lie outside of the
		// non-extended partitions
		if (result) {
			for (int32 i = 0; i < ptsOffsetCount; i++) {
				if (i > 0 && ptsOffsets[i] == ptsOffsets[i - 1]) {
					TRACE(("intel: PartitionMap::Check(): same PTS for "
						   "different extended partitions!\n"));
					result = false;
					break;
				} else if (is_inside_partitions(ptsOffsets[i], byOffset,
												byOffsetCount)) {
					TRACE(("intel: PartitionMap::Check(): a PTS lies "
						   "inside a non-extended partition!\n"));
					result = false;
					break;
				}
			}
		}
	} else
		result = false;		// no memory: assume failure
	// cleanup
	if (byOffset)
		delete[] byOffset;
	if (ptsOffsets)
		delete[] ptsOffsets;
	return result;
}


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
	status_t _ReadPTS(off_t offset);

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
		const partition_table_sector *pts
			= (const partition_table_sector*)block;
		error = _ParsePrimary(pts);
		if (error == B_OK && !fMap->Check())
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
			if (!partition->CheckLocation(fSessionSize)) {
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
		if (++partitionCount > kMaxLogicalPartitionCount) {
			TRACE(("intel: _ParseExtended(): Maximal number of logical "
				   "partitions for extended partition reached. Cycle?\n"));
			error = B_BAD_DATA;
		}
		// read the PTS
		if (error == B_OK)
			error = _ReadPTS(offset);
		// check the signature
		if (error == B_OK && fPTS->signature != kPartitionTableSectorSignature) {
			TRACE(("intel: _ParseExtended(): invalid PTS signature\n"));
			error = B_BAD_DATA;
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
					if (partition && !partition->CheckLocation(fSessionSize))
						error = B_BAD_DATA;
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
PartitionMapParser::_ReadPTS(off_t offset)
{
	status_t error = B_OK;
	int32 toRead = sizeof(partition_table_sector);
	// check the offset
	if (offset < 0 || offset + toRead > fSessionSize) {
		error = B_BAD_VALUE;
		TRACE(("intel: _ReadPTS(): bad offset: %Ld\n", offset));
	// read
	} else if (read_pos(fDeviceFD, fSessionOffset + offset, fPTS, toRead)
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

// intel_identify
static
bool
intel_identify(int deviceFD, const session_info *sessionInfo,
			   const uchar *block)
{
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
	// read the partition structure
	if (result) {
		PartitionMapParser parser(deviceFD, sessionInfo->offset,
								  sessionInfo->size, blockSize);
		PartitionMap map;
		result = (parser.Parse(block, &map) == B_OK
				  && map.CountPartitions() > 0);
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
			// parse the partition map
			PartitionMapParser parser(deviceFD, sessionOffset, sessionSize,
									  blockSize);
			PartitionMap map;
			error = parser.Parse(block, &map);
			if (error == B_OK) {
				if (map.CountPartitions() == 0)
					error = B_BAD_DATA;
			}
			if (error == B_OK) {
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
					strcpy(partitionInfo->partition_type,
						   partition->TypeString());
					partitionInfo->partition_code = partition->Type();
				} else
					error = B_ENTRY_NOT_FOUND;
			}
		}
	}
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
	// not yet supported
	return B_UNSUPPORTED;
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

