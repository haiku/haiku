/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "Node.h"
#include "Volume.h"

// constructor
Entry::Entry(const char *name, Node *node, Directory *parent)
	: fParent(parent),
	  fNode(NULL),
	  fName(name),
	  fReferrerLink(),
	  fIterators()
{
	if (node)
		Link(node);
}

// destructor
Entry::~Entry()
{
	if (fNode)
		Unlink();
}

// InitCheck
status_t
Entry::InitCheck() const
{
	return (fName.GetString() ? B_OK : B_NO_INIT);
}


status_t
Entry::Link(Node *node)
{
	if (node == NULL)
		return B_BAD_VALUE;
	if (node == fNode)
		return B_OK;

	// We can only be linked to one Node at a time, so force callers
	// to decide what to do when we're already linked to a Node.
	if (fNode != NULL)
		return B_BAD_VALUE;

	status_t status = node->Link(this);
	if (status == B_OK)
		fNode = node;
	return status;
}


status_t
Entry::Unlink()
{
	if (fNode == NULL)
		return B_BAD_VALUE;

	status_t status = fNode->Unlink(this);
	if (status == B_OK)
		fNode = NULL;
	return status;
}

// SetName
status_t
Entry::SetName(const char *newName)
{
	status_t error = (newName ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!fName.SetTo(newName))
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// AttachEntryIterator
void
Entry::AttachEntryIterator(EntryIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && !iterator->IsSuspended())
		fIterators.Insert(iterator);
}

// DetachEntryIterator
void
Entry::DetachEntryIterator(EntryIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && iterator->IsSuspended())
		fIterators.Remove(iterator);
}

// GetAllocationInfo
void
Entry::GetAllocationInfo(AllocationInfo &info)
{
	info.AddEntryAllocation();
	info.AddStringAllocation(fName.GetLength());
	fNode->GetAllocationInfo(info);
}
