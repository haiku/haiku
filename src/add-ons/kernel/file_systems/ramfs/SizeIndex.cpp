/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TypeConstants.h>

#include "Entry.h"
#include "EntryListener.h"
#include "IndexImpl.h"
#include "Node.h"
#include "NodeListener.h"
#include "SizeIndex.h"
#include "Volume.h"

// SizeIndexPrimaryKey
class SizeIndexPrimaryKey {
public:
	SizeIndexPrimaryKey(Node *node, off_t size)
		: node(node), size(size) {}
	SizeIndexPrimaryKey(Node *node)
		: node(node), size(node->GetSize()) {}
	SizeIndexPrimaryKey(off_t size)
		: node(NULL), size(size) {}

	Node	*node;
	off_t	size;
};

// SizeIndexGetPrimaryKey
class SizeIndexGetPrimaryKey {
public:
	inline SizeIndexPrimaryKey operator()(Node *a)
	{
		return SizeIndexPrimaryKey(a);
	}

	inline SizeIndexPrimaryKey operator()(Node *a) const
	{
		return SizeIndexPrimaryKey(a);
	}
};

// SizeIndexPrimaryKeyCompare
class SizeIndexPrimaryKeyCompare
{
public:
	inline int operator()(const SizeIndexPrimaryKey &a,
						  const SizeIndexPrimaryKey &b) const
	{
		if (a.node != NULL && a.node == b.node)
			return 0;
		if (a.size < b.size)
			return -1;
		if (a.size > b.size)
			return 1;
		return 0;
	}
};


// NodeTree
typedef TwoKeyAVLTree<Node*, SizeIndexPrimaryKey,
					  SizeIndexPrimaryKeyCompare,
					  SizeIndexGetPrimaryKey>
		_NodeTree;
class SizeIndex::NodeTree : public _NodeTree {};


// IteratorList
class SizeIndex::IteratorList : public DoublyLinkedList<Iterator> {};


// Iterator
class SizeIndex::Iterator
	: public NodeEntryIterator<SizeIndex::NodeTree::Iterator>,
	  public DoublyLinkedListLinkImpl<Iterator>, public EntryListener,
	  public NodeListener {
public:
	Iterator();
	virtual ~Iterator();

	virtual Entry *GetCurrent();
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength);

	virtual status_t Suspend();
	virtual status_t Resume();

	bool SetTo(SizeIndex *index, off_t size, bool ignoreValue = false);
	void Unset();

	virtual void EntryRemoved(Entry *entry);
	virtual void NodeRemoved(Node *node);

private:
	typedef NodeEntryIterator<SizeIndex::NodeTree::Iterator> BaseClass;

private:
	SizeIndex						*fIndex;
};


// SizeIndex

// constructor
SizeIndex::SizeIndex(Volume *volume)
	: Index(volume, "size", B_INT64_TYPE, true, sizeof(off_t)),
	  fNodes(new(nothrow) NodeTree),
	  fIterators(new(nothrow) IteratorList)
{
	if (fInitStatus == B_OK && (!fNodes || !fIterators))
		fInitStatus = B_NO_MEMORY;
	if (fInitStatus == B_OK) {
		fInitStatus = fVolume->AddNodeListener(this,
			NULL, NODE_LISTEN_ANY_NODE | NODE_LISTEN_ALL);
	}
}

// destructor
SizeIndex::~SizeIndex()
{
	if (fVolume)
		fVolume->RemoveNodeListener(this, NULL);
	if (fIterators) {
		// unset the iterators
		for (Iterator *iterator = fIterators->First();
			 iterator;
			 iterator = fIterators->GetNext(iterator)) {
			iterator->SetTo(NULL, 0);
		}
		delete fIterators;
	}
	if (fNodes)
		delete fNodes;
}

// CountEntries
int32
SizeIndex::CountEntries() const
{
	return fNodes->CountItems();
}

// Changed
status_t
SizeIndex::Changed(Node *node, off_t oldSize)
{
	status_t error = B_BAD_VALUE;
	if (node) {
		NodeTree::Iterator it;
		Node **foundNode = fNodes->Find(SizeIndexPrimaryKey(node, oldSize),
										node, &it);
		if (foundNode && *foundNode == node) {
			// update the iterators
			for (Iterator *iterator = fIterators->First();
				 iterator;
				 iterator = fIterators->GetNext(iterator)) {
				if (iterator->GetCurrentNode() == node)
					iterator->NodeRemoved(node);
			}

			// remove and re-insert the node
			it.Remove();
			error = fNodes->Insert(node);

			// udpate live queries
			off_t newSize = node->GetSize();
			fVolume->UpdateLiveQueries(NULL, node, GetName(), GetType(),
				(const uint8*)&oldSize, sizeof(oldSize), (const uint8*)&newSize,
				sizeof(newSize));
		}
	}
	return error;
}

