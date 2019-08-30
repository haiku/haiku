/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <limits.h>

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "SizeIndex.h"
#include "SymLink.h"
#include "Volume.h"

// constructor
SymLink::SymLink(Volume *volume)
	: Node(volume, NODE_TYPE_SYMLINK),
	  fLinkedPath()
{
}

// destructor
SymLink::~SymLink()
{
}

// SetSize
status_t
SymLink::SetSize(off_t newSize)
{
	status_t error = (newSize >= 0 && newSize < PATH_MAX ? B_OK : B_BAD_VALUE);
	int32 oldSize = GetLinkedPathLength();
	if (error == B_OK && newSize < oldSize) {
		fLinkedPath.Truncate(newSize);
		MarkModified(B_STAT_SIZE);
		// update the size index
		if (SizeIndex *index = GetVolume()->GetSizeIndex())
			index->Changed(this, oldSize);
	}
	return error;
}

// GetSize
off_t
SymLink::GetSize() const
{
	return GetLinkedPathLength();
}

// SetLinkedPath
status_t
SymLink::SetLinkedPath(const char *path)
{
	int32 oldLen = GetLinkedPathLength();
	int32 len = strnlen(path, PATH_MAX - 1);
	if (fLinkedPath.SetTo(path, len)) {
		MarkModified(B_STAT_MODIFICATION_TIME);
		// update the size index, if necessary
		if (len != oldLen) {
			MarkModified(B_STAT_SIZE);

			if (SizeIndex *index = GetVolume()->GetSizeIndex())
				index->Changed(this, oldLen);
		}
		return B_OK;
	}
	RETURN_ERROR(B_NO_MEMORY);
}

// GetAllocationInfo
void
SymLink::GetAllocationInfo(AllocationInfo &info)
{
	info.AddSymLinkAllocation(GetSize());
}

