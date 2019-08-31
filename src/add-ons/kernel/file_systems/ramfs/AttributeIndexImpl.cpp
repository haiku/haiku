/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TypeConstants.h>

#include "AttributeIndexImpl.h"
#include "DebugSupport.h"
#include "Entry.h"
#include "EntryListener.h"
#include "IndexImpl.h"
#include "Misc.h"
#include "Node.h"
#include "NodeListener.h"
#include "ramfs.h"
#include "TwoKeyAVLTree.h"
#include "Volume.h"

// compare_integral
template<typename Key>
static inline
int
compare_integral(const Key &a, const Key &b)
{
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	return 0;
}

// compare_keys
static
int
compare_keys(const uint8 *key1, size_t length1, const uint8 *key2,
			 size_t length2, uint32 type)
{
	switch (type) {
		case B_INT32_TYPE:
			return compare_integral(*(int32*)key1, *(int32*)key2);
		case B_UINT32_TYPE:
			return compare_integral(*(uint32*)key1, *(uint32*)key2);
		case B_INT64_TYPE:
			return compare_integral(*(int64*)key1, *(int64*)key2);
		case B_UINT64_TYPE:
			return compare_integral(*(uint64*)key1, *(uint64*)key2);
		case B_FLOAT_TYPE:
			return compare_integral(*(float*)key1, *(float*)key2);
		case B_DOUBLE_TYPE:
			return compare_integral(*(double*)key1, *(double*)key2);
		case B_STRING_TYPE:
		{
			int result = strncmp((const char*)key1, (const char*)key2,
								 min(length1, length2));
			if (result == 0) {
				result = compare_integral(strnlen((const char*)key1, length1),
										  strnlen((const char*)key2, length2));
			}
			return result;
		}
	}
	return -1;
}

// PrimaryKey
class AttributeIndexImpl::PrimaryKey {
public:
	PrimaryKey(Attribute *attribute, const uint8 *theKey,
			   size_t length)
		: attribute(attribute), length(length)
			{ memcpy(key, theKey, length); }
	PrimaryKey(Attribute *attribute)
		: attribute(attribute) { attribute->GetKey(key, &length); }
	PrimaryKey(const uint8 *theKey, size_t length)
		: attribute(NULL), length(length)
			{ memcpy(key, theKey, length); }

	Attribute	*attribute;
	uint8		key[kMaxIndexKeyLength];
	size_t		length;
};

// GetPrimaryKey
class AttributeIndexImpl::GetPrimaryKey {
public:
	inline PrimaryKey operator()(Attribute *a)
	{
		return PrimaryKey(a);
	}

	inline PrimaryKey operator()(Attribute *a) const
	{
		return PrimaryKey(a);
	}
};

// PrimaryKeyCompare
class AttributeIndexImpl::PrimaryKeyCompare
{
public:
	PrimaryKeyCompare(uint32 type) : fType(type) {}

	inline int operator()(const PrimaryKey &a,
						  const PrimaryKey &b) const
	{
		if (a.attribute != NULL && a.attribute == b.attribute)
			return 0;
		return compare_keys(a.key, a.length, b.key, b.length, fType);
	}

	uint32	fType;
};

// AttributeNodeIterator
template<typename AttributeIterator>
class AttributeNodeIterator {
public:
	inline Node **GetCurrent()
	{
		if (Attribute **attribute = fIterator.GetCurrent()) {
			fNode = (*attribute)->GetNode();
			return &fNode;
		}
		return NULL;
	}

	inline Node **GetNext()
	{
		if (Attribute **attribute = fIterator.GetNext()) {
			fNode = (*attribute)->GetNode();
			return &fNode;
		}
		return NULL;
	}

	AttributeIterator	fIterator;
	Node				*fNode;
};


// AttributeTree
class AttributeIndexImpl::AttributeTree
	: public TwoKeyAVLTree<Attribute*, PrimaryKey, PrimaryKeyCompare,
						   GetPrimaryKey> {
public:
	AttributeTree(uint32 type)
		: TwoKeyAVLTree<Attribute*, PrimaryKey, PrimaryKeyCompare,
						GetPrimaryKey>(PrimaryKeyCompare(type),
		  	GetPrimaryKey(), TwoKeyAVLTreeStandardCompare<Attribute*>(),
			TwoKeyAVLTreeStandardGetKey<Attribute*, Attribute*>())
	{
	}
};


