/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeIndex.h"

#include <new>

#include <TypeConstants.h>

#include <util/AVLTree.h>
#include <util/SinglyLinkedList.h>

#include <file_systems/QueryParserUtils.h>

#include "AttributeIndexer.h"
#include "DebugSupport.h"
#include "IndexImpl.h"
#include "Node.h"
#include "Volume.h"


struct AttributeIndexTreeKey {
	const void*	data;
	size_t		length;

	AttributeIndexTreeKey()
	{
	}

	AttributeIndexTreeKey(const void* data, size_t length)
		:
		data(data),
		length(length)
	{
	}
};


struct AttributeIndexTreeValue : AVLTreeNode {
	Node*					node;
	IndexedAttributeOwner*	owner;
	void*					attributeCookie;
	size_t					length;
	uint8					data[0];

	static AttributeIndexTreeValue* Create(IndexedAttributeOwner* owner,
		void* attributeCookie, size_t length)
	{
		AttributeIndexTreeValue* self = (AttributeIndexTreeValue*)malloc(
			sizeof(AttributeIndexTreeValue) + length);
		if (self == NULL)
			return NULL;

		self->owner = owner;
		self->attributeCookie = attributeCookie;
		self->length = length;
		return self;
	}

	void Delete()
	{
		free(this);
	}
};


struct AttributeIndex::TreeDefinition {
	typedef TreeKey		Key;
	typedef TreeValue	Value;

	TreeDefinition(uint32 type)
		:
		fType(type)
	{
	}

	AVLTreeNode* GetAVLTreeNode(Value* value) const
	{
		return value;
	}

	Value* GetValue(AVLTreeNode* node) const
	{
		return static_cast<Value*>(node);
	}

	int Compare(const Key& a, const Value* b) const
	{
		return QueryParser::compareKeys(fType, a.data, a.length, b->data,
			b->length);
	}

	int Compare(const Value* a, const Value* b) const
	{
		return QueryParser::compareKeys(fType, a->data, a->length, b->data,
			b->length);
	}

private:
	uint32	fType;
};


// #pragma mark - NodeTree


struct AttributeIndex::NodeTree : public AVLTree<TreeDefinition> {
	typedef TreeValue	Node;

	NodeTree(const TreeDefinition& definition)
		:
		AVLTree<TreeDefinition>(definition)
	{
	}
};


// #pragma mark - IteratorList


class AttributeIndex::IteratorList : public SinglyLinkedList<Iterator> {};


// #pragma mark - Iterator


struct AttributeIndex::IteratorPolicy {
	typedef AttributeIndex				Index;
	typedef TreeKey						Value;
	typedef AttributeIndex::NodeTree	NodeTree;
	typedef TreeValue					TreeNode;
	typedef IteratorPolicy				TreePolicy;

	static NodeTree* GetNodeTree(Index* index)
	{
		return index->fNodes;
	}

	static void GetTreeNodeValue(TreeNode* node, void* buffer,
		size_t* _keyLength)
	{
		if (node->length > 0)
			memcpy(buffer, node->data, node->length);
		*_keyLength = node->length;
	}

	static Node* GetNode(TreeNode* treeNode)
	{
		return treeNode->node;
	}

	static TreeNode* GetFirstTreeNode(Index* index)
	{
		return index->fNodes->GetIterator().Next();
	}

	static TreeNode* FindClosestTreeNode(Index* index, const Value& value)
	{
		return index->fNodes->FindClosest(value, false);
	}
};


class AttributeIndex::Iterator : public GenericIndexIterator<IteratorPolicy>,
	public SinglyLinkedListLinkImpl<Iterator> {
public:
	virtual	void				NodeChanged(Node* node, uint32 statFields,
									const OldNodeAttributes& oldAttributes);
};


// #pragma mark - AttributeIndexer


AttributeIndexer::AttributeIndexer(AttributeIndex* index)
	:
	fIndex(index),
	fIndexName(index->Name()),
	fIndexType(index->Type()),
	fCookie(NULL)
{
}


AttributeIndexer::~AttributeIndexer()
{
}


status_t
AttributeIndexer::CreateCookie(IndexedAttributeOwner* owner,
	void* attributeCookie, uint32 attributeType, size_t attributeSize,
	void*& _data, size_t& _toRead)
{
	// check the attribute type and size
	if (attributeType != fIndexType)
		return B_ERROR;

	if (fIndex->HasFixedKeyLength()) {
		if (attributeSize != fIndex->KeyLength())
			return B_ERROR;
	} else if (attributeSize > kMaxIndexKeyLength)
		attributeSize = kMaxIndexKeyLength;

	// create the tree value
	fCookie = AttributeIndexTreeValue::Create(owner, attributeCookie,
		attributeSize);
	if (fCookie == NULL)
		return B_NO_MEMORY;

	_data = fCookie->data;
	_toRead = attributeSize;

	return B_OK;
}


