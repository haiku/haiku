/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "File.h"

#include "AllocationInfo.h"
#include "SizeIndex.h"
#include "Volume.h"


File::File(Volume *volume)
	: Node(volume, NODE_TYPE_FILE),
	  DataContainer(volume)
{
}


File::~File()
{
}


status_t
File::ReadAt(off_t offset, void *buffer, size_t size, size_t *bytesRead)
{
	status_t error = DataContainer::ReadAt(offset, buffer, size, bytesRead);
	// TODO: update access time?
	return error;
}


status_t
File::WriteAt(off_t offset, const void *buffer, size_t size,
	size_t *bytesWritten)
{
	off_t oldSize = DataContainer::GetSize();
	status_t error = DataContainer::WriteAt(offset, buffer, size,
		bytesWritten);
	MarkModified(B_STAT_MODIFICATION_TIME);

	// update the size index, if our size has changed
	if (oldSize != DataContainer::GetSize()) {
		MarkModified(B_STAT_SIZE);

		if (SizeIndex *index = GetVolume()->GetSizeIndex())
			index->Changed(this, oldSize);
	}
	return error;
}


status_t
File::SetSize(off_t newSize)
{
	off_t oldSize = DataContainer::GetSize();
	if (newSize == oldSize) {
		// Update times (as per POSIX) even if the size stays the same.
		MarkModified(B_STAT_MODIFICATION_TIME | B_STAT_CHANGE_TIME);
		return B_OK;
	}

	status_t status = DataContainer::Resize(newSize);
	if (status != B_OK)
		return status;

	MarkModified(B_STAT_SIZE);

	// update the size index
	if (SizeIndex *index = GetVolume()->GetSizeIndex())
		index->Changed(this, oldSize);
	return B_OK;
}


off_t
File::GetSize() const
{
	return DataContainer::GetSize();
}


void
File::GetAllocationInfo(AllocationInfo &info)
{
	info.AddFileAllocation(DataContainer::GetCommittedSize());
}
