// SLList.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef SL_LIST_H
#define SL_LIST_H

#include <new>

// SLListStandardNode
template<typename Value>
struct SLListStandardNode {
	SLListStandardNode(const Value &a)
		: value(a),
		  next(NULL)
	{
	}

	Value						value;
	SLListStandardNode<Value>	*next;
};

// SLListStandardNodeAllocator
template<typename Value, typename Node>
class SLListStandardNodeAllocator
{
public:
	inline Node *Allocate(const Value &a) const
	{
		return new(nothrow) SLListStandardNode<Value>(a);
	}

	inline void Free(Node *node) const
	{
		delete node;
	}
};

// SLListValueNodeAllocator
template<typename Value, typename Node>
class SLListValueNodeAllocator
{
public:
	inline Node *Allocate(const Value &a) const
	{
		return a;
	}

	inline void Free(Node *node) const
	{
	}
};

// SLListStandardGetValue
template<typename Value, typename Node>
class SLListStandardGetValue
{
public:
	inline Value &operator()(Node *node) const
	{
		return node->value;
	}
};

// SLListValueNodeGetValue
template<typename Value, typename Node>
class SLListValueNodeGetValue
{
public:
	inline Value &operator()(Node *node) const
	{
		return *node;
	}
};

// for convenience
#define SL_LIST_TEMPLATE_LIST template<typename Value, typename Node, \
	typename NodeAllocator, typename GetValue>
#define SL_LIST_CLASS_NAME SLList<Value, Node, NodeAllocator, GetValue>

// SLList
template<typename Value, typename Node = SLListStandardNode<Value>,
		 typename NodeAllocator = SLListStandardNodeAllocator<Value, Node>,
		 typename GetValue = SLListStandardGetValue<Value, Node> >
class SLList {
public:
	class Iterator;

public:
	SLList();
	SLList(const NodeAllocator &nodeAllocator, const GetValue &getValue);
	~SLList();

	bool Insert(const Value &value, Iterator *iterator = NULL);
	bool Remove(const Value &value);
	void Remove(Iterator &iterator);
	void RemoveAll();

	bool Find(const Value &value, Iterator *iterator = NULL) const;
	void GetIterator(Iterator *iterator) const;

private:
	friend class Iterator;

	Node			*fHead;
	NodeAllocator	fNodeAllocator;
	GetValue		fGetValue;
};

// Iterator
SL_LIST_TEMPLATE_LIST
class SL_LIST_CLASS_NAME::Iterator {
public:
	Iterator() : fList(NULL), fCurrent(NULL)	{}
	~Iterator()	{}

	inline Value *GetCurrent()
	{
		return (fList && fCurrent ? &fList->fGetValue(fCurrent) : NULL);
	}

	inline Value *GetNext()
	{
		if (fCurrent)
			fCurrent = fCurrent->next;
		return GetCurrent();
	}

	inline void Remove()
	{
		if (fList)
			fList->Remove(*this);
	}

private:
	friend class SL_LIST_CLASS_NAME;

	inline void _SetTo(SL_LIST_CLASS_NAME *list, Node *previous, Node *node)
	{
		fList = list;
		fPrevious = previous;
		fCurrent = node;
	}

	inline SL_LIST_CLASS_NAME *_GetList() const	{ return fList; }
	inline Node *_GetPreviousNode() const		{ return fPrevious; }
	inline Node *_GetCurrentNode() const		{ return fCurrent; }

private:
	SL_LIST_CLASS_NAME	*fList;
	Node				*fPrevious;
	Node				*fCurrent;
};

// constructor
SL_LIST_TEMPLATE_LIST
SL_LIST_CLASS_NAME::SLList()
	: fHead(NULL)/*,
	  fNodeAllocator(),
	  fGetValue()*/
{
}

// constructor
SL_LIST_TEMPLATE_LIST
SL_LIST_CLASS_NAME::SLList(const NodeAllocator &nodeAllocator,
						   const GetValue &getValue)
	: fHead(NULL),
	  fNodeAllocator(nodeAllocator),
	  fGetValue(getValue)
{
}

// destructor
SL_LIST_TEMPLATE_LIST
SL_LIST_CLASS_NAME::~SLList()
{
	RemoveAll();
}

// Insert
SL_LIST_TEMPLATE_LIST
bool
SL_LIST_CLASS_NAME::Insert(const Value &value, Iterator *iterator)
{
	Node *node = fNodeAllocator.Allocate(value);
	if (node) {
		node->next = fHead;
		fHead = node;
		if (iterator)
			iterator->_SetTo(this, NULL, node);
	}
	return node;
}

// Remove
SL_LIST_TEMPLATE_LIST
bool
SL_LIST_CLASS_NAME::Remove(const Value &value)
{
	Iterator iterator;
	bool result = Find(value, &iterator);
	if (result)
		iterator.Remove();
	return result;
}

// Remove
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::Remove(Iterator &iterator)
{
	Node *node = iterator._GetCurrentNode();
	if (iterator._GetList() == this && node) {
		Node *previous = iterator._GetPreviousNode();
		iterator._SetTo(this, previous, node->next);
		if (previous)
			previous->next = node->next;
		else
			fHead = node->next;
		fNodeAllocator.Free(node);
	}
}

// RemoveAll
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::RemoveAll()
{
	for (Node *node = fHead; node; ) {
		Node *next = node->next;
		fNodeAllocator.Free(node);
		node = next;
	}
	fHead = NULL;
}

// Find
SL_LIST_TEMPLATE_LIST
bool
SL_LIST_CLASS_NAME::Find(const Value &value, Iterator *iterator) const
{
	Node *node = fHead;
	Node *previous = NULL;
	while (node && fGetValue(node) != value) {
		previous = node;
		node = node->next;
	}
	if (node && iterator) {
		iterator->_SetTo(const_cast<SL_LIST_CLASS_NAME*>(this), previous,
						 node);
	}
	return node;
}

// GetIterator
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::GetIterator(Iterator *iterator) const
{
	if (iterator)
		iterator->_SetTo(const_cast<SL_LIST_CLASS_NAME*>(this), NULL, fHead);
}

#endif	// SL_LIST_H