void
AttributeIndexer::DeleteCookie()
{
	fCookie->Delete();
	fCookie = NULL;
}


// #pragma mark - AttributeIndex


AttributeIndex::AttributeIndex()
	:
	Index(),
	fNodes(NULL),
	fIteratorsToUpdate(NULL),
	fIndexer(NULL)
{
}


AttributeIndex::~AttributeIndex()
{
	if (IsListening())
		fVolume->RemoveNodeListener(this);

	ASSERT(fIteratorsToUpdate->IsEmpty());

	delete fIteratorsToUpdate;
	delete fNodes;
	delete fIndexer;
}


status_t
AttributeIndex::Init(Volume* volume, const char* name,  uint32 type,
	size_t keyLength)
{
	status_t error = Index::Init(volume, name, type, keyLength > 0, keyLength);
	if (error != B_OK)
		return error;

	// TODO: Letting each attribute index be a listener is gets more expensive
	// the more attribute indices we have. Since most attribute indices are
	// rather sparse, it might be a good idea to rather let Volume iterate
	// through the actual attributes of an added node and look up and call the
	// index for each one explicitly. When removing the node, the volume would
	// iterate through the attributes again and determine based on the index
	// cookie whether an index has to be notified.
	fVolume->AddNodeListener(this, NULL);

	fNodes = new(std::nothrow) NodeTree(TreeDefinition(type));
	fIteratorsToUpdate = new(std::nothrow) IteratorList;
	fIndexer = new(std::nothrow) AttributeIndexer(this);

	if (fNodes == NULL || fIteratorsToUpdate == NULL || fIndexer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


int32
AttributeIndex::CountEntries() const
{
	return fNodes->Count();
}


void
AttributeIndex::NodeAdded(Node* node)
{
	if (node->IndexAttribute(fIndexer) != B_OK)
		return;

	TreeValue* treeValue = fIndexer->Cookie();
	treeValue->node = node;

	fNodes->Insert(treeValue);
}


void
AttributeIndex::NodeRemoved(Node* node)
{
	TreeValue* treeValue = (TreeValue*)node->IndexCookieForAttribute(Name());
	if (treeValue == NULL)
		return;

	treeValue->owner->UnsetIndexCookie(treeValue->attributeCookie);
	fNodes->Remove(treeValue);
}


void
AttributeIndex::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	IteratorList iterators;
	iterators.MoveFrom(fIteratorsToUpdate);

	TreeValue* oldTreeValue
		= (TreeValue*)oldAttributes.IndexCookieForAttribute(Name());
	TreeValue* treeValue = (TreeValue*)node->IndexCookieForAttribute(Name());
	if (treeValue == NULL && oldTreeValue == NULL)
		return;

	// move the iterators that point to the node to the previous node
	if (oldTreeValue != NULL) {
		for (IteratorList::Iterator it = iterators.GetIterator();
				Iterator* iterator = it.Next();) {
			iterator->NodeChangeBegin(node);
		}

		// remove the node
		fNodes->Remove(oldTreeValue);
	}

	// re-insert the node
	if (treeValue != NULL)
		fNodes->Insert(treeValue);

	// Move the iterators to the next node again. If the node hasn't changed
	// its place, they will point to it again, otherwise to the node originally
	// succeeding it.
	if (oldTreeValue != NULL) {
		for (IteratorList::Iterator it = iterators.GetIterator();
				Iterator* iterator = it.Next();) {
			iterator->NodeChangeEnd(node);
		}
	}

	// update live queries
	fVolume->UpdateLiveQueries(node, Name(), Type(),
		oldTreeValue != NULL ? oldTreeValue->data : NULL,
		oldTreeValue != NULL ? oldTreeValue->length : 0,
		treeValue != NULL ? treeValue->data : NULL,
		treeValue != NULL ? treeValue->length : 0);

	if (oldTreeValue != NULL)
		oldTreeValue->Delete();
}


AbstractIndexIterator*
AttributeIndex::InternalGetIterator()
{
	Iterator* iterator = new(std::nothrow) Iterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, TreeKey(), true)) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


AbstractIndexIterator*
AttributeIndex::InternalFind(const void* key, size_t length)
{
	if (key == NULL)
		return NULL;
	Iterator* iterator = new(std::nothrow) Iterator;
	if (iterator != NULL) {
		if (!iterator->SetTo(this, TreeKey(key, length))) {
			delete iterator;
			iterator = NULL;
		}
	}
	return iterator;
}


void
AttributeIndex::_AddIteratorToUpdate(Iterator* iterator)
{
	fIteratorsToUpdate->Add(iterator);
}


// #pragma mark - Iterator


void
AttributeIndex::Iterator::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	fIndex->_AddIteratorToUpdate(this);
}