// Iterator
class AttributeIndexImpl::Iterator
	: public NodeEntryIterator<
		AttributeNodeIterator<AttributeTree::Iterator> >,
	  public DoublyLinkedListLinkImpl<Iterator>, public EntryListener,
	  public NodeListener {
public:
	Iterator();
	virtual ~Iterator();

	virtual Entry *GetCurrent();
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength);

	virtual status_t Suspend();
	virtual status_t Resume();

	bool SetTo(AttributeIndexImpl *index, const uint8 *key, size_t length,
			   bool ignoreValue = false);
	void Unset();

	virtual void EntryRemoved(Entry *entry);
	virtual void NodeRemoved(Node *node);

private:
	typedef NodeEntryIterator<
		AttributeNodeIterator<AttributeTree::Iterator> > BaseClass;

private:
	AttributeIndexImpl	*fIndex;
};


// IteratorList
class AttributeIndexImpl::IteratorList : public DoublyLinkedList<Iterator> {};


// AttributeIndexImpl

// constructor
AttributeIndexImpl::AttributeIndexImpl(Volume *volume, const char *name,
									   uint32 type, size_t keyLength)
	: AttributeIndex(volume, name, type, (keyLength > 0), keyLength),
	  fAttributes(new(nothrow) AttributeTree(type)),
	  fIterators(new(nothrow) IteratorList)
{
	if (fInitStatus == B_OK && (!fAttributes || !fIterators))
		fInitStatus = B_NO_MEMORY;
}

// destructor
AttributeIndexImpl::~AttributeIndexImpl()
{
	if (fIterators) {
		// unset the iterators
		for (Iterator *iterator = fIterators->First();
			 iterator;
			 iterator = fIterators->GetNext(iterator)) {
			iterator->SetTo(NULL, NULL, 0);
		}
		delete fIterators;
	}
	// unset all attributes and delete the tree
	if (fAttributes) {
		AttributeTree::Iterator it;
		fAttributes->GetIterator(&it);
		for (Attribute **attribute = it.GetCurrent(); attribute; it.GetNext())
			(*attribute)->SetIndex(NULL, false);
		delete fAttributes;
	}
}

// CountEntries
int32
AttributeIndexImpl::CountEntries() const
{
	return fAttributes->CountItems();
}

// Changed
status_t
AttributeIndexImpl::Changed(Attribute *attribute, const uint8 *oldKey,
							size_t oldLength)
{
	status_t error = B_BAD_VALUE;
	if (attribute && attribute->GetIndex() == this) {
		// update the iterators and remove the attribute from the tree
		error = B_OK;
		if (attribute->IsInIndex()) {
			AttributeTree::Iterator it;
			Attribute **foundAttribute = fAttributes->Find(
				PrimaryKey(attribute, oldKey, oldLength), attribute, &it);
			if (foundAttribute && *foundAttribute == attribute) {
				Node *node = attribute->GetNode();
				// update the iterators
				for (Iterator *iterator = fIterators->First();
					 iterator;
					 iterator = fIterators->GetNext(iterator)) {
					if (iterator->GetCurrentNode() == node)
						iterator->NodeRemoved(node);
				}
				// remove and re-insert the attribute
				it.Remove();
			}
		}
		// re-insert the attribute
		if (fKeyLength > 0 && attribute->GetSize() != fKeyLength) {
			attribute->SetIndex(this, false);
		} else {
			error = fAttributes->Insert(attribute);
			if (error == B_OK)
				attribute->SetIndex(this, true);
			else
				attribute->SetIndex(NULL, false);
		}
	}
	return error;
}

// Added
status_t
AttributeIndexImpl::Added(Attribute *attribute)
{
PRINT("AttributeIndex::Add(%p)\n", attribute);
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		size_t size = attribute->GetSize();
		if (fKeyLength > 0 && size != fKeyLength) {
			attribute->SetIndex(this, false);
		} else {
			error = fAttributes->Insert(attribute);
			if (error == B_OK)
				attribute->SetIndex(this, true);
		}
	}
	return error;
}

// Removed
bool
AttributeIndexImpl::Removed(Attribute *attribute)
{
PRINT("AttributeIndex::Removed(%p)\n", attribute);
	bool result = (attribute && attribute->GetIndex() == this);
	if (result) {
		if (attribute->IsInIndex())
			fAttributes->Remove(attribute, attribute);
		attribute->SetIndex(NULL, false);
	}
	return result;
}

