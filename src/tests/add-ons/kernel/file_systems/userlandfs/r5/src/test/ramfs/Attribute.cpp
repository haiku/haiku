// Attribute.cpp

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

// WriteAt
status_t
Attribute::WriteAt(off_t offset, const void *buffer, size_t size,
				   size_t *bytesWritten)
{
	status_t error = B_OK;
	if (offset < kMaxIndexKeyLength && size > 0 && fIndex) {
		// There is an index and a change will be made within the key.
		uint8 oldKey[kMaxIndexKeyLength];
		size_t oldLength;
		GetKey(oldKey, &oldLength);
		error = DataContainer::WriteAt(offset, buffer, size, bytesWritten);
		fIndex->Changed(this, oldKey, oldLength);
	} else
		error = DataContainer::WriteAt(offset, buffer, size, bytesWritten);
	// node has been changed
	if (fNode)
		fNode->MarkModified();
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
Attribute::GetKey(const uint8 **key, size_t *length)
{
	if (key && length) {
		GetFirstDataBlock(key, length);
		*length = min(*length, kMaxIndexKeyLength);
	}
}

// GetKey
void
Attribute::GetKey(uint8 *key, size_t *length)
{
	if (key && length) {
		const uint8 *originalKey = NULL;
		GetKey(&originalKey, length);
		if (length > 0)
			memcpy(key, originalKey, *length);
	}
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

