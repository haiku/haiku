//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_SPARABLE_PARTITION_H
#define _UDF_SPARABLE_PARTITION_H

/*! \file SparablePartition.h
*/

#include <util/kernel_cpp.h>

#include "UdfStructures.h"
#include "Partition.h"
#include "UdfDebug.h"

/*! \brief Type 2 sparable partition

	Sparable partitions provide a defect-managed partition
	space for media that does not implicitly provide defect management,
	such as CD-RW. Arbitrary packets of blocks in the sparable partition
	may be transparently remapped to other locations on disc should the
	original locations become defective.
	
	Per UDF-2.01 2.2.11, sparable partitions shall be recorded only on
	disk/drive systems that do not perform defect management.
	
	See also UDF-2.01 2.2.9, UDF-2.01 2.2.11
*/
class SparablePartition : public Partition {
public:
	SparablePartition(uint16 number, uint32 start, uint32 length, uint16 packetLength,
	                  uint8 tableCount, uint32 *tableLocations);
	virtual ~SparablePartition();
	virtual status_t MapBlock(uint32 logicalBlock, off_t &physicalBlock);
	
	status_t InitCheck();

	uint16 Number() const { return fNumber; }
	uint32 Start() const { return fStart; }
	uint32 Length() const { return fLength; }
	uint32 PacketLength() const { return fPacketLength; }
	uint8 TableCount() const { return fTableCount; }
	
	//! Maximum number of redundant sparing tables per SparablePartition
	static const uint8 kMaxSparingTableCount = UDF_MAX_SPARING_TABLE_COUNT;
private:
	uint16 fNumber;
	uint32 fStart;
	uint32 fLength;
	uint32 fPacketLength;
	uint8 fTableCount;
	uint32 fTableLocations[kMaxSparingTableCount];
	status_t fInitStatus;
};

#endif	// _UDF_SPARABLE_PARTITION_H
