/*
 * Copyright 2003-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INTEL_PARTITION_MAP_H
#define _INTEL_PARTITION_MAP_H

/*!	\file PartitionMap.h
	\ingroup intel_module
	\brief Definitions for "intel" style partitions and interface definitions
		   for related classes.
*/

// NOTE: <http://www.win.tue.nl/~aeb/partitions/partition_tables-2.html>

#include <SupportDefs.h>
#include <driver_settings.h>

#include <disk_device_types.h>

#ifndef _USER_MODE
#	include <util/kernel_cpp.h>
#else
#	include <new>
#endif


// partition_type
struct partition_type {
	uint8		type;
	const char*	name;
	bool		used;
};


// is_empty_type
static inline bool
is_empty_type(uint8 type)
{
	return type == 0x00;
}


// is_extended_type
static inline bool
is_extended_type(uint8 type)
{
	return type == 0x05 || type == 0x0f || type == 0x85;
}


void get_partition_type_string(uint8 type, char* buffer);

// chs
// NOTE: The CHS cannot express locations within larger disks and is therefor
// mostly obsolete.
struct chs {
	uint8	cylinder;
	uint16	head_sector;	// head[15:10], sector[9:0]
	void Unset() { cylinder = 0; head_sector = 0; }
} _PACKED;

// partition_descriptor
struct partition_descriptor {
	uint8	active;
	chs		begin;				// mostly ignored
	uint8	type;				// empty, filesystem or extended
	chs		end;				// mostly ignored
	uint32	start;				// in sectors
	uint32	size;				// in sectors

	bool is_empty() const		{ return is_empty_type(type); }
	bool is_extended() const	{ return is_extended_type(type); }
} _PACKED;

// partition_table
struct partition_table {
	char					code_area[440];
	uint32					disk_id;
	uint16					reserved;
	partition_descriptor	table[4];
	uint16					signature;

	void clear_code_area()
	{
		memset(code_area, 0, sizeof(code_area));
	}

	void fill_code_area(const uint8* code, size_t size)
	{
		memcpy(code_area, code, min_c(sizeof(code_area), size));
	}
} _PACKED;

static const uint16 kPartitionTableSectorSignature = 0xaa55;

class Partition;
class PrimaryPartition;
class LogicalPartition;


// PartitionType
/*!
  \brief Class for validating partition types.

  To this class we can set a partition type and then we can check whether
  this type is valid, empty or if it represents an extended partition.
  We can also retrieve the name of that partition type or find the next
  supported type.
*/
class PartitionType {
public:
	PartitionType();

	bool SetType(uint8 type);
	bool SetType(const char* typeName);
	bool SetContentType(const char* contentType);

	bool IsValid() const	{ return fValid; }
	bool IsEmpty() const	{ return is_empty_type(fType); }
	bool IsExtended() const	{ return is_extended_type(fType); }

	uint8 Type() const		{ return fType; }
	bool FindNext();
	void GetTypeString(char* buffer) const
		{ get_partition_type_string(fType, buffer); }
private:
	uint8	fType;
	bool	fValid;
};


// Partition
class Partition {
public:
								Partition();
								Partition(const partition_descriptor* descriptor,
									off_t tableOffset, off_t baseOffset,
									uint32 blockSize);

			void				SetTo(const partition_descriptor* descriptor,
									off_t tableOffset, off_t baseOffset,
									uint32 blockSize);
			void				SetTo(off_t offset, off_t size, uint8 type,
									bool active, off_t tableOffset,
									uint32 blockSize);
			void				Unset();

			bool				IsEmpty() const
									{ return is_empty_type(fType); }
			bool				IsExtended() const
									{ return is_extended_type(fType); }

	// NOTE: Both PartitionTableOffset() and Offset() are absolute with regards
	// to the session (usually the disk). Ie, for all primary partitions,
	// including the primary extended partition, the PartitionTableOffset()
	// points to the MBR (0).
	// For logical partitions, the PartitionTableOffset() is located within the
	// primary extended partition, but again, the returned values are absolute
	// with regards to the session. All values are expressed in bytes.
			off_t				PartitionTableOffset() const
									{ return fPartitionTableOffset; }
									// offset of the partition table
			off_t				Offset() const		{ return fOffset; }
									// start offset of the partition contents
			off_t				Size() const		{ return fSize; }
			uint8				Type() const		{ return fType; }
			bool				Active() const		{ return fActive; }
			uint32				BlockSize() const	{ return fBlockSize; }
			void				GetTypeString(char* buffer) const
									{ get_partition_type_string(fType, buffer); }

