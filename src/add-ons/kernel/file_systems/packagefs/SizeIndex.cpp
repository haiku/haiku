/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SizeIndex.h"

#include <new>

#include <TypeConstants.h>

#include <util/SinglyLinkedList.h>

#include "DebugSupport.h"
#include "IndexImpl.h"
#include "Node.h"
#include "TwoKeyAVLTree.h"
#include "Volume.h"


// #pragma mark - SizeIndexPrimaryKey


class SizeIndexPrimaryKey {
public:
	SizeIndexPrimaryKey(Node* node, off_t size)
		:
		node(node),
		size(size)
	{
	}

	SizeIndexPrimaryKey(Node* node)
		:
		node(node),
		size(node->FileSize())
	{
	}

	SizeIndexPrimaryKey(off_t size)
		:
		node(NULL),
		size(size)
	{
	}

	Node*	node;
	off_t	size;
};


// #pragma mark - SizeIndexGetPrimaryKey


class SizeIndexGetPrimaryKey {
public:
	inline SizeIndexPrimaryKey operator()(Node* a)
	{
		return SizeIndexPrimaryKey(a);
	}

	inline SizeIndexPrimaryKey operator()(Node* a) const
	{
		return SizeIndexPrimaryKey(a);
	}
};


// #pragma mark - SizeIndexPrimaryKeyCompare


class SizeIndexPrimaryKeyCompare {
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


// #pragma mark - NodeTree


typedef TwoKeyAVLTree<Node*, SizeIndexPrimaryKey, SizeIndexPrimaryKeyCompare,
	SizeIndexGetPrimaryKey> _NodeTree;
class SizeIndex::NodeTree : public _NodeTree {};


// #pragma mark - IteratorList


class SizeIndex::IteratorList : public SinglyLinkedList<Iterator> {};


// #pragma mark - Iterator


struct SizeIndex::IteratorPolicy {
	typedef SizeIndex				Index;
	typedef off_t					Value;
	typedef SizeIndex::NodeTree		NodeTree;

	static NodeTree* GetNodeTree(Index* index)
	{
		return index->fNodes;
	}

	static void GetNodeValue(Node* node, void* buffer, size_t* _keyLength)
	{
		*(off_t*)buffer = node->FileSize();
		*_keyLength = sizeof(off_t);
	}
};


class SizeIndex::Iterator : public GenericIndexIterator<IteratorPolicy>,
	public SinglyLinkedListLinkImpl<Iterator> {
public:
	virtual	void				NodeChanged(Node* node, uint32 statFields,
									const OldNodeAttributes& oldAttributes);
};


// #pragma mark - SizeIndex


SizeIndex::SizeIndex()
	:
	Index(),
	fNodes(NULL),
	fIteratorsToUpdate(NULL)
{
}


SizeIndex::~SizeIndex()
{
	if (IsListening())
		fVolume->RemoveNodeListener(this);

	ASSERT(fIteratorsToUpdate->IsEmpty());

	delete fIteratorsToUpdate;
	delete fNodes;
}


status_t
SizeIndex::Init(Volume* volume)
{
	status_t error = Index::Init(volume, "size", B_INT64_TYPE, true,
		sizeof(off_t));
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
SizeIndex::CountEntries() const
{
	return fNodes->CountItems();
}


void
SizeIndex::NodeAdded(Node* node)
{
	fNodes->Insert(node);
}


void
SizeIndex::NodeRemoved(Node* node)
{
	fNodes->Remove(node, node);
}


void
SizeIndex::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	IteratorList iterators;
	iterators.MoveFrom(fIteratorsToUpdate);

	off_t oldSize = oldAttributes.FileSize();
	off_t newSize = node->FileSize();
	if (newSize == oldSize)
		return;

	NodeTree::Iterator nodeIterator;
	Node** foundNode = fNodes->Find(SizeIndexPrimaryKey(node, oldSize), node,
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
		(const uint8*)&oldSize, sizeof(oldSize), (const uint8*)&newSize,
		sizeof(newSize));
}


AbstractIndexIterator*
SizeIndex::InternalGetIterator()
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
SizeIndex::InternalFind(const void* key, size_t length)
{
	if (!key || length != sizeof(off_t))
		return NULL;
	Iterator* iterator = new(std::nothrow) Iterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, *(const off_t*)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


void
SizeIndex::_AddIteratorToUpdate(Iterator* iterator)
{
	fIteratorsToUpdate->Add(iterator);
}


// #pragma mark - Iterator


void
SizeIndex::Iterator::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	fIndex->_AddIteratorToUpdate(this);
}
