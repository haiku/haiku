//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITIONING_INFO_H
#define _PARTITIONING_INFO_H

class BPartition;

class BPartitioningInfo {
public:
	status_t GetPartitionableSpaceAt(uint32 index, off_t *Offset, off_t *Size) const;
	uint32 CountPartitionableSpaces() const;

	BPartition* Parent() const;
private:
	off_t *fOffsetArray;
	off_t *fSizeArray;
	uint32 fCount;
};

#endif	// _PARTITIONING_INFO_H
