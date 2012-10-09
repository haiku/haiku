/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2003 Tyler Dauwalder, tyler@dauwalder.net
 * This file may be used under the terms of the MIT License.
 */


#include "MetadataPartition.h"

#include "Icb.h"


/*! \brief Creates a new MetadataPartition object.
*/
MetadataPartition::MetadataPartition(Volume *volume,
	uint16 parentNumber, Partition &parentPartition, uint32 metadataFileLocation,
	uint32 metadataMirrorFileLocation, uint32 metadataBitmapFileLocation,
	uint32 allocationUnitSize, uint16 alignmentUnitSize,
	bool metadataIsDuplicated)
	: fPartition(parentNumber),
	fParentPartition(parentPartition),
	fAllocationUnitSize(allocationUnitSize),
	fAlignmentUnitSize(alignmentUnitSize),
	fMetadataIsDuplicated(metadataIsDuplicated),
	fInitStatus(B_NO_INIT),
	fMetadataIcb(NULL)
{
	long_address address;
	address.set_to(metadataFileLocation, fPartition);
	
	fMetadataIcb = new(nothrow) Icb(volume, address);
	if (fMetadataIcb == NULL || fMetadataIcb->InitCheck() != B_OK)
		fInitStatus = B_NO_MEMORY;

	fInitStatus = B_OK;
}

/*! \brief Destroys the MetadataPartition object.
*/
MetadataPartition::~MetadataPartition()
{
	delete fMetadataIcb;
}

/*! \brief Maps the given logical block to a physical block on disc.
*/
status_t
MetadataPartition::MapBlock(uint32 logicalBlock, off_t &physicalBlock)
{
	off_t block = 0;
	status_t status = fMetadataIcb->FindBlock(logicalBlock, block);
	if (status != B_OK)
		return status;
	return fParentPartition.MapBlock(block, physicalBlock);
}

/*! Returns the initialization status of the object.
*/
status_t
MetadataPartition::InitCheck()
{
	return fInitStatus;
}
