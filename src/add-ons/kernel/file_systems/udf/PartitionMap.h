//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_PARTITION_MAP_H
#define _UDF_PARTITION_MAP_H

/*! \file PartitionMap.h
*/

#include "kernel_cpp.h"
#include "UdfDebug.h"
#include "DiskStructures.h"

#include "SinglyLinkedList.h"

namespace Udf {

class PartitionMap {
public:
	PartitionMap();

	status_t Add(const udf_partition_descriptor* partition);
	const udf_partition_descriptor* Find(uint32 partitionNumber) const;
	const udf_partition_descriptor* operator[](uint32 partitionNumber) const;
	
	void dump();
private:
	SinglyLinkedList<Udf::udf_partition_descriptor> fList;
	uint32 fCount;
};

};	// namespace Udf

#endif	// _UDF_PARTITION_MAP_H
