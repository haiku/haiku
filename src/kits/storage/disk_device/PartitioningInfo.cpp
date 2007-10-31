//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <string.h>

#include <new>

#include <PartitioningInfo.h>

#include <syscalls.h>
#include <disk_device_manager/ddm_userland_interface.h>


using namespace std;


// constructor
BPartitioningInfo::BPartitioningInfo()
	: fPartitionID(-1),
	  fSpaces(NULL),
	  fCount(0),
	  fCapacity(0)
{
}


// destructor
BPartitioningInfo::~BPartitioningInfo()
{
	Unset();
}


// SetTo
status_t
BPartitioningInfo::SetTo(off_t offset, off_t size)
{
	fPartitionID = -1;
	delete[] fSpaces;

	if (size > 0) {
		fSpaces = new(nothrow) partitionable_space_data[1];
		if (!fSpaces)
			return B_NO_MEMORY;

		fCount = 1;
		fSpaces[0].offset = offset;
		fSpaces[0].size = size;
	} else {
		fSpaces = NULL;
		fCount = 0;
	}

	fCapacity = fCount;

	return B_OK;
}


// Unset
void
BPartitioningInfo::Unset()
{
	delete[] fSpaces;
	fPartitionID = -1;
	fSpaces = NULL;
	fCount = 0;
	fCapacity = 0;
}


// ExcludeOccupiedSpace
status_t
BPartitioningInfo::ExcludeOccupiedSpace(off_t offset, off_t size)
{
	if (size <= 0)
		return B_OK;

	int32 leftIndex = -1;
	int32 rightIndex = -1;
	for (int32 i = 0; i < fCount; i++) {
		if (leftIndex == -1
			&& offset < fSpaces[i].offset + fSpaces[i].size) {
			leftIndex = i;
		}

		if (fSpaces[i].offset < offset + size)
			rightIndex = i;
	}

	// If there's no intersection with any free space, we're done.
	if (leftIndex == -1 || rightIndex == -1 || leftIndex > rightIndex)
		return B_OK;

	partitionable_space_data& leftSpace = fSpaces[leftIndex];
	partitionable_space_data& rightSpace = fSpaces[rightIndex];

	off_t rightSpaceEnd = rightSpace.offset + rightSpace.size;

	// special case: split a single space in two
	if (leftIndex == rightIndex && leftSpace.offset < offset
		&& rightSpaceEnd > offset + size) {
		// add a space after this one
		status_t error = _InsertSpaces(leftIndex + 1, 1);
		if (error != B_OK)
			return error;

		partitionable_space_data& space = fSpaces[leftIndex];
		partitionable_space_data& newSpace = fSpaces[leftIndex + 1];

		space.size = offset - space.offset;

		newSpace.offset = offset + size;
		newSpace.size = rightSpaceEnd - newSpace.offset;

		return B_OK;
	}

	// check whether the first affected space is eaten completely
	int32 deleteFirst = leftIndex;
	if (leftSpace.offset < offset) {
		leftSpace.size = offset - leftSpace.offset;
		deleteFirst++;
	}

	// check whether the last affected space is eaten completely
	int32 deleteLast = rightIndex;
	if (rightSpaceEnd > offset + size) {
		rightSpace.offset = offset + size;
		rightSpace.size = rightSpaceEnd - rightSpace.offset;
		deleteLast--;
	}

	// remove all spaces that are completely eaten
	if (deleteFirst <= deleteLast)
		_RemoveSpaces(deleteFirst, deleteLast - deleteFirst + 1);

	return B_OK;
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


// _InsertSpaces
status_t
BPartitioningInfo::_InsertSpaces(int32 index, int32 count)
{
	if (index <= 0 || index > fCount || count <= 0)
		return B_BAD_VALUE;

	// If the capacity is sufficient, we just need to copy the spaces to create
	// a gap.
	if (fCount + count <= fCapacity) {
		memmove(fSpaces + index + count, fSpaces + index, fCount - index);
		fCount += count;
		return B_OK;
	}

	// alloc new array
	int32 capacity = (fCount + count) * 2;
		// add a bit room for further resizing

	partitionable_space_data* spaces
		= new(nothrow) partitionable_space_data[capacity];
	if (!spaces)
		return B_NO_MEMORY;

	// copy items
	memcpy(spaces, fSpaces, index);
	memcpy(spaces + index + count, fSpaces + index, fCount - index);

	delete[] fSpaces;
	fSpaces = spaces;
	fCapacity = capacity;
	fCount += count;

	return B_OK;
}


// _RemoveSpaces
void
BPartitioningInfo::_RemoveSpaces(int32 index, int32 count)
{
	if (index <= 0 || count <= 0 || index + count > fCount)
		return;

	if (count < fCount) {
		int32 endIndex = index + count;
		memmove(fSpaces + index, fSpaces + endIndex, fCount - endIndex);
	}

	fCount -= count;
}
