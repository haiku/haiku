//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <new>

#include <PartitioningInfo.h>

#include <syscalls.h>
#include <disk_device_manager/ddm_userland_interface.h>

using namespace std;

// constructor
BPartitioningInfo::BPartitioningInfo()
	: fPartitionID(-1),
	  fSpaces(NULL),
	  fCount(0)
{
}

// destructor
BPartitioningInfo::~BPartitioningInfo()
{
	Unset();
}

// Unset
void
BPartitioningInfo::Unset()
{
	delete[] fSpaces;
	fPartitionID = -1;
	fSpaces = NULL;
	fCount = 0;
}

// PartitionID
partition_id
BPartitioningInfo::PartitionID() const
{
	return fPartitionID;
}

// GetPartitionableSpaceAt
status_t
BPartitioningInfo::GetPartitionableSpaceAt(int32 index, off_t *offset,
										   off_t *size) const
{
	if (!fSpaces || !offset || !size || index < 0 || index >= fCount)
		return B_BAD_VALUE;
	*offset = fSpaces[index].offset;
	*size = fSpaces[index].size;
	return B_OK;
}

// CountPartitionableSpaces
int32
BPartitioningInfo::CountPartitionableSpaces() const
{
	return fCount;
}

// _SetTo
status_t
BPartitioningInfo::_SetTo(partition_id partition, int32 changeCounter)
{
	Unset();

	status_t error = B_OK;
	partitionable_space_data* buffer = NULL;
	int32 count = 0;
	int32 actualCount = 0;
	while (true) {
		error = _kern_get_partitionable_spaces(partition, changeCounter,
			buffer, count, &actualCount);
		if (error != B_BUFFER_OVERFLOW)
			break;

		// buffer too small re-allocate it
		delete[] buffer;
		buffer = new(nothrow) partitionable_space_data[actualCount];
		if (!buffer) {
			error = B_NO_MEMORY;
			break;
		}

		count = actualCount;
	}

	// set data / cleanup on failure
	if (error == B_OK) {
		fPartitionID = partition;
		fSpaces = buffer;
		fCount = actualCount;
	} else if (buffer)
		delete[] buffer;

	return error;
}