// InternalGetIterator
AbstractIndexEntryIterator *
AttributeIndexImpl::InternalGetIterator()
{
	Iterator *iterator = new(nothrow) Iterator;
	if (iterator) {
		if (!iterator->SetTo(this, NULL, 0, true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// InternalFind
AbstractIndexEntryIterator *
AttributeIndexImpl::InternalFind(const uint8 *key, size_t length)
{
	if (!key || (fKeyLength > 0 && length != fKeyLength))
		return NULL;
	Iterator *iterator = new(nothrow) Iterator;
	if (iterator) {
		if (!iterator->SetTo(this, key, length)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}

// _AddIterator
void
AttributeIndexImpl::_AddIterator(Iterator *iterator)
{
	fIterators->Insert(iterator);
}

// _RemoveIterator
void
AttributeIndexImpl::_RemoveIterator(Iterator *iterator)
{
	fIterators->Remove(iterator);
}


// Iterator

// constructor
AttributeIndexImpl::Iterator::Iterator()
	: BaseClass(),
	  fIndex(NULL)
{
}

// destructor
AttributeIndexImpl::Iterator::~Iterator()
{
	SetTo(NULL, NULL, 0);
}

// GetCurrent
Entry *
AttributeIndexImpl::Iterator::GetCurrent()
{
	return BaseClass::GetCurrent();
}

// GetCurrent
Entry *
AttributeIndexImpl::Iterator::GetCurrent(uint8 *buffer, size_t *keyLength)
{
	Entry *entry = GetCurrent();
	if (entry) {
		if (Attribute **attribute = fIterator.fIterator.GetCurrent()) {
			if ((*attribute)->GetNode() == entry->GetNode()) {
				(*attribute)->GetKey(buffer, keyLength);
			} else {
				FATAL("Node of current attribute and node of current entry "
					   "differ: %" B_PRIdINO " vs. %" B_PRIdINO "\n",
					   (*attribute)->GetNode()->GetID(),
					   entry->GetNode()->GetID());
				entry = NULL;
			}
		} else {
			FATAL("We have a current entry (`%s', node: %" B_PRIdINO "), but no current "
				   "attribute.\n", entry->GetName(),
				   entry->GetNode()->GetID());
			entry = NULL;
		}
	}
	return entry;
}

// Suspend
status_t
AttributeIndexImpl::Iterator::Suspend()
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
AttributeIndexImpl::Iterator::Resume()
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
AttributeIndexImpl::Iterator::SetTo(AttributeIndexImpl *index,
	const uint8 *key, size_t length, bool ignoreValue)
{
	Resume();
	Unset();
	// set the new values
	fIndex = index;
	if (fIndex)
		fIndex->_AddIterator(this);
	fInitialized = fIndex;
	// get the attribute node's first entry
	if (fIndex) {
		// get the first node
		bool found = true;
		if (ignoreValue)
			fIndex->fAttributes->GetIterator(&fIterator.fIterator);
		else {
			found = fIndex->fAttributes->FindFirst(PrimaryKey(key, length),
												   &(fIterator.fIterator));
		}
		// get the first entry
		if (found) {
			if (Node **nodeP = fIterator.GetCurrent()) {
				fNode = *nodeP;
				fEntry = fNode->GetFirstReferrer();
				if (!fEntry)
					BaseClass::GetNext();
				if (Attribute **attribute = fIterator.fIterator.GetCurrent()) {
					uint8 attrKey[kMaxIndexKeyLength];
					size_t attrKeyLength;
					(*attribute)->GetKey(attrKey, &attrKeyLength);
					if (!ignoreValue
						&& compare_keys(attrKey, attrKeyLength, key, length,
										fIndex->GetType()) != 0) {
						Unset();
					}
				}
			}
		}
	}
	return fEntry;
}

// Unset
void
AttributeIndexImpl::Iterator::Unset()
{
	if (fIndex) {
		fIndex->_RemoveIterator(this);
		fIndex = NULL;
	}
	BaseClass::Unset();
}

// EntryRemoved
void
AttributeIndexImpl::Iterator::EntryRemoved(Entry */*entry*/)
{
	Resume();
	fIsNext = BaseClass::GetNext();
	Suspend();
}

// NodeRemoved
void
AttributeIndexImpl::Iterator::NodeRemoved(Node */*node*/)
{
	Resume();
	fEntry = NULL;
	fIsNext = BaseClass::GetNext();
	Suspend();
}

