#include "PartitionMap.h"

using namespace Udf;

/*! \brief Initializes the empty partition map.
*/
PartitionMap::PartitionMap()
	: fCount(0)
{
}

/*! Debugging dump function.
*/
void
PartitionMap::dump()
{
	DUMP_INIT(CF_DUMP, "PartitionMap");
	PRINT(("fCount: %ld\n", fCount));
	PRINT(("fList:\n"));

	SinglyLinkedList<Udf::udf_partition_descriptor>::Iterator i = fList.Begin();
	if (i != fList.End()) {
		for ( ; i != fList.End(); ++i) {
			PRINT(("  partition #%d: start: %ld, length: %ld\n", i->partition_number(),
			      i->start(), i->length()));
		}	
	} else {
		PRINT(("  (empty)\n"));
	}	
}


/*! \brief Adds a copy of the given partition to the mapping,
	replacing any existing partition with the same partition
	number.
*/
status_t
PartitionMap::Add(const udf_partition_descriptor* partition)
{
	status_t err = partition ? B_OK : B_BAD_VALUE;
	if (!err) {
		udf_partition_descriptor *temp = const_cast<udf_partition_descriptor*>(Find(partition->partition_number()));
		if (temp) {
			*temp = *partition;
		} else {
			err = fList.PushBack(*partition) ? B_OK : B_ERROR;
		}
	}
	return err;
}

/*! \brief Returns a pointer to the partition in the list with the
	given number, or NULL if no such partition exists.
*/ 
const udf_partition_descriptor*
PartitionMap::Find(uint32 partitionNumber) const
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_HELPER, "PartitionMap", ("partitionNumber: %ld", partitionNumber));
	SinglyLinkedList<Udf::udf_partition_descriptor>::ConstIterator i;
	for (i = fList.Begin(); i != fList.End(); ++i) {
		if (i->partition_number() == partitionNumber) {
			PRINT(("found partition #%ld\n", partitionNumber));
			DUMP(*i);
			return &(*i);
		}
	}
	return NULL;
}

const udf_partition_descriptor*
PartitionMap::operator[](uint32 partitionNumber) const
{
	return Find(partitionNumber);
}
