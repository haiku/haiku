/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "File.h"
#include "SizeIndex.h"
#include "Volume.h"

// constructor
File::File(Volume *volume)
	: Node(volume, NODE_TYPE_FILE),
	  DataContainer(volume)
{
}

// destructor
File::~File()
{
}

// ReadAt
status_t
File::ReadAt(off_t offset, void *buffer, size_t size, size_t *bytesRead)
{
	status_t error = DataContainer::ReadAt(offset, buffer, size, bytesRead);
	// TODO: update access time?
	return error;
}

// WriteAt
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

// SetSize
status_t
File::SetSize(off_t newSize)
{
	status_t error = B_OK;
	off_t oldSize = DataContainer::GetSize();
	if (newSize != oldSize) {
		error = DataContainer::Resize(newSize);
		MarkModified(B_STAT_SIZE);
		// update the size index
		if (SizeIndex *index = GetVolume()->GetSizeIndex())
			index->Changed(this, oldSize);
	}
	return error;
}

// GetSize
off_t
File::GetSize() const
{
	return DataContainer::GetSize();
}

// GetAllocationInfo
void
File::GetAllocationInfo(AllocationInfo &info)
{
	info.AddFileAllocation(GetSize());
}

