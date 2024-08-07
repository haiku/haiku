/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "EntryIterator.h"
#include "LastModifiedIndex.h"
#include "Node.h"
#include "Volume.h"


Node::Node(Volume *volume, uint8 type)
	: fVolume(volume),
	  fID(fVolume->NextNodeID()),
	  fRefCount(0),
	  fMode(0),
	  fUID(0),
	  fGID(0),
	  fATime(0),
	  fMTime(0),
	  fCTime(0),
	  fCrTime(0),
	  fModified(0),
	  fIsKnownToVFS(false),
	  fAttributes(),
	  fReferrers()
{
	// set file type
	switch (type) {
		case NODE_TYPE_DIRECTORY:
			fMode = S_IFDIR;
			break;
		case NODE_TYPE_FILE:
			fMode = S_IFREG;
			break;
		case NODE_TYPE_SYMLINK:
			fMode = S_IFLNK;
			break;
	}
	// set defaults for time
	fATime = fMTime = fCTime = fCrTime = time(NULL);
}


Node::~Node()
{
	ASSERT(fRefCount == 0);

	// delete all attributes
	while (Attribute *attribute = fAttributes.First()) {
		status_t error = DeleteAttribute(attribute);
		if (error != B_OK) {
			FATAL("Node::~Node(): Failed to delete attribute!\n");
			break;
		}
	}
}


status_t
Node::InitCheck() const
{
	return (fVolume && fID >= 0 ? B_OK : B_NO_INIT);
}


status_t
Node::AddReference()
{
	if (++fRefCount == 1) {
		status_t error = GetVolume()->PublishVNode(this);
		if (error != B_OK) {
			fRefCount--;
			return error;
		}

		fIsKnownToVFS = true;
	}

	return B_OK;
}


void
Node::RemoveReference()
{
	ASSERT(fRefCount > 0);
	if (--fRefCount == 0) {
		GetVolume()->RemoveVNode(this);
			// RemoveVNode can potentially delete us immediately!
	}
}


status_t
Node::Link(Entry *entry)
{
PRINT("Node[%" B_PRIdINO "]::Link(): %" B_PRId32 " ->...\n", fID, fRefCount);
	fReferrers.Insert(entry);

	status_t error = AddReference();
	if (error != B_OK)
		fReferrers.Remove(entry);

	return error;
}


status_t
Node::Unlink(Entry *entry)
{
PRINT("Node[%" B_PRIdINO "]::Unlink(): %" B_PRId32 " ->...\n", fID, fRefCount);
	fReferrers.Remove(entry);

	RemoveReference();
	return B_OK;
}


void
Node::SetMTime(time_t mTime)
{
	time_t oldMTime = fMTime;
	fATime = fMTime = mTime;
	if (oldMTime != fMTime) {
		if (LastModifiedIndex *index = fVolume->GetLastModifiedIndex())
			index->Changed(this, oldMTime);
	}
}


status_t
Node::CheckPermissions(int mode) const
{
	return check_access_permissions(mode, fMode, fGID, fUID);
}


status_t
Node::CreateAttribute(const char *name, Attribute **_attribute)
{
	status_t error = (name && _attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create attribute
		Attribute *attribute = new(nothrow) Attribute(fVolume, NULL, name);
		if (attribute) {
			error = attribute->InitCheck();
			if (error == B_OK) {
				// add attribute to node
				error = AddAttribute(attribute);
				if (error == B_OK)
					*_attribute = attribute;
			}
			if (error != B_OK)
				delete attribute;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}


status_t
Node::DeleteAttribute(Attribute *attribute)
{
	status_t error = RemoveAttribute(attribute);
	if (error == B_OK)
		delete attribute;
	return error;
}


status_t
Node::AddAttribute(Attribute *attribute)
{
	status_t error = (attribute && !attribute->GetNode() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = GetVolume()->NodeAttributeAdded(GetID(), attribute);
		if (error == B_OK) {
			fAttributes.Insert(attribute);
			attribute->SetNode(this);
			MarkModified(B_STAT_MODIFICATION_TIME);
		}
	}
	return error;
}


status_t
Node::RemoveAttribute(Attribute *attribute)
{
	status_t error = (attribute && attribute->GetNode() == this
					  ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// move all iterators pointing to the attribute to the next attribute
		if (GetVolume()->IteratorLock()) {
			// set the iterators' current entry
			Attribute *nextAttr = fAttributes.GetNext(attribute);
			DoublyLinkedList<AttributeIterator> *iterators
				= attribute->GetAttributeIteratorList();
			for (AttributeIterator *iterator = iterators->First();
				 iterator;
				 iterator = iterators->GetNext(iterator)) {
				iterator->SetCurrent(nextAttr, true);
			}
			// Move the iterators from one list to the other, or just remove
			// them, if there is no next attribute.
			if (nextAttr) {
				DoublyLinkedList<AttributeIterator> *nextIterators
					= nextAttr->GetAttributeIteratorList();
				nextIterators->MoveFrom(iterators);
			} else
				iterators->RemoveAll();
			GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
		// remove the attribute
		if (error == B_OK) {
			error = GetVolume()->NodeAttributeRemoved(GetID(), attribute);
			if (error == B_OK) {
				fAttributes.Remove(attribute);
				attribute->SetNode(NULL);
				MarkModified(B_STAT_MODIFICATION_TIME);
			}
		}
	}
	return error;
}


status_t
Node::FindAttribute(const char *name, Attribute **_attribute) const
{
	status_t error = (name && _attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		Attribute *attribute = NULL;
		while (GetNextAttribute(&attribute) == B_OK) {
			if (!strcmp(attribute->GetName(), name)) {
				*_attribute = attribute;
				return B_OK;
			}
		}
		error = B_ENTRY_NOT_FOUND;
	}
	return error;
}


status_t
Node::GetPreviousAttribute(Attribute **attribute) const
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*attribute)
			*attribute = fAttributes.Last();
		else if ((*attribute)->GetNode() == this)
			*attribute = fAttributes.GetPrevious(*attribute);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*attribute)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}


status_t
Node::GetNextAttribute(Attribute **attribute) const
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*attribute)
			*attribute = fAttributes.First();
		else if ((*attribute)->GetNode() == this)
			*attribute = fAttributes.GetNext(*attribute);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*attribute)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}


Entry *
Node::GetFirstReferrer() const
{
	return fReferrers.First();
}


Entry *
Node::GetLastReferrer() const
{
	return fReferrers.Last();
}


Entry *
Node::GetPreviousReferrer(Entry *entry) const
{
	return (entry ? fReferrers.GetPrevious(entry) : NULL );
}


Entry *
Node::GetNextReferrer(Entry *entry) const
{
	return (entry ? fReferrers.GetNext(entry) : NULL );
}


void
Node::GetAllocationInfo(AllocationInfo &info)
{
	Attribute *attribute = NULL;
	while (GetNextAttribute(&attribute) == B_OK)
		attribute->GetAllocationInfo(info);
}
