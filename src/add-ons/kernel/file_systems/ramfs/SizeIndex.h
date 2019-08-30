/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SIZE_INDEX_H
#define SIZE_INDEX_H

#include "Index.h"
#include "NodeListener.h"
#include "TwoKeyAVLTree.h"

// SizeIndex
class SizeIndex : public Index, private NodeListener {
public:
	SizeIndex(Volume *volume);
	virtual ~SizeIndex();

	virtual int32 CountEntries() const;

	virtual status_t Changed(Node *node, off_t oldSize);

private:
	virtual void NodeAdded(Node *node);
	virtual void NodeRemoved(Node *node);

protected:
	virtual AbstractIndexEntryIterator *InternalGetIterator();
	virtual AbstractIndexEntryIterator *InternalFind(const uint8 *key,
													 size_t length);

private:
	class Iterator;
	class IteratorList;
	class NodeTree;
	friend class Iterator;

private:
	void _AddIterator(Iterator *iterator);
	void _RemoveIterator(Iterator *iterator);

private:
	NodeTree		*fNodes;
	IteratorList	*fIterators;
};

#endif	// SIZE_INDEX_H
