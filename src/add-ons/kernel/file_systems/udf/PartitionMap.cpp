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

	SLList<udf_partition_descriptor>::Iterator i;
	fList.GetIterator(&i);
	if (i.GetCurrent()) {
		for (udf_partition_descriptor* partition = NULL;
		       (partition = i.GetCurrent());
		         i.GetNext())
		{
			PRINT(("  partition #%d: start: %ld, length: %ld\n", partition->partition_number(),
			      partition->start(), partition->length()));
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
		udf_partition_descriptor *temp = Find(partition->partition_number());
		if (temp) {
			*temp = *partition;
		} else {
			err = fList.Insert(*partition) ? B_OK : B_ERROR;
		}
	}
	return err;
}

/*! \brief Returns a pointer to the partition in the list with the
	given number, or NULL if no such partition exists.
*/ 
udf_partition_descriptor*
PartitionMap::Find(uint32 partitionNumber) const
{
	udf_partition_descriptor *partition;
	SLList<udf_partition_descriptor>::Iterator i;
	fList.GetIterator(&i);	
	while ((partition = i.GetCurrent())
	         && partition->partition_number() != partitionNumber)
	{
		i.GetNext();
	}
	return partition;
}

udf_partition_descriptor*
PartitionMap::operator[](uint32 partitionNumber) const
{
	return Find(partitionNumber);
}
