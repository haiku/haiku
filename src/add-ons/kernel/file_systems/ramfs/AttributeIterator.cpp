/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AttributeIterator.h"
#include "Node.h"
#include "Volume.h"

// constructor
AttributeIterator::AttributeIterator(Node *node)
	: fNode(node),
	  fAttribute(NULL),
	  fSuspended(false),
	  fIsNext(false),
	  fDone(false)
{
}

// destructor
AttributeIterator::~AttributeIterator()
{
	Unset();
}

// SetTo
status_t
AttributeIterator::SetTo(Node *node)
{
	Unset();
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fNode = node;
		fAttribute = NULL;
		fSuspended = false;
		fIsNext = false;
		fDone = false;
	}
	return error;
}

// Unset
void
AttributeIterator::Unset()
{
	if (fNode && fSuspended)
		Resume();
	fNode = NULL;
	fAttribute = NULL;
	fSuspended = false;
	fIsNext = false;
	fDone = false;
}

// Suspend
status_t
AttributeIterator::Suspend()
{
	status_t error = (fNode ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fNode->GetVolume()->IteratorLock()) {
			if (!fSuspended) {
				if (fAttribute)
					fAttribute->AttachAttributeIterator(this);
				fNode->GetVolume()->IteratorUnlock();
				fSuspended = true;
			} else
				error = B_ERROR;
		} else
			error = B_ERROR;
	}
	return error;
}

// Resume
status_t
AttributeIterator::Resume()
{
	status_t error = (fNode ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fNode->GetVolume()->IteratorLock()) {
			if (fSuspended) {
				if (fAttribute)
					fAttribute->DetachAttributeIterator(this);
				fSuspended = false;
			}
			fNode->GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
	}
	return error;
}

// GetNext
status_t
AttributeIterator::GetNext(Attribute **attribute)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (!fDone && fNode && attribute) {
		if (fIsNext) {
			fIsNext = false;
			if (fAttribute)
				error = B_OK;
		} else
			error = fNode->GetNextAttribute(&fAttribute);
		*attribute = fAttribute;
	}
	fDone = (error != B_OK);
	return error;
}

// Rewind
status_t
AttributeIterator::Rewind()
{
	status_t error = (fNode ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fNode->GetVolume()->IteratorLock()) {
			if (fSuspended && fAttribute)
				fAttribute->DetachAttributeIterator(this);
			fAttribute = NULL;
			fIsNext = false;
			fDone = false;
			fNode->GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
	}
	return error;
}

// SetCurrent
void
AttributeIterator::SetCurrent(Attribute *attribute, bool isNext)
{
	fIsNext = isNext;
	fAttribute = attribute;
	fDone = !fAttribute;
}

