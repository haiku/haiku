#include "MetadataPartition.h"

#define B_NOT_IMPLEMENTED B_ERROR


/*! \brief Creates a new MetadataPartition object.
*/
MetadataPartition::MetadataPartition(Partition &parentPartition,
                                     uint32 metadataFileLocation,
	                                 uint32 metadataMirrorFileLocation,
	                                 uint32 metadataBitmapFileLocation,
	                                 uint32 allocationUnitSize,
	                                 uint16 alignmentUnitSize,
	                                 bool metadataIsDuplicated)
	: fParentPartition(parentPartition)
	, fAllocationUnitSize(allocationUnitSize)
	, fAlignmentUnitSize(alignmentUnitSize)
	, fMetadataIsDuplicated(metadataIsDuplicated)
	, fInitStatus(B_NO_INIT)
{
}

/*! \brief Destroys the MetadataPartition object.
*/
MetadataPartition::~MetadataPartition()
{
}

/*! \brief Maps the given logical block to a physical block on disc.
*/
status_t
MetadataPartition::MapBlock(uint32 logicalBlock, off_t &physicalBlock)
{
	return B_NOT_IMPLEMENTED;
}

/*! Returns the initialization status of the object.
*/
status_t
MetadataPartition::InitCheck()
{
	return fInitStatus;
}
