//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file PartitionMap.h
	\brief Definitions for "intel" style partitions and interface definitions
		   for related classes.
*/

#ifndef _INTEL_PARTITION_MAP_H
#define _INTEL_PARTITION_MAP_H

#include <SupportDefs.h>
#include <util/kernel_cpp.h>

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

void get_partition_type_string(uint8 type, char *buffer);

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

	bool is_empty() const		{ return is_empty_type(type); }
	bool is_extended() const	{ return is_extended_type(type); }
} _PACKED;

// partition_table_sector
struct partition_table_sector {
	char					pad1[446];
	partition_descriptor	table[4];
	uint16					signature;
} _PACKED;

static const uint16 kPartitionTableSectorSignature = 0xaa55;

class Partition;
class PrimaryPartition;
class LogicalPartition;

// Partition
class Partition {
public:
	Partition();
	Partition(const partition_descriptor *descriptor, off_t ptsOffset,
			  off_t baseOffset, int32 blockSize);

	void SetTo(const partition_descriptor *descriptor, off_t ptsOffset,
			   off_t baseOffset, int32 blockSize);
	void Unset();

	bool IsEmpty() const	{ return is_empty_type(fType); }
	bool IsExtended() const	{ return is_extended_type(fType); }

	off_t PTSOffset() const	{ return fPTSOffset; }
	off_t Offset() const	{ return fOffset; }
	off_t Size() const		{ return fSize; }
	uint8 Type() const		{ return fType; }
	bool Active() const		{ return fActive; }
	void GetTypeString(char *buffer) const
		{ get_partition_type_string(fType, buffer); }

	void SetPTSOffset(off_t offset)	{ fPTSOffset = offset; }
	void SetOffset(off_t offset)	{ fOffset = offset; }
	void SetSize(off_t size)		{ fSize = size; }
	void SetType(uint8 type)		{ fType = type; }
	void SetActive(bool active)		{ fActive = active; }

	bool CheckLocation(off_t sessionSize, int32 blockSize) const;

private:
	off_t	fPTSOffset;
	off_t	fOffset;	// relative to the start of the session
	off_t	fSize;
	uint8	fType;
	bool	fActive;
};

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

	void SetPrimaryPartition(PrimaryPartition *primary) { fPrimary = primary; }
	PrimaryPartition *GetPrimaryPartition() const { return fPrimary; }

	void SetNext(LogicalPartition *next) { fNext = next; }
	LogicalPartition *Next() const { return fNext; }

private:
	PrimaryPartition	*fPrimary;
	LogicalPartition	*fNext;
};

// PartitionMap
class PartitionMap {
public:
	PartitionMap();
	~PartitionMap();

	void Unset();

	PrimaryPartition *PrimaryPartitionAt(int32 index);
	const PrimaryPartition *PrimaryPartitionAt(int32 index) const;

	int32 CountPartitions() const;
	Partition *PartitionAt(int32 index);
	const Partition *PartitionAt(int32 index) const;

	bool Check(off_t sessionSize, int32 blockSize) const;

private:
	PrimaryPartition		fPrimaries[4];
};

#endif	// _INTEL_PARTITION_MAP_H
