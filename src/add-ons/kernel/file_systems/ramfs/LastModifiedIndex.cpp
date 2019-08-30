/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TypeConstants.h>

#include "DebugSupport.h"
#include "Entry.h"
#include "EntryListener.h"
#include "IndexImpl.h"
#include "LastModifiedIndex.h"
#include "Node.h"
#include "NodeListener.h"
#include "Volume.h"

// LastModifiedIndexPrimaryKey
class LastModifiedIndexPrimaryKey {
public:
	LastModifiedIndexPrimaryKey(Node *node, time_t modified)
		: node(node), modified(modified) {}
	LastModifiedIndexPrimaryKey(Node *node)
		: node(node), modified(node->GetMTime()) {}
	LastModifiedIndexPrimaryKey(time_t modified)
		: node(NULL), modified(modified) {}

	Node	*node;
	time_t	modified;
};

// LastModifiedIndexGetPrimaryKey
class LastModifiedIndexGetPrimaryKey {
public:
	inline LastModifiedIndexPrimaryKey operator()(Node *a)
	{
		return LastModifiedIndexPrimaryKey(a);
	}

	inline LastModifiedIndexPrimaryKey operator()(Node *a) const
	{
		return LastModifiedIndexPrimaryKey(a);
	}
};

// LastModifiedIndexPrimaryKeyCompare
class LastModifiedIndexPrimaryKeyCompare
{
public:
	inline int operator()(const LastModifiedIndexPrimaryKey &a,
						  const LastModifiedIndexPrimaryKey &b) const
	{
		if (a.node != NULL && a.node == b.node)
			return 0;
		if (a.modified < b.modified)
			return -1;
		if (a.modified > b.modified)
			return 1;
		return 0;
	}
};


// NodeTree
typedef TwoKeyAVLTree<Node*, LastModifiedIndexPrimaryKey,
					  LastModifiedIndexPrimaryKeyCompare,
					  LastModifiedIndexGetPrimaryKey>
		_NodeTree;
class LastModifiedIndex::NodeTree : public _NodeTree {};


// IteratorList
class LastModifiedIndex::IteratorList : public DoublyLinkedList<Iterator> {};


// Iterator
class LastModifiedIndex::Iterator
	: public NodeEntryIterator<LastModifiedIndex::NodeTree::Iterator>,
	  public DoublyLinkedListLinkImpl<Iterator>, public EntryListener,
	  public NodeListener {
public:
	Iterator();
	virtual ~Iterator();

	virtual Entry *GetCurrent();
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength);

	virtual status_t Suspend();
	virtual status_t Resume();

	bool SetTo(LastModifiedIndex *index, time_t modified,
			   bool ignoreValue = false);
	void Unset();

	virtual void EntryRemoved(Entry *entry);
	virtual void NodeRemoved(Node *node);

private:
	typedef NodeEntryIterator<LastModifiedIndex::NodeTree::Iterator> BaseClass;

private:
	LastModifiedIndex						*fIndex;
};


// LastModifiedIndex

// constructor
LastModifiedIndex::LastModifiedIndex(Volume *volume)
	: Index(volume, "last_modified", B_INT32_TYPE, true, sizeof(time_t)),
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
LastModifiedIndex::~LastModifiedIndex()
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
LastModifiedIndex::CountEntries() const
{
	return fNodes->CountItems();
}

// Changed
status_t
LastModifiedIndex::Changed(Node *node, time_t oldModified)
{
	status_t error = B_BAD_VALUE;
	if (node) {
		NodeTree::Iterator it;
		Node **foundNode = fNodes->Find(LastModifiedIndexPrimaryKey(node,
			oldModified), node, &it);
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
			time_t newModified = node->GetMTime();
			fVolume->UpdateLiveQueries(NULL, node, GetName(), GetType(),
				(const uint8*)&oldModified, sizeof(oldModified),
				(const uint8*)&newModified, sizeof(newModified));
		}
	}
	return error;
}

// NodeAdded
void
LastModifiedIndex::NodeAdded(Node *node)
{
	if (node)
		fNodes->Insert(node);
}

// NodeRemoved
void
LastModifiedIndex::NodeRemoved(Node *node)
{
	if (node)
		fNodes->Remove(node, node);
}

// InternalGetIterator
AbstractIndexEntryIterator *
LastModifiedIndex::InternalGetIterator()
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
LastModifiedIndex::InternalFind(const uint8 *key, size_t length)
{
	if (!key || length != sizeof(time_t))
		return NULL;
	Iterator *iterator = new(nothrow) Iterator;
	if (iterator) {
		if (!iterator->SetTo(this, *(const time_t*)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// _AddIterator
void
LastModifiedIndex::_AddIterator(Iterator *iterator)
{
	fIterators->Insert(iterator);
}

// _RemoveIterator
void
LastModifiedIndex::_RemoveIterator(Iterator *iterator)
{
	fIterators->Remove(iterator);
}


// Iterator

// constructor
LastModifiedIndex::Iterator::Iterator()
	: BaseClass(),
	  fIndex(NULL)
{
}

// destructor
LastModifiedIndex::Iterator::~Iterator()
{
	SetTo(NULL, 0);
}

// GetCurrent
Entry *
LastModifiedIndex::Iterator::GetCurrent()
{
	return BaseClass::GetCurrent();
}

// GetCurrent
Entry *
LastModifiedIndex::Iterator::GetCurrent(uint8 *buffer, size_t *keyLength)
{
	Entry *entry = GetCurrent();
	if (entry) {
		*(time_t*)buffer = entry->GetNode()->GetMTime();
		*keyLength = sizeof(time_t);
	}
	return entry;
}

// Suspend
status_t
LastModifiedIndex::Iterator::Suspend()
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
LastModifiedIndex::Iterator::Resume()
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
LastModifiedIndex::Iterator::SetTo(LastModifiedIndex *index, time_t modified,
								   bool ignoreValue)
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
			found = fIndex->fNodes->FindFirst(modified, &fIterator);
		// get the first entry
		if (found) {
			if (Node **nodeP = fIterator.GetCurrent()) {
				fNode = *nodeP;
				fEntry = fNode->GetFirstReferrer();
				if (!fEntry)
					BaseClass::GetNext();
				if (!ignoreValue && fNode && fNode->GetMTime() != modified)
					Unset();
			}
		}
	}
	return fEntry;
}

// Unset
void
LastModifiedIndex::Iterator::Unset()
{
	if (fIndex) {
		fIndex->_RemoveIterator(this);
		fIndex = NULL;
	}
	BaseClass::Unset();
}

// EntryRemoved
void
LastModifiedIndex::Iterator::EntryRemoved(Entry */*entry*/)
{
	Resume();
	fIsNext = BaseClass::GetNext();
	Suspend();
}

// NodeRemoved
void
LastModifiedIndex::Iterator::NodeRemoved(Node */*node*/)
{
	Resume();
	fEntry = NULL;
	fIsNext = BaseClass::GetNext();
	Suspend();
}

