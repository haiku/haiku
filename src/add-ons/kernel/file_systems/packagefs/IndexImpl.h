/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INDEX_IMPL_H
#define INDEX_IMPL_H


#include "Index.h"
#include "Node.h"
#include "NodeListener.h"


class AbstractIndexIterator {
public:
								AbstractIndexIterator();
	virtual						~AbstractIndexIterator();

	virtual	bool				HasNext() const = 0;
	virtual Node*				Next(void* buffer, size_t* _keyLength) = 0;

	virtual	status_t			Suspend();
	virtual	status_t			Resume();
};


template<typename Policy>
class GenericIndexIterator : public AbstractIndexIterator,
	public NodeListener {
public:
			typedef typename Policy::Index		Index;
			typedef typename Policy::Value		Value;
			typedef typename Policy::NodeTree	NodeTree;
			typedef typename NodeTree::Node		TreeNode;

public:
								GenericIndexIterator();
	virtual						~GenericIndexIterator();

	virtual	bool				HasNext() const;
	virtual	Node*				Next(void* buffer, size_t* _keyLength);

	virtual	status_t			Suspend();
	virtual	status_t			Resume();

			bool				SetTo(Index* index, const Value& name,
									bool ignoreValue = false);

	inline	void				NodeChangeBegin(Node* node);
	inline	void				NodeChangeEnd(Node* node);

	virtual void				NodeRemoved(Node* node);

protected:
	inline	Node*				_ToNode() const;

protected:
			Index*				fIndex;
			TreeNode*			fNextTreeNode;
			bool				fSuspended;
};


template<typename Policy>
GenericIndexIterator<Policy>::GenericIndexIterator()
	:
	AbstractIndexIterator(),
	fIndex(NULL),
	fNextTreeNode(NULL),
	fSuspended(false)
{
}


template<typename Policy>
GenericIndexIterator<Policy>::~GenericIndexIterator()
{
	SetTo(NULL, NULL);
}


template<typename Policy>
bool
GenericIndexIterator<Policy>::HasNext() const
{
	return fNextTreeNode != NULL;
}


template<typename Policy>
Node*
GenericIndexIterator<Policy>::Next(void* buffer, size_t* _keyLength)
{
	if (fSuspended || fNextTreeNode == NULL)
		return NULL;

	Node* node = _ToNode();
	if (node != NULL) {
		if (buffer != NULL)
			Policy::GetNodeValue(node, buffer, _keyLength);

		fNextTreeNode = Policy::GetNodeTree(fIndex)->Next(fNextTreeNode);
	}

	return node;
}


template<typename Policy>
status_t
GenericIndexIterator<Policy>::Suspend()
{
	if (fSuspended)
		return B_BAD_VALUE;

	if (fNextTreeNode != NULL)
		fIndex->GetVolume()->AddNodeListener(this, _ToNode());

	fSuspended = true;
	return B_OK;
}


template<typename Policy>
status_t
GenericIndexIterator<Policy>::Resume()
{
	if (!fSuspended)
		return B_BAD_VALUE;

	if (fNextTreeNode != NULL)
		fIndex->GetVolume()->RemoveNodeListener(this);

	fSuspended = false;
	return B_OK;
}


template<typename Policy>
bool
GenericIndexIterator<Policy>::SetTo(Index* index, const Value& value,
	bool ignoreValue)
{
	Resume();

	fIndex = index;
	fSuspended = false;
	fNextTreeNode = NULL;

	if (fIndex == NULL)
		return false;

	typename NodeTree::Iterator iterator;
	if (ignoreValue) {
		Policy::GetNodeTree(fIndex)->GetIterator(&iterator);
	} else if (Policy::GetNodeTree(fIndex)->FindFirstClosest(value, false,
			&iterator) == NULL) {
		return false;
	}

	fNextTreeNode = iterator.CurrentNode();
	return fNextTreeNode != NULL;
}


/*!	Moves the iterator temporarily off the current node.
	Called when the node the iterator currently points to has been modified and
	the index is about to remove it from and reinsert it into the tree. After
	having done that NodeChangeEnd() must be called.
*/
template<typename Policy>
void
GenericIndexIterator<Policy>::NodeChangeBegin(Node* node)
{
	fNextTreeNode = Policy::GetNodeTree(fIndex)->Previous(fNextTreeNode);
}


/*!	Brackets a NodeChangeBegin() call.
*/
template<typename Policy>
void
GenericIndexIterator<Policy>::NodeChangeEnd(Node* node)
{
	if (fNextTreeNode != NULL) {
		fNextTreeNode = Policy::GetNodeTree(fIndex)->Next(fNextTreeNode);
	} else {
		typename NodeTree::Iterator iterator;
		Policy::GetNodeTree(fIndex)->GetIterator(&iterator);
		fNextTreeNode = iterator.CurrentNode();
	}

	// If the node is no longer the one we originally pointed to, re-register
	// the node listener.
	if (fNextTreeNode == NULL) {
		fIndex->GetVolume()->RemoveNodeListener(this);
	} else {
		Node* newNode = _ToNode();
		if (newNode != node) {
			fIndex->GetVolume()->RemoveNodeListener(this);
			fIndex->GetVolume()->AddNodeListener(this, newNode);
		}
	}
}


template<typename Policy>
void
GenericIndexIterator<Policy>::NodeRemoved(Node* node)
{
	Resume();
	Next(NULL, NULL);
	Suspend();
}


template<typename Policy>
Node*
GenericIndexIterator<Policy>::_ToNode() const
{
//	return NodeTree::NodeStrategy().GetValue(fNextTreeNode);
	typename NodeTree::NodeStrategy strategy;
	return strategy.GetValue(fNextTreeNode);
}


#endif	// INDEX_IMPL_H