			void				SetPartitionTableOffset(off_t offset)
									{ fPartitionTableOffset = offset; }
			void				SetOffset(off_t offset)
									{ fOffset = offset; }
			void				SetSize(off_t size)
									{ fSize = size; }
			void				SetType(uint8 type)
									{ fType = type; }
			void				SetActive(bool active)
									{ fActive = active; }
			void				SetBlockSize(uint32 blockSize)
									{ fBlockSize = blockSize; }

			bool				CheckLocation(off_t sessionSize) const;
			bool				FitSizeToSession(off_t sessionSize);

private:
			off_t				fPartitionTableOffset;
			off_t				fOffset;
				// relative to the start of the session
			off_t				fSize;
			uint32				fBlockSize;
			uint8				fType;
			bool				fActive;
};


// PrimaryPartition
class PrimaryPartition : public Partition {
public:
								PrimaryPartition();

			void				SetTo(const partition_descriptor* descriptor,
									off_t tableOffset, uint32 blockSize);
			void				SetTo(off_t offset, off_t size, uint8 type,
									bool active, uint32 blockSize);
			void				Unset();

			status_t			Assign(const PrimaryPartition& other);

			int32				Index() const			{ return fIndex; }
			void				SetIndex(int32 index)	{ fIndex = index; }
			void				GetPartitionDescriptor(
									partition_descriptor* descriptor) const;

				// private

			// only if extended
			int32				CountLogicalPartitions() const
									{ return fLogicalPartitionCount; }
			LogicalPartition*	LogicalPartitionAt(int32 index) const;
			void				AddLogicalPartition(LogicalPartition* partition);
			void				RemoveLogicalPartition(
									LogicalPartition* partition);

private:
			LogicalPartition*	fHead;
			LogicalPartition*	fTail;
			int32				fLogicalPartitionCount;
			int32				fIndex;
};


// LogicalPartition
class LogicalPartition : public Partition {
public:
								LogicalPartition();
								LogicalPartition(
									const partition_descriptor* descriptor,
									off_t tableOffset,
									PrimaryPartition* primary);

			void				SetTo(const partition_descriptor* descriptor,
									off_t tableOffset,
									PrimaryPartition* primary);
			void				SetTo(off_t offset, off_t size, uint8 type,
									bool active, off_t tableOffset,
									PrimaryPartition* primary);
			void				Unset();
			void				GetPartitionDescriptor(
									partition_descriptor* descriptor,
									bool inner = false) const;


			void				SetPrimaryPartition(PrimaryPartition* primary)
									{ fPrimary = primary; }
			PrimaryPartition*	GetPrimaryPartition() const
									{ return fPrimary; }

			void				SetNext(LogicalPartition* next)
									{ fNext = next; }
			LogicalPartition*	Next() const
									{ return fNext; }

			void				SetPrevious(LogicalPartition* previous)
									{ fPrevious = previous; }
			LogicalPartition*	Previous() const
									{ return fPrevious; }

private:
			PrimaryPartition*	fPrimary;
			LogicalPartition*	fNext;
			LogicalPartition*	fPrevious;
};


// PartitionMap
class PartitionMap {
public:
								PartitionMap();
								~PartitionMap();

			void				Unset();

			status_t			Assign(const PartitionMap& other);

			PrimaryPartition*	PrimaryPartitionAt(int32 index);
			const PrimaryPartition* PrimaryPartitionAt(int32 index) const;
			int32				IndexOfPrimaryPartition(
									const PrimaryPartition* partition) const;
			int32				CountNonEmptyPrimaryPartitions() const;

			int32				ExtendedPartitionIndex() const;

			int32				CountPartitions() const;
			int32				CountNonEmptyPartitions() const;
			Partition*			PartitionAt(int32 index);
			const Partition*	PartitionAt(int32 index) const;

			bool				Check(off_t sessionSize) const;
			const partition_type* GetNextSupportedPartitionType(uint32 cookie);

private:
			PrimaryPartition	fPrimaries[4];
};

#endif	// _INTEL_PARTITION_MAP_H
