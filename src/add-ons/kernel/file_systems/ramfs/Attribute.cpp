/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "Attribute.h"
#include "Misc.h"
#include "Node.h"
#include "ramfs.h"
#include "Volume.h"

// constructor
Attribute::Attribute(Volume *volume, Node *node, const char *name,
					 uint32 type)
	: DataContainer(volume),
	  fNode(node),
	  fName(name),
	  fType(type),
	  fIndex(NULL),
	  fInIndex(false),
	  fIterators()
{
}

// destructor
Attribute::~Attribute()
{
}

// InitCheck
status_t
Attribute::InitCheck() const
{
	return (fName.GetString() ? B_OK : B_NO_INIT);
}

// SetType
void
Attribute::SetType(uint32 type)
{
	if (type != fType) {
		if (fIndex)
			fIndex->Removed(this);
		fType = type;
		if (AttributeIndex *index = GetVolume()->FindAttributeIndex(GetName(),
																	fType)) {
			index->Added(this);
		}
	}
}

// SetSize
status_t
Attribute::SetSize(off_t newSize)
{
	status_t error = B_OK;
	off_t oldSize = DataContainer::GetSize();
	if (newSize != oldSize) {
		if (fNode)
			fNode->MarkModified(B_STAT_MODIFICATION_TIME);

		error = DataContainer::Resize(newSize);
	}
	return error;
}

// WriteAt
status_t
Attribute::WriteAt(off_t offset, const void *buffer, size_t size,
				   size_t *bytesWritten)
{
	// get the current key for the attribute
	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength;
	GetKey(oldKey, &oldLength);

	// write the new value
	status_t error = DataContainer::WriteAt(offset, buffer, size, bytesWritten);

	// If there is an index and a change has been made within the key, notify
	// the index.
	if (offset < kMaxIndexKeyLength && size > 0 && fIndex)
		fIndex->Changed(this, oldKey, oldLength);

	// update live queries
	uint8 newKey[kMaxIndexKeyLength];
	size_t newLength;
	GetKey(newKey, &newLength);
	GetVolume()->UpdateLiveQueries(NULL, fNode, GetName(), fType, oldKey,
		oldLength, newKey, newLength);

	// node has been changed
	if (fNode && size > 0)
		fNode->MarkModified(B_STAT_MODIFICATION_TIME);

	return error;
}

// SetIndex
void
Attribute::SetIndex(AttributeIndex *index, bool inIndex)
{
	fIndex = index;
	fInIndex = inIndex;
}

// GetKey
void
Attribute::GetKey(uint8 *key, size_t *length)
{
	*length = min(*length, kMaxIndexKeyLength);
	ReadAt(0, key, *length, length);
}

// AttachAttributeIterator
void
Attribute::AttachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && !iterator->IsSuspended())
		fIterators.Insert(iterator);
}

// DetachAttributeIterator
void
Attribute::DetachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && iterator->IsSuspended())
		fIterators.Remove(iterator);
}

// GetAllocationInfo
void
Attribute::GetAllocationInfo(AllocationInfo &info)
{
	DataContainer::GetAllocationInfo(info);
	info.AddAttributeAllocation(GetSize());
	info.AddStringAllocation(fName.GetLength());
}

