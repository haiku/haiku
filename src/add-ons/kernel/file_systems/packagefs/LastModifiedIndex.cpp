/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "LastModifiedIndex.h"

#include <new>

#include <TypeConstants.h>

#include <util/SinglyLinkedList.h>

#include "DebugSupport.h"
#include "IndexImpl.h"
#include "Node.h"
#include "TwoKeyAVLTree.h"
#include "Volume.h"


// #pragma mark - LastModifiedIndexPrimaryKey


class LastModifiedIndexPrimaryKey {
public:
	LastModifiedIndexPrimaryKey(Node* node, time_t modified)
		:
		node(node),
		modified(modified)
	{
	}

	LastModifiedIndexPrimaryKey(Node* node)
		:
		node(node),
		modified(node->ModifiedTime().tv_sec)
	{
	}

	LastModifiedIndexPrimaryKey(time_t modified)
		:
		node(NULL),
		modified(modified)
	{
	}

	Node*	node;
	time_t	modified;
};


// #pragma mark - LastModifiedIndexGetPrimaryKey


class LastModifiedIndexGetPrimaryKey {
public:
	inline LastModifiedIndexPrimaryKey operator()(Node* a)
	{
		return LastModifiedIndexPrimaryKey(a);
	}

	inline LastModifiedIndexPrimaryKey operator()(Node* a) const
	{
		return LastModifiedIndexPrimaryKey(a);
	}
};


// #pragma mark - LastModifiedIndexPrimaryKeyCompare


class LastModifiedIndexPrimaryKeyCompare {
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


// #pragma mark - NodeTree


typedef TwoKeyAVLTree<Node*, LastModifiedIndexPrimaryKey,
		LastModifiedIndexPrimaryKeyCompare, LastModifiedIndexGetPrimaryKey>
	_NodeTree;
class LastModifiedIndex::NodeTree : public _NodeTree {};


// #pragma mark - IteratorList


class LastModifiedIndex::IteratorList : public SinglyLinkedList<Iterator> {};


// #pragma mark - Iterator


struct LastModifiedIndex::IteratorPolicy {
	typedef LastModifiedIndex			Index;
	typedef time_t						Value;
	typedef LastModifiedIndex::NodeTree	NodeTree;

	static NodeTree* GetNodeTree(Index* index)
	{
		return index->fNodes;
	}

	static void GetNodeValue(Node* node, void* buffer, size_t* _keyLength)
	{
		*(time_t*)buffer = node->ModifiedTime().tv_sec;
		*_keyLength = sizeof(time_t);
	}
};


class LastModifiedIndex::Iterator : public GenericIndexIterator<IteratorPolicy>,
	public SinglyLinkedListLinkImpl<Iterator> {
public:
	virtual	void				NodeChanged(Node* node, uint32 statFields,
									const OldNodeAttributes& oldAttributes);
};


// #pragma mark - LastModifiedIndex


LastModifiedIndex::LastModifiedIndex()
	:
	Index(),
	fNodes(NULL),
	fIteratorsToUpdate(NULL)
{
}


LastModifiedIndex::~LastModifiedIndex()
{
	if (IsListening())
		fVolume->RemoveNodeListener(this);

	ASSERT(fIteratorsToUpdate->IsEmpty());

	delete fIteratorsToUpdate;
	delete fNodes;
}


status_t
LastModifiedIndex::Init(Volume* volume)
{
	status_t error = Index::Init(volume, "last_modified", B_INT32_TYPE, true,
		sizeof(time_t));
	if (error != B_OK)
		return error;

	fVolume->AddNodeListener(this, NULL);

	fNodes = new(std::nothrow) NodeTree;
	fIteratorsToUpdate = new(std::nothrow) IteratorList;
	if (fNodes == NULL || fIteratorsToUpdate == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


int32
LastModifiedIndex::CountEntries() const
{
	return fNodes->CountItems();
}


void
LastModifiedIndex::NodeAdded(Node* node)
{
	fNodes->Insert(node);
}


void
LastModifiedIndex::NodeRemoved(Node* node)
{
	fNodes->Remove(node, node);
}


void
LastModifiedIndex::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	IteratorList iterators;
	iterators.MoveFrom(fIteratorsToUpdate);

	time_t oldLastModified = oldAttributes.ModifiedTime().tv_sec;
	time_t newLastModified = node->ModifiedTime().tv_sec;
	if (newLastModified == oldLastModified)
		return;

	NodeTree::Iterator nodeIterator;
	Node** foundNode = fNodes->Find(
		LastModifiedIndexPrimaryKey(node, oldLastModified), node,
		&nodeIterator);

	if (foundNode == NULL || *foundNode != node)
		return;

	// move the iterators that point to the node to the previous node
	for (IteratorList::Iterator it = iterators.GetIterator();
			Iterator* iterator = it.Next();) {
		iterator->NodeChangeBegin(node);
	}

	// remove and re-insert the node
	nodeIterator.Remove();
	if (fNodes->Insert(node) != B_OK) {
		fIteratorsToUpdate->MakeEmpty();
		return;
	}

	// Move the iterators to the next node again. If the node hasn't changed
	// its place, they will point to it again, otherwise to the node originally
	// succeeding it.
	for (IteratorList::Iterator it = iterators.GetIterator();
			Iterator* iterator = it.Next();) {
		iterator->NodeChangeEnd(node);
	}

	// update live queries
	fVolume->UpdateLiveQueries(node, Name(), Type(),
		(const uint8*)&oldLastModified, sizeof(oldLastModified),
		(const uint8*)&newLastModified, sizeof(newLastModified));
}


AbstractIndexIterator*
LastModifiedIndex::InternalGetIterator()
{
	Iterator* iterator = new(std::nothrow) Iterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, 0, true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


AbstractIndexIterator*
LastModifiedIndex::InternalFind(const void* key, size_t length)
{
	if (!key || length != sizeof(time_t))
		return NULL;
	Iterator* iterator = new(std::nothrow) Iterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, *(const time_t*)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


void
LastModifiedIndex::_AddIteratorToUpdate(Iterator* iterator)
{
	fIteratorsToUpdate->Add(iterator);
}


// #pragma mark - Iterator


void
LastModifiedIndex::Iterator::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	fIndex->_AddIteratorToUpdate(this);
}
