//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITIONABLE_SPACE_H
#define _PARTITIONABLE_SPACE_H

class BPartition;

class BPartitionableSpace {
public:
	off_t Offset() const;		
	off_t Size() const;
	int32 Index() const;		

	BPartition* Parent() const;
};

#endif	// _PARTITIONABLE_SPACE_H
