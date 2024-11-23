/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AttributeIterator.h"

#include <util/AutoLock.h>

#include "Node.h"
#include "Volume.h"


AttributeIterator::AttributeIterator(Node *node)
	: fNode(node),
	  fAttribute(NULL),
	  fSuspended(false),
	  fIsNext(false),
	  fDone(false)
{
}


AttributeIterator::~AttributeIterator()
{
	Unset();
}


status_t
AttributeIterator::SetTo(Node *node)
{
	Unset();

	if (node == NULL)
		return B_BAD_VALUE;

	fNode = node;
	fAttribute = NULL;
	fSuspended = false;
	fIsNext = false;
	fDone = false;
	return B_OK;
}


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


status_t
AttributeIterator::Suspend()
{
	if (fNode == NULL)
		return B_ERROR;

	RecursiveLocker locker(fNode->GetVolume()->AttributeIteratorLocker());
	if (!locker.IsLocked())
		return B_ERROR;

	if (fSuspended)
		return B_ERROR;

	if (fAttribute)
		fAttribute->AttachAttributeIterator(this);
	fSuspended = true;

	return B_OK;
}


status_t
AttributeIterator::Resume()
{
	if (fNode == NULL)
		return B_ERROR;

	RecursiveLocker locker(fNode->GetVolume()->AttributeIteratorLocker());
	if (!locker.IsLocked())
		return B_ERROR;

	if (!fSuspended)
		return B_ERROR;

	if (fAttribute)
		fAttribute->DetachAttributeIterator(this);
	fSuspended = false;

	return B_OK;
}


status_t
AttributeIterator::GetNext(Attribute **attribute)
{
	if (attribute == NULL)
		return B_BAD_VALUE;

	status_t error = B_ENTRY_NOT_FOUND;
	if (!fDone && fNode != NULL) {
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


status_t
AttributeIterator::Rewind()
{
	if (fNode == NULL)
		return B_ERROR;

	RecursiveLocker locker(fNode->GetVolume()->AttributeIteratorLocker());
	if (!locker.IsLocked())
		return B_ERROR;

	if (fSuspended && fAttribute)
		fAttribute->DetachAttributeIterator(this);
	fAttribute = NULL;
	fIsNext = false;
	fDone = false;
	return B_OK;
}


void
AttributeIterator::SetCurrent(Attribute *attribute, bool isNext)
{
	fIsNext = isNext;
	fAttribute = attribute;
	fDone = !fAttribute;
}
