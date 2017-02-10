//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_METADATA_PARTITION_H
#define _UDF_METADATA_PARTITION_H

/*! \file MetadataPartition.h
*/

#include <kernel_cpp.h>

#include "Partition.h"
#include "UdfDebug.h"

namespace Udf {

/*! \brief Type 2 metadata partition

	Metadata partitions allow for clustering of metadata (ICBs, directory
	contents, etc.) and also provide the option of metadata duplication.
	
	See also UDF-2.50 2.2.10, UDF-2.50 2.2.13
*/
class MetadataPartition : public Partition {
public:
	MetadataPartition(Partition &parentPartition, uint32 metadataFileLocation,
	                  uint32 metadataMirrorFileLocation, uint32 metadataBitmapFileLocation,
	                  uint32 allocationUnitSize, uint16 alignmentUnitSize,
	                  bool metadataIsDuplicated);
	virtual ~MetadataPartition();
	virtual status_t MapBlock(uint32 logicalBlock, off_t &physicalBlock);
	
	status_t InitCheck();

	uint32 AllocationUnitSize() const { return fAllocationUnitSize; }
	uint16 AlignmentUnitSize() const { return fAlignmentUnitSize; }
	uint32 MetadataIsDuplicated() const { return fMetadataIsDuplicated; }
private:
	Partition &fParentPartition;
	uint32 fAllocationUnitSize;
	uint16 fAlignmentUnitSize;
	bool fMetadataIsDuplicated;
	status_t fInitStatus;
};

};	// namespace Udf

#endif	// _UDF_METADATA_PARTITION_H
