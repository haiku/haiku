//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITIONING_INFO_H
#define _PARTITIONING_INFO_H

#include <SupportDefs.h>

class BPartitioningInfo {
public:
	status_t GetPartitionableSpaceAt(int32 index, off_t *offset,
									 off_t *size) const;
	int32 CountPartitionableSpaces() const;

private:
	off_t	*fOffsets;
	off_t	*fSizes;
	int32	fCount;
};

#endif	// _PARTITIONING_INFO_H