// NodeAdded
void
SizeIndex::NodeAdded(Node *node)
{
	if (node)
		fNodes->Insert(node);
}

// NodeRemoved
void
SizeIndex::NodeRemoved(Node *node)
{
	if (node)
		fNodes->Remove(node, node);
}

// InternalGetIterator
AbstractIndexEntryIterator *
SizeIndex::InternalGetIterator()
{
	Iterator *iterator = new(nothrow) Iterator;
	if (iterator) {
		if (!iterator->SetTo(this, 0, true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// InternalFind
AbstractIndexEntryIterator *
SizeIndex::InternalFind(const uint8 *key, size_t length)
{
	if (!key || length != sizeof(off_t))
		return NULL;
	Iterator *iterator = new(nothrow) Iterator;
	if (iterator) {
		if (!iterator->SetTo(this, *(const off_t*)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// _AddIterator
void
SizeIndex::_AddIterator(Iterator *iterator)
{
	fIterators->Insert(iterator);
}

// _RemoveIterator
void
SizeIndex::_RemoveIterator(Iterator *iterator)
{
	fIterators->Remove(iterator);
}


// Iterator

// constructor
SizeIndex::Iterator::Iterator()
	: BaseClass(),
	  fIndex(NULL)
{
}

// destructor
SizeIndex::Iterator::~Iterator()
{
	SetTo(NULL, 0);
}

// GetCurrent
Entry *
SizeIndex::Iterator::GetCurrent()
{
	return BaseClass::GetCurrent();
}

// GetCurrent
Entry *
SizeIndex::Iterator::GetCurrent(uint8 *buffer, size_t *keyLength)
{
	Entry *entry = GetCurrent();
	if (entry) {
		*(off_t*)buffer = entry->GetNode()->GetSize();
		*keyLength = sizeof(size_t);
	}
	return entry;
}

// Suspend
status_t
SizeIndex::Iterator::Suspend()
{
	status_t error = BaseClass::Suspend();
	if (error == B_OK) {
		if (fNode) {
			error = fIndex->GetVolume()->AddNodeListener(this, fNode,
														 NODE_LISTEN_REMOVED);
			if (error == B_OK && fEntry) {
				error = fIndex->GetVolume()->AddEntryListener(this, fEntry,
					ENTRY_LISTEN_REMOVED);
				if (error != B_OK)
					fIndex->GetVolume()->RemoveNodeListener(this, fNode);
			}
			if (error != B_OK)
				BaseClass::Resume();
		}
	}
	return error;
}

// Resume
status_t
SizeIndex::Iterator::Resume()
{
	status_t error = BaseClass::Resume();
	if (error == B_OK) {
		if (fEntry)
			error = fIndex->GetVolume()->RemoveEntryListener(this, fEntry);
		if (fNode) {
			if (error == B_OK)
				error = fIndex->GetVolume()->RemoveNodeListener(this, fNode);
			else
				fIndex->GetVolume()->RemoveNodeListener(this, fNode);
		}
	}
	return error;
}

// SetTo
bool
SizeIndex::Iterator::SetTo(SizeIndex *index, off_t size, bool ignoreValue)
{
	Resume();
	Unset();
	// set the new values
	fIndex = index;
	if (fIndex)
		fIndex->_AddIterator(this);
	fInitialized = fIndex;
	// get the node's first entry
	if (fIndex) {
		// get the first node
		bool found = true;
		if (ignoreValue)
			fIndex->fNodes->GetIterator(&fIterator);
		else
			found = fIndex->fNodes->FindFirst(size, &fIterator);
		// get the first entry
		if (found) {
			if (Node **nodeP = fIterator.GetCurrent()) {
				fNode = *nodeP;
				fEntry = fNode->GetFirstReferrer();
				if (!fEntry)
					BaseClass::GetNext();
				if (!ignoreValue && fNode && fNode->GetSize() != size)
					Unset();
			}
		}
	}
	return fEntry;
}

// Unset
void
SizeIndex::Iterator::Unset()
{
	if (fIndex) {
		fIndex->_RemoveIterator(this);
		fIndex = NULL;
	}
	BaseClass::Unset();
}

// EntryRemoved
void
SizeIndex::Iterator::EntryRemoved(Entry */*entry*/)
{
	Resume();
	fIsNext = BaseClass::GetNext();
	Suspend();
}

// NodeRemoved
void
SizeIndex::Iterator::NodeRemoved(Node */*node*/)
{
	Resume();
	fEntry = NULL;
	fIsNext = BaseClass::GetNext();
	Suspend();
}

