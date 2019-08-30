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
	  fNode(node),
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

// Link
status_t
Entry::Link(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// We first link to the new node and then unlink the old one. So, no
		// harm is done, if both are the same.
		Node *oldNode = fNode;
		error = node->Link(this);
		if (error == B_OK) {
			fNode = node;
			if (oldNode)
				oldNode->Unlink(this);
		}
	}
	return error;
}

// Unlink
status_t
Entry::Unlink()
{
	status_t error = (fNode ? B_OK : B_BAD_VALUE);
	if (error == B_OK && (error = fNode->Unlink(this)) == B_OK)
		fNode = NULL;
	return error;
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

