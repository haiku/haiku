/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "NameIndex.h"

#include <TypeConstants.h>

#include <util/TwoKeyAVLTree.h>

#include "DebugSupport.h"
#include "IndexImpl.h"
#include "Node.h"
#include "Volume.h"


// #pragma mark - NameIndexPrimaryKey


class NameIndexPrimaryKey {
public:
	NameIndexPrimaryKey(const Node* entry, const char* name = NULL)
		:
		entry(entry),
		name(name ? name : entry->Name())
	{
	}

	NameIndexPrimaryKey(const char* name)
		:
		entry(NULL),
		name(name)
	{
	}

	const Node*	entry;
	const char*	name;
};


// #pragma mark - NameIndexGetPrimaryKey


class NameIndexGetPrimaryKey {
public:
	inline NameIndexPrimaryKey operator()(const Node* a)
	{
		return NameIndexPrimaryKey(a);
	}

	inline NameIndexPrimaryKey operator()(const Node* a) const
	{
		return NameIndexPrimaryKey(a);
	}
};


// #pragma mark - NameIndexPrimaryKeyCompare


class NameIndexPrimaryKeyCompare {
public:
	inline int operator()(const NameIndexPrimaryKey &a,
		const NameIndexPrimaryKey &b) const
	{
		if (a.entry != NULL && a.entry == b.entry)
			return 0;
		return strcmp(a.name, b.name);
	}
};


// #pragma mark - EntryTree


typedef TwoKeyAVLTree<Node*, NameIndexPrimaryKey, NameIndexPrimaryKeyCompare,
	NameIndexGetPrimaryKey> _EntryTree;

class NameIndex::EntryTree : public _EntryTree {
};


// #pragma mark -  NameIndexIterator


class NameIndexIterator : public AbstractIndexIterator,
	public NodeListener {
public:
								NameIndexIterator();
	virtual						~NameIndexIterator();

	virtual	bool				HasNext() const;
	virtual	Node*				Next(void* buffer, size_t* _keyLength);

	virtual	status_t			Suspend();
	virtual	status_t			Resume();

			bool				SetTo(NameIndex* index, const char* name,
									bool ignoreValue = false);

	virtual void				NodeRemoved(Node* node);

private:
			friend class NameIndex;

			typedef NameIndex::EntryTree EntryTree;

private:
	inline	Node*				_ToNode() const;

private:
			NameIndex*			fIndex;
			EntryTree::Node*	fNextTreeNode;
			bool				fSuspended;
};


// #pragma mark - NameIndex


NameIndex::NameIndex()
	:
	Index(),
	fEntries(NULL)
{
}


NameIndex::~NameIndex()
{
	if (fVolume != NULL)
		fVolume->RemoveNodeListener(this);

	delete fEntries;
	// Actually we would need to maintain a list of iterators and unset the
	// still existing iterators here. But since the name index is deleted
	// when the volume is unmounted, there shouldn't be any iterators left
	// anymore.
}


status_t
NameIndex::Init(Volume* volume)
{
	status_t error = Index::Init(volume, "name", B_STRING_TYPE, false);
	if (error != B_OK)
		return error;

	fEntries = new(std::nothrow) EntryTree;
	if (fEntries == NULL)
		return B_NO_MEMORY;

	fVolume = volume;
	fVolume->AddNodeListener(this, NULL);

	return B_OK;
}


int32
NameIndex::CountEntries() const
{
	return fEntries->CountItems();
}


void
NameIndex::NodeAdded(Node* node)
{
	fEntries->Insert(node);

	// update live queries
	_UpdateLiveQueries(node, NULL, node->Name());
}


void
NameIndex::NodeRemoved(Node* node)
{
	fEntries->Remove(node, node);

	// update live queries
	_UpdateLiveQueries(node, node->Name(), NULL);
}


void
NameIndex::NodeChanged(Node* node, uint32 statFields)
{
	// nothing to do -- the name remains the same
}


AbstractIndexIterator*
NameIndex::InternalGetIterator()
{
	NameIndexIterator* iterator = new(std::nothrow) NameIndexIterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, NULL, true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


AbstractIndexIterator*
NameIndex::InternalFind(const void* _key, size_t length)
{
	if (_key == NULL || length == 0)
		return NULL;

	const char* key = (const char*)_key;

	// if the key is not null-terminated, copy it
	char clonedKey[kMaxIndexKeyLength];
	if (key[length - 1] != '\0') {
		if (length >= kMaxIndexKeyLength)
			length = kMaxIndexKeyLength - 1;

		memcpy(clonedKey, key, length);
		clonedKey[length] = '\0';
		length++;
		key = clonedKey;
	}

	NameIndexIterator* iterator = new(std::nothrow) NameIndexIterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, (const char*)key)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


void
NameIndex::_UpdateLiveQueries(Node* entry, const char* oldName,
	const char* newName)
{
	fVolume->UpdateLiveQueries(entry, Name(), Type(),
		oldName, oldName ? strlen(oldName) : 0,
		newName, newName ? strlen(newName) : 0);
}


// #pragma mark - NameIndexIterator


NameIndexIterator::NameIndexIterator()
	:
	AbstractIndexIterator(),
	fIndex(NULL),
	fNextTreeNode(NULL),
	fSuspended(false)
{
}


NameIndexIterator::~NameIndexIterator()
{
	SetTo(NULL, NULL);
}


bool
NameIndexIterator::HasNext() const
{
	return fNextTreeNode != NULL;
}


Node*
NameIndexIterator::Next(void* buffer, size_t* _keyLength)
{
	if (fSuspended || fNextTreeNode == NULL)
		return NULL;

	Node* entry = _ToNode();
	if (entry != NULL) {
		if (buffer != NULL) {
			strlcpy((char*)buffer, entry->Name(), kMaxIndexKeyLength);
			*_keyLength = strlen(entry->Name());
		}

		fNextTreeNode = fIndex->fEntries->Next(fNextTreeNode);
	}

	return entry;
}


status_t
NameIndexIterator::Suspend()
{
	if (fSuspended)
		return B_BAD_VALUE;

	if (fNextTreeNode != NULL)
		fIndex->GetVolume()->AddNodeListener(this, _ToNode());

	fSuspended = true;
	return B_OK;
}


status_t
NameIndexIterator::Resume()
{
	if (!fSuspended)
		return B_BAD_VALUE;

	if (fNextTreeNode != NULL)
		fIndex->GetVolume()->RemoveNodeListener(this);

	fSuspended = false;
	return B_OK;
}


bool
NameIndexIterator::SetTo(NameIndex* index, const char* name, bool ignoreValue)
{
	Resume();

	fIndex = index;
	fSuspended = false;
	fNextTreeNode = NULL;

	if (fIndex == NULL)
		return false;

	EntryTree::Iterator iterator;
	if (ignoreValue)
		fIndex->fEntries->GetIterator(&iterator);
	else if (fIndex->fEntries->FindFirst(name, &iterator) == NULL)
		return false;

	fNextTreeNode = iterator.CurrentNode();
	return fNextTreeNode != NULL;
}


void
NameIndexIterator::NodeRemoved(Node* node)
{
	Resume();
	Next(NULL, NULL);
	Suspend();
}


Node*
NameIndexIterator::_ToNode() const
{
	return EntryTree::NodeStrategy().GetValue(fNextTreeNode);
}
