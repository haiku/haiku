/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <util/DoublyLinkedList.h>

#include "AttributeIndex.h"
#include "AttributeIterator.h"
#include "DataContainer.h"
#include "String.h"

class AllocationInfo;
class Node;
class Volume;

class Attribute : public DataContainer,
	public DoublyLinkedListLinkImpl<Attribute> {
public:
	Attribute(Volume *volume, Node *node, const char *name, uint32 type = 0);
	~Attribute();

	status_t InitCheck() const;

	void SetNode(Node *node)	{ fNode = node; }
	Node *GetNode() const		{ return fNode; }

	const char *GetName() { return fName.GetString(); }

	void SetType(uint32 type);
	uint32 GetType() const		{ return fType; }

	status_t SetSize(off_t newSize);
	off_t GetSize() const		{ return DataContainer::GetSize(); }

	virtual status_t WriteAt(off_t offset, const void *buffer, size_t size,
							 size_t *bytesWritten);

	// index support
	void SetIndex(AttributeIndex *index, bool inIndex);
	AttributeIndex *GetIndex() const	{ return fIndex; }
	bool IsInIndex() const				{ return fInIndex; }
	void GetKey(uint8 *key, size_t *length);

	// iterator management
	void AttachAttributeIterator(AttributeIterator *iterator);
	void DetachAttributeIterator(AttributeIterator *iterator);
	inline DoublyLinkedList<AttributeIterator> *GetAttributeIteratorList()
		{ return &fIterators; }

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	Node						*fNode;
	String						fName;
	uint32						fType;
	AttributeIndex				*fIndex;
	bool						fInIndex;

	// iterator management
	DoublyLinkedList<AttributeIterator>	fIterators;
};

#endif	// ATTRIBUTE_H
