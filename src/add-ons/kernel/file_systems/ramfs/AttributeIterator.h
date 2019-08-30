/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ATTRIBUTE_ITERATOR_H
#define ATTRIBUTE_ITERATOR_H

#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>


class Attribute;
class Node;

class AttributeIterator : public DoublyLinkedListLinkImpl<AttributeIterator> {
public:
	AttributeIterator(Node *node = NULL);
	~AttributeIterator();

	status_t SetTo(Node *node);
	void Unset();

	Node *GetNode() const { return fNode; }

	status_t Suspend();
	status_t Resume();
	bool IsSuspended() const { return fSuspended; }

	status_t GetNext(Attribute **attribute);
	Attribute *GetCurrent() const { return fAttribute; }

	status_t Rewind();

private:
	void SetCurrent(Attribute *attribute, bool isNext);

private:
	friend class Node;

private:
	Node							*fNode;
	Attribute						*fAttribute;
	bool							fSuspended;
	bool							fIsNext;
	bool							fDone;
};

#endif	// ATTRIBUTE_ITERATOR_H
