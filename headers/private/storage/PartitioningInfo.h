//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITIONING_INFO_H
#define _PARTITIONING_INFO_H

class BPartition;

class BPartitioningInfo {
public:
	status_t GetPartitionableSpaceAt(int32 index, off_t *offset,
									 off_t *size) const;
	int32 CountPartitionableSpaces() const;

	BPartition *Parent() const;	// needed?

private:
	off_t	*fOffsetArray;
	off_t	*fSizeArray;
	int32	fCount;
};

#endif	// _PARTITIONING_INFO_H
