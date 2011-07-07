/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "NameIndex.h"

#include <TypeConstants.h>

#include "DebugSupport.h"
#include "IndexImpl.h"
#include "Node.h"
#include "TwoKeyAVLTree.h"
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


// #pragma mark -  Iterator


struct NameIndex::IteratorPolicy {
	typedef NameIndex				Index;
	typedef const char*				Value;
	typedef NameIndex::EntryTree	NodeTree;

	static NodeTree* GetNodeTree(Index* index)
	{
		return index->fEntries;
	}

	static void GetNodeValue(Node* node, void* buffer, size_t* _keyLength)
	{
		strlcpy((char*)buffer, node->Name(), kMaxIndexKeyLength);
		*_keyLength = strlen(node->Name());
	}
};


struct NameIndex::Iterator : public GenericIndexIterator<IteratorPolicy> {
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
	if (IsListening())
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

	fVolume->AddNodeListener(this, NULL);

	fEntries = new(std::nothrow) EntryTree;
	if (fEntries == NULL)
		return B_NO_MEMORY;

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
NameIndex::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	// nothing to do -- the name remains the same
}


AbstractIndexIterator*
NameIndex::InternalGetIterator()
{
	Iterator* iterator = new(std::nothrow) Iterator;
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

	Iterator* iterator = new(std::nothrow) Iterator;
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
