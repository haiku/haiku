#include "PhysicalPartition.h"

#define B_NOT_IMPLEMENTED B_ERROR


/*! \brief Creates a new PhysicalPartition object.
*/
PhysicalPartition::PhysicalPartition(uint16 number, uint32 start, uint32 length)
	:
	fNumber(number),
	fStart(start),
	fLength(length)
{
	TRACE(("PhysicalPartition::PhysicalPartition: number = %d, start = %lu,"
		"length = %lu\n", number, start, length));
}


/*! \brief Destroys the PhysicalPartition object.
*/
PhysicalPartition::~PhysicalPartition()
{
}


/*! \brief Maps the given logical block to a physical block on disc.

	The given logical block is simply treated as an offset from the
	start of the physical partition.
*/
status_t
PhysicalPartition::MapBlock(uint32 logicalBlock, off_t &physicalBlock)
{
	TRACE(("PhysicalPartition::MapBlock: %ld\n", logicalBlock));
	if (logicalBlock >= fLength) {
		TRACE_ERROR(("PhysicalPartition::MapBlock: block %ld invalid,"
			"length = %ld\n", logicalBlock, fLength));
		return B_BAD_ADDRESS;
	} else {
		physicalBlock = fStart + logicalBlock;
		TRACE(("PhysicalPartition::MapBlock: block %ld mapped to %Ld\n",
			logicalBlock, physicalBlock));
		return B_OK;
	}
}
