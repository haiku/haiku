#include "VirtualPartition.h"

#define B_NOT_IMPLEMENTED B_ERROR

using namespace Udf;

/*! \brief Creates a new VirtualPartition object.

	VirtualPartition objects require a valid VAT to be found on disc. This involves
	looking up the last recorded sector on the disc (via the "READ CD RECORDED
	CAPACITY" SCSI-MMC call (code 0x25)), which	should contain the file entry for
	the VAT. Once found, the VAT can be loaded and accessed like a normal file.
*/ 
VirtualPartition::VirtualPartition(PhysicalPartition &physicalPartition)
	: fPhysicalPartition(physicalPartition)
{
	// Find VAT
}

/*! \brief Destroys the VirtualPartition object.
*/
VirtualPartition::~VirtualPartition()
{
}

/*! \brief Maps the given logical block to a physical block on disc.

	The given logical block is indexed into the VAT. If a corresponding
	mapped block exists, that block is mapped to a physical block via the
	VirtualPartition object's physical partition.
*/
status_t
VirtualPartition::MapBlock(uint32 logicalBlock, off_t &physicalBlock)
{
	return B_NOT_IMPLEMENTED;
}

/*! Returns the initialization status of the object.
*/
status_t
VirtualPartition::InitCheck()
{
	return B_NO_INIT;
}
