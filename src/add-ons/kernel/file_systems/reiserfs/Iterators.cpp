// Iterators.h
//
// Copyright (c) 2003-2010, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.


#include "Iterators.h"

#include "Block.h"
#include "BlockCache.h"
#include "Key.h"
#include "IndirectItem.h"
#include "StatItem.h"
#include "Tree.h"


// min and max
// We don't want to include <algobase.h> otherwise we also get <iostream.h>
// and other undesired things.
template<typename C>
static inline C min(const C &a, const C &b) { return (a < b ? a : b); }
template<typename C>
static inline C max(const C &a, const C &b) { return (a > b ? a : b); }

/*!
	\class TreePath
	\brief Represents a path in the tree.

	It is composed of TreePath::Elements each one storing the block number
	of a node and the index of a child node.
*/

// constructor
TreePath::TreePath(uint32 maxLength)
	: fMaxLength(maxLength),
	  fLength(0),
	  fElements(NULL)
{
	fElements = new(nothrow) Element[fMaxLength];
}

// destructor
TreePath::~TreePath()
{
	if (fElements)
		delete[] fElements;
}

// InitCheck
status_t
TreePath::InitCheck() const
{
	return (fElements ? B_OK : B_NO_MEMORY);
}

// GetMaxLength
uint32
TreePath::GetMaxLength() const
{
	return fMaxLength;
}

// GetLength
uint32
TreePath::GetLength() const
{
	return fLength;
}

// ElementAt
const TreePath::Element *
TreePath::ElementAt(int32 index) const
{
	Element *element = NULL;
	if (InitCheck() == B_OK && index >= 0 && (uint32)index < GetLength())
		element = fElements + index;
	return element;
}

// GetTopElement
const TreePath::Element *
TreePath::GetTopElement() const
{
	return ElementAt(GetLength() - 1);
}

// PushElement
status_t
TreePath::PushElement(uint64 blockNumber, int32 index)
{
	status_t error = (fLength < fMaxLength ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK) {
		error = fElements[fLength].SetTo(blockNumber, index);
		if (error == B_OK)
			fLength++;
	}
	return error;
}

// PopElement
status_t
TreePath::PopElement()
{
	status_t error = InitCheck();
	if (error == B_OK) {
		if (fLength > 0)
			fLength--;
		else
			error = B_ERROR;
	}
	return error;
}


/*!
	\class TreePath::Element
	\brief Store information about one element in a tree path.
*/

// SetTo
status_t
TreePath::Element::SetTo(uint64 blockNumber, int32 index)
{
	fBlockNumber = blockNumber;
	fIndex = index;
	return B_OK;
}

// GetBlockNumber
uint64
TreePath::Element::GetBlockNumber() const
{
	return fBlockNumber;
}

// GetIndex
int32
TreePath::Element::GetIndex() const
{
	return fIndex;
}


/*!
	\class TreeIterator
	\brief Class used to iterate and navigate in trees.

	A TreeIterator is usually initialized to the root node of the tree
	and can be used to navigate and search in the tree.
*/

// constructor
TreeIterator::TreeIterator(Tree *tree)
	: fTree(NULL),
	  fCurrentNode(NULL),
	  fPath(NULL)
{
	SetTo(tree);
}

// destructor
TreeIterator::~TreeIterator()
{
	Unset();
}

// SetTo
status_t
TreeIterator::SetTo(Tree *tree)
{
	Unset();
	fTree = tree;
	fCurrentNode = NULL;
	fPath = NULL;
	if (fTree) {
		fCurrentNode = fTree->GetRootNode();
		if (fCurrentNode)
			fCurrentNode->Get();
		fPath = new(nothrow) TreePath(tree->GetTreeHeight());
	}
	return InitCheck();
}

// Unset
void
TreeIterator::Unset()
{
	if (fPath) {
		delete fPath;
		fPath = NULL;
	}
	if (fCurrentNode) {
		fCurrentNode->Put();
		fCurrentNode = NULL;
	}
}

// InitCheck
status_t
TreeIterator::InitCheck() const
{
	return (fTree && fCurrentNode && fPath ? fPath->InitCheck() : B_NO_INIT);
}

// GetNode
Node *
TreeIterator::GetNode() const
{
	return fCurrentNode;
}

// GetLevel
int32
TreeIterator::GetLevel() const
{
	int32 level = 0;
	if (fCurrentNode)
		level = fCurrentNode->GetLevel();
	return level;
}

// GoTo
/*!	\brief Goes into a given direction.

	\a direction can be
	- \c FORWARD: Forward/to the right. Goes to the next child node of the
	  current node.
	- \c BACKWARDS: Forward/to the right. Goes to the previous child node of
	  the current node.
	- \c UP: Up one level (in root direction). Goes to the parent node of
	  the current node. The current node is the child node, it is pointed
	  to afterward.
	- \c DOWN: Down one level (in leaf direction). Goes to the child node of
	  the current node the iterator currently points at. Points afterwards to
	  the 0th child node of the new current node (unless it is a leaf node).

	\c FORWARD and \c BACKWARDS do not change the current node!

	\param direction \c FORWARD, \c BACKWARDS, \c UP or \c DOWN
	\return \c B_OK, if everything went fine
*/
status_t
TreeIterator::GoTo(uint32 direction)
{
	status_t error = InitCheck();
	if (error == B_OK) {
		switch (direction) {
			case FORWARD:
			{
				if (fCurrentNode->IsInternal()
					&& fIndex < fCurrentNode->CountItems()) {
					fIndex++;
				} else {
					error = B_ENTRY_NOT_FOUND;
				}
				break;
			}
			case BACKWARDS:
			{
				if (fCurrentNode->IsInternal() && fIndex > 0)
					fIndex--;
				else
					error = B_ENTRY_NOT_FOUND;
				break;
			}
			case UP:
			{
				error = _PopTopNode();
				break;
			}
			case DOWN:
			{
				if (fCurrentNode->IsInternal()) {
					InternalNode *internal = fCurrentNode->ToInternalNode();
					const DiskChild *child = internal->ChildAt(fIndex);
					if (child) {
						Node *node = NULL;
						error = fTree->GetNode(child->GetBlockNumber(), &node);
						if (error == B_OK)
							error = _PushCurrentNode(node, 0);
					} else
						error = B_ENTRY_NOT_FOUND;
				} else
					error = B_ENTRY_NOT_FOUND;
				break;
			}
		}
	}
	return error;
}

// GoToPreviousLeaf
/*!	\brief Goes to the previous leaf node.

	This method operates only at leaf node level. It finds the next leaf
	node to the left. Fails, if the current node is no leaf node.

	\param node Pointer to a pre-allocated LeafNode pointer that shall be set
		   to the found node.
	\return \c B_OK, if everything went fine
*/
status_t
TreeIterator::GoToPreviousLeaf(LeafNode **node)
{
	status_t error = InitCheck();
	if (error == B_OK) {
		if (fCurrentNode->IsLeaf()) {
			// find downmost branch on our path, that leads further left
			bool found = false;
			while (error == B_OK && !found) {
				error = GoTo(UP);
				if (error == B_OK)
					found = (GoTo(BACKWARDS) == B_OK);
			}
			// descend the branch to the rightmost leaf
			if (error == B_OK) {
				// one level down
				error = GoTo(DOWN);
				// then keep right
				while (error == B_OK && fCurrentNode->IsInternal()) {
					fIndex = fCurrentNode->CountItems();
					error = GoTo(DOWN);
				}
			}
			// set the result
			if (error == B_OK && node)
				*node = fCurrentNode->ToLeafNode();
		} else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GoToNextLeaf
/*!	\brief Goes to the next leaf node.

	This method operates only at leaf node level. It finds the next leaf
	node to the right. Fails, if the current node is no leaf node.

	\param node Pointer to a pre-allocated LeafNode pointer that shall be set
		   to the found node.
	\return \c B_OK, if everything went fine
*/
status_t
TreeIterator::GoToNextLeaf(LeafNode **node)
{
	status_t error = InitCheck();
	if (error == B_OK) {
		if (fCurrentNode->IsLeaf()) {
			// find downmost branch on our path, that leads further right
			bool found = false;
			while (error == B_OK && !found) {
				error = GoTo(UP);
				if (error == B_OK)
					found = (GoTo(FORWARD) == B_OK);
			}
			// descend the branch to the leftmost leaf
			while (error == B_OK && fCurrentNode->IsInternal())
				error = GoTo(DOWN);
			// set the result
			if (error == B_OK && node)
				*node = fCurrentNode->ToLeafNode();
		} else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// FindRightMostLeaf
/*!	\brief Finds the rightmost leaf node that may contain the supplied key.

	The method starts at the current node and only goes downwards.
	In the spanned subtree it finds the rightmost leaf node whose left
	delimiting key is not greater than the supplied key.

	\param key The key to be found.
	\param node Pointer to a pre-allocated LeafNode pointer that shall be set
		   to the found node.
	\return \c B_OK, if everything went fine.
*/
status_t
TreeIterator::FindRightMostLeaf(const VKey *k, LeafNode **node)
{
//printf("TreeIterator::FindRightMostLeaf()\n");
	status_t error = (k ? InitCheck() : B_BAD_VALUE);
	while (error == B_OK && fCurrentNode->IsInternal()) {
		int32 index = 0;
		error = _SearchRightMost(fCurrentNode->ToInternalNode(), k, &index);
		if (error == B_OK) {
			fIndex = index;
			error = GoTo(DOWN);
		}
	}
//if (error == B_OK)
//printf("found node: index: %ld\n", fIndex);
	// set the result
	if (error == B_OK && node)
		*node = fCurrentNode->ToLeafNode();
//printf("TreeIterator::FindRightMostLeaf() done: %s\n", strerror(error));
	return error;
}

// Suspend
/*!	\brief Suspends the iterator.

	This means, the current node is Put() and its block number and child node
	index pushed onto the tree path. This should be done, when the iteration
	is not to be continued immediately (or for the foreseeable future) and
	the node block shall not be locked in memory for that time.

	Resume() is to be called, before the iteration can be continued.

	\return \c B_OK, if everything went fine.
*/
status_t
TreeIterator::Suspend()
{
	status_t error = InitCheck();
	if (error == B_OK)
		error = _PushCurrentNode();
	if (error == B_OK)
		fCurrentNode = NULL;
	return error;
}

// Resume
/*!	\brief Resumes the iteration.

	To be called after a Suspend(), before the iteration can be continued.

	\return \c B_OK, if everything went fine.
*/
status_t
TreeIterator::Resume()
{
	status_t error
		= (fTree && !fCurrentNode && fPath ? fPath->InitCheck() : B_NO_INIT);
	if (error == B_OK)
		error = _PopTopNode();
	return error;
}

// _PushCurrentNode
status_t
TreeIterator::_PushCurrentNode(Node *newTopNode, int32 newIndex)
{
	status_t error = fPath->PushElement(fCurrentNode->GetNumber(), fIndex);
	if (error == B_OK) {
		fCurrentNode->Put();
		fCurrentNode = newTopNode;
		fIndex = newIndex;
	}
	return error;
}

// _PopTopNode
status_t
TreeIterator::_PopTopNode()
{
	status_t error = B_OK;
	if (fPath->GetLength() > 0) {
		const TreePath::Element *element = fPath->GetTopElement();
		Node *node = NULL;
		error = fTree->GetNode(element->GetBlockNumber(), &node);
		if (error == B_OK) {
			if (fCurrentNode)
				fCurrentNode->Put();
			fCurrentNode = node;
			fIndex = element->GetIndex();
			fPath->PopElement();
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// _SearchRightMost
status_t
TreeIterator::_SearchRightMost(InternalNode *node, const VKey *k, int32 *index)
{
//FUNCTION_START();
	// find the last node that may contain the key, i.e. the node with the
	// greatest index whose left delimiting key is less or equal the given key
	status_t error = (node && k && index ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
//PRINT(("  searching: "));
//k->Dump();
		// binary search: lower and upper are node indices, mid is a key index
		int32 lower = 0;
		int32 upper = node->CountItems();
		while (lower < upper) {
			int32 mid = (lower + upper) / 2;	// lower <= mid < upper <= count
			VKey midKey(node->KeyAt(mid));
//PRINT(("  mid: %3ld: ", mid));
//midKey.Dump();
			if (*k < midKey)			// => node index <= mid
				upper = mid;					// lower <= upper < count
			else								// => node index > mid
				lower = mid + 1;				// lower <= upper <= count
//PRINT(("  lower: %ld, upper: %ld\n", lower, upper));
		}
		if (error == B_OK)
			*index = lower;
	}
//RETURN_ERROR(error);
	return error;
}


/*!
	\class ItemIterator
	\brief Class used to iterate through leaf node items.

	A ItemIterator is usually initialized to the root node of the tree,
	and before it can be used for iteration, FindRightMostClose() or
	FindRightMost() must be invoked. They set the iterator to an item
	and afterwards one can use GoToPrevious() and GoToNext() to get the
	previous/next ones.
*/

// constructor
ItemIterator::ItemIterator(Tree *tree)
	: fTreeIterator(tree),
	  fIndex(0)
{
}

// SetTo
status_t
ItemIterator::SetTo(Tree *tree)
{
	status_t error = fTreeIterator.SetTo(tree);
	fIndex = 0;
	return error;
}

// InitCheck
status_t
ItemIterator::InitCheck() const
{
	return fTreeIterator.InitCheck();
}

// GetCurrent
status_t
ItemIterator::GetCurrent(Item *item)
{
	LeafNode *node = NULL;
	status_t error = (item ? _GetLeafNode(&node) : B_BAD_VALUE);
	if (error == B_OK) {
		if (fIndex >= 0 && fIndex < node->CountItems())
			error = item->SetTo(node, fIndex);
		else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GoToPrevious
status_t
ItemIterator::GoToPrevious(Item *item)
{
	LeafNode *node = NULL;
	status_t error = _GetLeafNode(&node);
	if (error == B_OK) {
		// get the leaf node on which the next item resides
		int32 newIndex = fIndex - 1;
		while (error == B_OK
			   && (newIndex < 0 || newIndex >= node->CountItems())) {
			error = fTreeIterator.GoToPreviousLeaf(&node);
			if (error == B_OK)
				newIndex = node->CountItems() - 1;
		}
		// got the node, get the item
		if (error == B_OK) {
			fIndex = newIndex;
			if (item)
				error = item->SetTo(node, fIndex);
		}
	}
	return error;
}

// GoToNext
status_t
ItemIterator::GoToNext(Item *item)
{
//PRINT(("ItemIterator::GoToNext()\n"));
	LeafNode *node = NULL;
	status_t error = _GetLeafNode(&node);
	if (error == B_OK) {
//PRINT(("  leaf node: %Ld\n", node->GetNumber()));
		// get the leaf node on which the next item resides
		int32 newIndex = fIndex + 1;
		while (error == B_OK && newIndex >= node->CountItems()) {
			error = fTreeIterator.GoToNextLeaf(&node);
			newIndex = 0;
		}
		// got the node, get the item
		if (error == B_OK) {
//PRINT(("  leaf node now: %Ld\n", node->GetNumber()));
			fIndex = newIndex;
//PRINT(("  index now: %ld\n", fIndex));
			if (item)
				error = item->SetTo(node, fIndex);
		}
	}
//PRINT(("ItemIterator::GoToNext() done: %s\n", strerror(error)));
	return error;
}

// FindRightMostClose
/*!	\brief Finds the rightmost item that may contain the supplied key.

	The method finds the rightmost item whose left delimiting key is not
	greater than the supplied key.

	\param key The key to be found.
	\param item Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\return \c B_OK, if everything went fine.
*/
status_t
ItemIterator::FindRightMostClose(const VKey *k, Item *item)
{
//printf("ItemIterator::FindRightMostClose()\n");
	status_t error = (k ? InitCheck() : B_BAD_VALUE);
	// find rightmost leaf node with the given key
	if (error == B_OK)
		error = fTreeIterator.FindRightMostLeaf(k);
	// search the node for the key
	if (error == B_OK) {
		LeafNode *node = fTreeIterator.GetNode()->ToLeafNode();
		error = _SearchRightMost(node, k, &fIndex);
		if (error == B_OK && item)
{
			error = item->SetTo(node, fIndex);
//VKey itemKey;
//printf("  found item: ");
//item->GetKey(&itemKey)->Dump();
}
	}
//printf("ItemIterator::FindRightMostClose() done: %s\n", strerror(error));
	return error;
}

// FindRightMost
/*!	\brief Finds the rightmost item that starts with the supplied key.

	The method finds the rightmost item whose left delimiting key is equal
	to the supplied key.

	\param key The key to be found.
	\param item Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\return \c B_OK, if everything went fine.
*/
status_t
ItemIterator::FindRightMost(const VKey *k, Item *item)
{
//TFUNCTION_START();
	// find the first item with a greater or equal key, and check whether the
	// key is equal
	Item closeItem;
	status_t error = FindRightMostClose(k, &closeItem);
//REPORT_ERROR(error);
	if (error == B_OK) {
		VKey itemKey;
		closeItem.GetHeader()->GetKey(&itemKey);
		if (*k == itemKey) {
			if (item)
				*item = closeItem;
		} else
{
//PRINT(("keys not equal: dirID: %lu, objectID: %lu, offset: %Lu, type: %hu\n",
//itemKey.GetDirID(), itemKey.GetObjectID(), itemKey.GetOffset(), itemKey.GetType()));
			error = B_ENTRY_NOT_FOUND;
}
	}
//REPORT_ERROR(error);
	return error;
}

// Suspend
/*! \brief Suspends the iterator.
	\see TreeIterator::Suspend().
	\return \c B_OK, if everything went fine.
*/
status_t
ItemIterator::Suspend()
{
	return fTreeIterator.Suspend();
}

// Resume
/*! \brief Resumes the iteration.
	\see TreeIterator::Resume().
	\return \c B_OK, if everything went fine.
*/
status_t
ItemIterator::Resume()
{
	return fTreeIterator.Resume();
}

// _GetLeafNode
status_t
ItemIterator::_GetLeafNode(LeafNode **leafNode) const
{
	status_t error = InitCheck();
	if (error == B_OK) {
		if (Node *node = fTreeIterator.GetNode()) {
			if (node->IsLeaf())
				*leafNode = node->ToLeafNode();
			else
				error = B_ENTRY_NOT_FOUND;
		} else
			error = B_ERROR;
	}
	return error;
}

// _SearchRightMost
status_t
ItemIterator::_SearchRightMost(LeafNode *node, const VKey *k,
							   int32 *index) const
{
//FUNCTION_START();
	status_t error = (node && k && index ? B_OK : B_BAD_VALUE);
	// find the first item with a less or equal key
	if (error == B_OK) {
//PRINT(("  searching: "));
//k->Dump();
/*
		// simple linear search backwards
		int32 foundIndex = -1;
		for (int32 i = node->CountItems() - 1; foundIndex < 0 && i >= 0; i--) {
			VKey itemKey;
			node->ItemHeaderAt(i)->GetKey(&itemKey);
			if (itemKey <= *k) {
				foundIndex = i;
				error = B_OK;
				break;
			}
		}
		// check whether all item keys are greater
		if (foundIndex < 0)
			error = B_ENTRY_NOT_FOUND;
		// set result
		if (error == B_OK)
			*index = foundIndex;
*/
		// binary search
		// check lower bound first
		VKey lowerKey;
		node->ItemHeaderAt(0)->GetKey(&lowerKey);
		if (lowerKey <= *k) {
			// pre-conditions hold
			int32 lower = 0;
			int32 upper = node->CountItems() - 1;
			while (lower < upper) {
				int32 mid = (lower + upper + 1) / 2;
				VKey midKey;
				node->ItemHeaderAt(mid)->GetKey(&midKey);
//PRINT(("  mid: %3ld: ", mid));
//midKey.Dump();
				if (midKey > *k)
					upper = mid - 1;
				else
					lower = mid;
//PRINT(("  lower: %ld, upper: %ld\n", lower, upper));
			}
			*index = lower;
		} else
			error = B_ENTRY_NOT_FOUND;
	}
//RETURN_ERROR(error);
	return error;
}


/*!
	\class ObjectItemIterator
	\brief Class used to iterate through leaf node items.

	This class basically wraps the ItemIterator class and provides a
	more convenient interface for iteration through items of one given
	object (directory, link or file). It finds only items of the object
	identified by the dir and object ID specified on initialization.
*/

// constructor
/*!	\brief Creates and initializes a ObjectItemIterator.

	The iterator is initialized to find only items belonging to the object
	identified by \a dirID and \a objectID. \a startOffset specifies the
	offset (key offset) at which the iteration shall begin.

	Note, that offsets don't need to be unique for an object, at least not
	for files -- all indirect (and direct?) items have the same offset (1).
	This has the ugly side effect, when iterating forward there may be items
	with the same offset on previous nodes that will be skipped at the
	beginning. The GetNext() does always find the item on the rightmost
	possible node. Therefore searching forward starting with FIRST_ITEM_OFFSET
	doesn't work for files!

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
	\param startOffset The offset at which to begin the iteration.
*/
ObjectItemIterator::ObjectItemIterator(Tree *tree, uint32 dirID,
									   uint32 objectID, uint64 startOffset)
	: fItemIterator(tree),
	  fDirID(dirID),
	  fObjectID(objectID),
	  fOffset(startOffset),
	  fFindFirst(true),
	  fDone(false)
{
}

// SetTo
/*!	\brief Re-initializes a ObjectItemIterator.

	The iterator is initialized to find only items belonging to the object
	identified by \a dirID and \a objectID. \a startOffset specifies the
	offset (key offset) at which the iteration shall begin.

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
	\param startOffset The offset at which to begin the iteration.
	\return \c B_OK, if everything went fine.
*/
status_t
ObjectItemIterator::SetTo(Tree *tree, uint32 dirID, uint32 objectID,
						  uint64 startOffset)
{
	status_t error = fItemIterator.SetTo(tree);
	fDirID = dirID;
	fObjectID = objectID;
	fOffset = startOffset;
	fFindFirst = true;
	fDone = false;
	return error;
}

// InitCheck
status_t
ObjectItemIterator::InitCheck() const
{
	return fItemIterator.InitCheck();
}

// GetNext
/*!	\brief Returns the next item belonging to the object.

	Note, that offsets don't need to be unique for an object, at least not
	for files -- all indirect (and direct?) items have the same offset (1).
	This has the ugly side effect, when iterating forward there may be items
	with the same offset on previous nodes that will be skipped at the
	beginning. The GetNext() does always find the item on the rightmost
	possible node. Therefore searching forward starting with FIRST_ITEM_OFFSET
	doesn't work for files!

	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
ObjectItemIterator::GetNext(Item *foundItem)
{
//PRINT(("ObjectItemIterator::GetNext(): fDirID: %lu, fObjectID: %lu\n", fDirID, fObjectID));
	status_t error = (foundItem ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK && fDone)
		error = B_ENTRY_NOT_FOUND;
	if (error == B_OK) {
		// get the next item
		Item item;
		if (fFindFirst) {
			// first invocation: find the item with the given IDs and offset
			VKey k(fDirID, fObjectID, fOffset, 0, KEY_FORMAT_3_5);
			error = fItemIterator.FindRightMostClose(&k, &item);
			fFindFirst = false;
		} else {
			// item iterator positioned, get the next item
			error = fItemIterator.GoToNext(&item);
//REPORT_ERROR(error);
		}
		// check whether the item belongs to our object
		if (error == B_OK) {
			VKey itemKey;
			if (item.GetDirID() == fDirID && item.GetObjectID() == fObjectID)
				*foundItem = item;
			else
{
//PRINT(("  found item for another object: (%lu, %lu)\n", item.GetDirID(), item.GetObjectID()));
				error = B_ENTRY_NOT_FOUND;
}
		}
		if (error != B_OK)
			fDone = true;
	}
//	return error;
RETURN_ERROR(error)
}

// GetNext
/*!	\brief Returns the next item belonging to the object.

	This version finds only items of the specified type.

	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\param type The type the found item must have.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
ObjectItemIterator::GetNext(Item *foundItem, uint32 type)
{
	status_t error = B_OK;
	do {
		error = GetNext(foundItem);
	} while (error == B_OK && foundItem->GetType() != type);
	return error;
}

// GetPrevious
/*!	\brief Returns the previous item belonging to the object.
	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
ObjectItemIterator::GetPrevious(Item *foundItem)
{
//PRINT(("ObjectItemIterator::GetPrevious()\n"));
	status_t error = (foundItem ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK && fDone)
		error = B_ENTRY_NOT_FOUND;
	if (error == B_OK) {
		// get the next item
		Item item;
		if (fFindFirst) {
			// first invocation: find the rightmost item of the object
			VKey k(fDirID, fObjectID, fOffset, 0xffffffffUL, KEY_FORMAT_3_5);
			error = fItemIterator.FindRightMostClose(&k, &item);
			fFindFirst = false;
		} else {
			// item iterator positioned, get the previous item
			error = fItemIterator.GoToPrevious(&item);
		}
		// check whether the item belongs to our object
		if (error == B_OK) {
			VKey itemKey;
			if (item.GetDirID() == fDirID && item.GetObjectID() == fObjectID) {
//PRINT(("  found item: %lu, %lu, %Lu\n", fDirID, fObjectID, item.GetOffset()));
				*foundItem = item;
			} else
{
//PRINT(("  item belongs to different object: (%lu, %lu)\n", item.GetDirID(), item.GetObjectID()));
				error = B_ENTRY_NOT_FOUND;
}
		}
		if (error != B_OK)
			fDone = true;
	}
//PRINT(("ObjectItemIterator::GetPrevious() done: %s\n", strerror(error)));
	return error;
}

// GetPrevious
/*!	\brief Returns the previous item belonging to the object.

	This version finds only items of the specified type.

	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\param type The type the found item must have.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
ObjectItemIterator::GetPrevious(Item *foundItem, uint32 type)
{
	status_t error = B_OK;
	do {
		error = GetPrevious(foundItem);
	} while (error == B_OK && foundItem->GetType() != type);
	return error;
}

// Suspend
/*! \brief Suspends the iterator.
	\see ItemIterator::Suspend().
	\return \c B_OK, if everything went fine.
*/
status_t
ObjectItemIterator::Suspend()
{
	return fItemIterator.Suspend();
}

// Resume
/*! \brief Resumes the iteration.
	\see ItemIterator::Resume().
	\return \c B_OK, if everything went fine.
*/
status_t
ObjectItemIterator::Resume()
{
	return fItemIterator.Resume();
}


/*!
	\class DirEntryIterator
	\brief Class used to iterate through DirEntries.
*/

// constructor
/*!	\brief Creates and initializes a DirEntryIterator.

	The iterator is initialized to find only entries belonging to the directory
	identified by \a dirID and \a objectID. \a startOffset specifies the
	offset (key offset) at which the iteration shall begin. If \a fixedHash
	is \c true, only items that have the same hash value as \a startOffset
	are found.

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
	\param startOffset The offset at which to begin the iteration.
	\param fixedHash \c true to find only entries with the same hash as
		   \a startOffset
*/
DirEntryIterator::DirEntryIterator(Tree *tree, uint32 dirID, uint32 objectID,
								   uint64 startOffset, bool fixedHash)
	: fItemIterator(tree, dirID, objectID, startOffset),
	  fDirItem(),
	  fIndex(-1),
	  fFixedHash(fixedHash),
	  fDone(false)
{
}

// SetTo
/*!	\brief Re-initializes a DirEntryIterator.

	The iterator is initialized to find only entries belonging to the directory
	identified by \a dirID and \a objectID. \a startOffset specifies the
	offset (key offset) at which the iteration shall begin. If \a fixedHash
	is \c true, only items that have the same hash value as \a startOffset
	are found.

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
	\param startOffset The offset at which to begin the iteration.
	\param fixedHash \c true to find only entries with the same hash as
		   \a startOffset
*/
status_t
DirEntryIterator::SetTo(Tree *tree, uint32 dirID, uint32 objectID,
						uint64 startOffset, bool fixedHash)
{
	fDirItem.Unset();
	status_t error = fItemIterator.SetTo(tree, dirID, objectID, startOffset);
	fIndex = -1;
	fFixedHash = fixedHash;
	fDone = false;
	return error;
}

// InitCheck
status_t
DirEntryIterator::InitCheck() const
{
	return fItemIterator.InitCheck();
}

// Rewind
/*!	\brief Rewinds the iterator.

	Simply re-initializes the iterator to the parameters of the last
	initialization.

	\return \c B_OK, if everything went fine.
*/
status_t
DirEntryIterator::Rewind()
{
	return SetTo(GetTree(), GetDirID(), GetObjectID(), GetOffset(),
				 fFixedHash);
}

// GetNext
/*!	\brief Returns the next entry belonging to the directory.
	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\param entryIndex Pointer to a pre-allocated int32 that shall be set
		   to the found entry index.
	\param _entry Pointer to a pre-allocated DirEntry pointer that shall be set
		   to the found entry. May be \c NULL.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
DirEntryIterator::GetNext(DirItem *foundItem, int32 *entryIndex,
						  DirEntry **_entry)
{
	status_t error = (foundItem && entryIndex ? InitCheck() : B_BAD_VALUE);
	// get the next DirItem, if necessary
	// the loop skips empty DirItems gracefully
	while (error == B_OK
		   && (fIndex < 0 || fIndex >= fDirItem.GetEntryCount())) {
		error = fItemIterator.GetNext(&fDirItem, TYPE_DIRENTRY);
		if (error == B_OK) {
			if (fDirItem.Check() == B_OK)
				fIndex = 0;
			else	// bad data: skip the item
				fIndex = -1;
		}
	}
	// get the next entry and check whether it has the correct offset
	if (error == B_OK) {
		DirEntry *entry = fDirItem.EntryAt(fIndex);
		if (!fFixedHash
			|| offset_hash_value(entry->GetOffset())
			   == offset_hash_value(GetOffset())) {
			*foundItem = fDirItem;
			*entryIndex = fIndex;
			if (_entry)
				*_entry = entry;
			fIndex++;
		} else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetPrevious
/*!	\brief Returns the previous entry belonging to the directory.
	\param foundItem Pointer to a pre-allocated Item that shall be set
		   to the found item.
	\param entryIndex Pointer to a pre-allocated int32 that shall be set
		   to the found entry index.
	\param _entry Pointer to a pre-allocated DirEntry pointer that shall be set
		   to the found entry. May be \c NULL.
	\return \c B_OK, if everything went fine, \c B_ENTRY_NOT_FOUND, if we're
			through.
*/
status_t
DirEntryIterator::GetPrevious(DirItem *foundItem, int32 *entryIndex,
							  DirEntry **_entry)
{
//printf("DirEntryIterator::GetPrevious()\n");
	status_t error = (foundItem && entryIndex ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK && fDone)
		error = B_ENTRY_NOT_FOUND;
	// get the next DirItem, if necessary
	// the loop skips empty DirItems gracefully
	while (error == B_OK
		   && (fIndex < 0 || fIndex >= fDirItem.GetEntryCount())) {
		error = fItemIterator.GetPrevious(&fDirItem, TYPE_DIRENTRY);
		if (error == B_OK) {
			if (fDirItem.Check() == B_OK)
				fIndex = fDirItem.GetEntryCount() - 1;
			else	// bad data: skip the item
				fIndex = -1;
		}
	}
//printf("  found dir item: %s\n", strerror(error));
	// skip entries with a greater offset
	while (error == B_OK && fIndex >= 0
		   && fDirItem.EntryAt(fIndex)->GetOffset() > GetOffset()) {
//printf("  skipping entry %ld: offset %lu\n", fIndex, fDirItem.EntryAt(fIndex)->GetOffset());
		fIndex--;
	}
	// get the entry and check whether it has the correct offset
	if (error == B_OK) {
//printf("  entries with greater offsets skipped: index: %ld\n", fIndex);
		if (fIndex >= 0
//&& (printf("  entry index %ld: offset %lu\n", fIndex, fDirItem.EntryAt(fIndex)->GetOffset()), true)
			&& (!fFixedHash
				|| offset_hash_value(fDirItem.EntryAt(fIndex)->GetOffset())
				   == offset_hash_value(GetOffset()))) {
//printf("  entry found\n");
			DirEntry *entry = fDirItem.EntryAt(fIndex);
			*foundItem = fDirItem;
			*entryIndex = fIndex;
			fDone = (fFixedHash
					 && offset_generation_number(entry->GetOffset()) == 0);
			if (_entry)
				*_entry = entry;
			fIndex--;
		} else
			error = B_ENTRY_NOT_FOUND;
	}
//printf("DirEntryIterator::GetPrevious() done: %s\n", strerror(error));
	return error;
}

// Suspend
/*! \brief Suspends the iterator.
	\see ObjectItemIterator::Suspend().
	\return \c B_OK, if everything went fine.
*/
status_t
DirEntryIterator::Suspend()
{
	return fItemIterator.Suspend();
}

// Resume
/*! \brief Resumes the iteration.
	\see ObjectItemIterator::Resume().
	\return \c B_OK, if everything went fine.
*/
status_t
DirEntryIterator::Resume()
{
	return fItemIterator.Resume();
}


/*!
	\class StreamReader
	\brief Class used to read object streams (files, links).
*/

// constructor
/*!	\brief Creates and initializes a StreamReader.

	The reader is initialized to read the stream of the object
	identified by \a dirID and \a objectID.

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
*/
StreamReader::StreamReader(Tree *tree, uint32 dirID, uint32 objectID)
	: fItemIterator(tree, dirID, objectID, SD_OFFSET),
	  fItem(),
	  fStreamSize(-1),
	  fItemOffset(-1),
	  fItemSize(0),
	  fBlockSize(0)
{
	fBlockSize = tree->GetBlockSize();
}

// SetTo
/*!	\brief Creates and initializes a StreamReader.

	The reader is initialized to read the stream of the object
	identified by \a dirID and \a objectID.

	\param tree The tree.
	\param dirID The directory ID of the object.
	\param objectID The object ID of the object.
	\return \c B_OK, if everything went fine.
*/
status_t
StreamReader::SetTo(Tree *tree, uint32 dirID, uint32 objectID)
{
	fItem.Unset();
	status_t error = fItemIterator.SetTo(tree, dirID, objectID, SD_OFFSET);
	fStreamSize = -1;
	fItemOffset = -1;
	fItemSize = 0;
	fBlockSize = tree->GetBlockSize();
	return error;
}

// InitCheck
status_t
StreamReader::InitCheck() const
{
	return fItemIterator.InitCheck();
}

// ReadAt
/*!	\brief Tries to read the specified number of bytes from the stream into
		   the supplied buffer.
	\param position Stream position at which to start reading.
	\param buffer Pointer to a pre-allocated buffer into which the read data
		   shall be written.
	\param bufferSize Number of bytes to be read.
	\param _bytesRead Pointer to a pre-allocated size_t which shall be set to
		   the number of bytes actually read.
	\return \c B_OK, if everything went fine.
*/
status_t
StreamReader::ReadAt(off_t position, void *buffer, size_t bufferSize,
					 size_t *_bytesRead)
{
//PRINT(("StreamReader::ReadAt(%Ld, %p, %lu)\n", position, buffer, bufferSize));
	status_t error = (position >= 0 && buffer && _bytesRead ? InitCheck()
															: B_BAD_VALUE);
	// get the size of the stream
	if (error == B_OK)
		error = _GetStreamSize();
	// compute the number of bytes that acually have to be read
	if (error == B_OK) {
		if (position < fStreamSize) {
			if (position + bufferSize > fStreamSize)
				bufferSize = fStreamSize - position;
		} else
			bufferSize = 0;
	}
	// do the reading
	if (error == B_OK) {
		size_t bytesRead = 0;
		while (error == B_OK && bufferSize > 0
			   && (error = _SeekTo(position)) == B_OK) {
//PRINT(("  seeked to %Ld: fItemOffset: %Ld, fItemSize: %Ld\n", position,
//fItemOffset, fItemSize));
			off_t inItemOffset = max_c(0LL, position - fItemOffset);
			off_t toRead = min_c(fItemSize - inItemOffset, (off_t)bufferSize);
			switch (fItem.GetType()) {
				case TYPE_INDIRECT:
					error = _ReadIndirectItem(inItemOffset, buffer, toRead);
					break;
				case TYPE_DIRECT:
					error = _ReadDirectItem(inItemOffset, buffer, toRead);
					break;
				case TYPE_STAT_DATA:
				case TYPE_DIRENTRY:
				case TYPE_ANY:
				default:
					FATAL(("Neither direct nor indirect item! type: %u\n",
						   fItem.GetType()));
					error = B_IO_ERROR;
					break;
			}
			if (error == B_OK) {
				buffer = (uint8*)buffer + toRead;
				position += toRead;
				bufferSize -= toRead;
				bytesRead += toRead;
			}
		}
		*_bytesRead = bytesRead;
	}
//if (error == B_OK)
//PRINT(("StreamReader::ReadAt() done: read: %lu bytes\n", *_bytesRead))
//else
//PRINT(("StreamReader::ReadAt() done: %s\n", strerror(error)))
	return error;
}

// Suspend
/*! \brief Suspends the reader.
	\see ObjectItemIterator::Suspend().
	\return \c B_OK, if everything went fine.
*/
status_t
StreamReader::Suspend()
{
	return fItemIterator.Suspend();
}

// Resume
/*! \brief Resumes the reader.
	\see ObjectItemIterator::Resume().
	\return \c B_OK, if everything went fine.
*/
status_t
StreamReader::Resume()
{
	return fItemIterator.Resume();
}

// _GetStreamSize
status_t
StreamReader::_GetStreamSize()
{
	status_t error = B_OK;
	if (fStreamSize < 0) {
		// get the stat item
		error = fItemIterator.GetNext(&fItem, TYPE_STAT_DATA);
		if (error == B_OK) {
			StatData statData;
			error = (static_cast<StatItem*>(&fItem))->GetStatData(&statData);
			if (error == B_OK)
				fStreamSize = statData.GetSize();
		}
	}
	return error;
}

// _SeekTo
status_t
StreamReader::_SeekTo(off_t position)
{
	status_t error = _GetStreamSize();
	if (error == B_OK && fItemOffset < 0)
		fItemOffset = 0;	// prepare for the loop
	if (error == B_OK) {
		if (2 * position < fItemOffset) {
			// seek backwards
			// since the position is closer to the beginning of the file than
			// to the current position, we simply reinit the item iterator
			// and seek forward
			error = fItemIterator.SetTo(GetTree(), GetDirID(), GetObjectID(),
										SD_OFFSET);
			fStreamSize = -1;
			fItemOffset = -1;
			fItemSize = 0;
			if (error == B_OK)
				error = _SeekTo(position);
		} else if (position < fItemOffset) {
			// seek backwards
			// iterate through the items
			while (error == B_OK && position < fItemOffset
				   && (error = fItemIterator.GetPrevious(&fItem)) == B_OK) {
				fItemSize = 0;
				switch (fItem.GetType()) {
					case TYPE_INDIRECT:
					{
						IndirectItem &indirect
							= *static_cast<IndirectItem*>(&fItem);
						// Note, that it is assumed, that the existence of a
						// next item implies, that all blocks are fully used.
						// This is safe, since otherwise we would have never
						// come to the next item.
						fItemSize = indirect.CountBlocks() * (off_t)fBlockSize;
						break;
					}
					case TYPE_DIRECT:
						// See the comment for indirect items.
						fItemSize = fItem.GetLen();
						break;
					case TYPE_STAT_DATA:
					case TYPE_DIRENTRY:
					case TYPE_ANY:
					default:
						// simply skip items of other kinds
						break;
				}
				fItemOffset -= fItemSize;
			}
		} else if (position >= fItemOffset + fItemSize) {
			// seek forward
			// iterate through the items
			while (error == B_OK && position >= fItemOffset + fItemSize
				   && (error = fItemIterator.GetNext(&fItem)) == B_OK) {
				fItemOffset += fItemSize;
				fItemSize = 0;
				switch (fItem.GetType()) {
					case TYPE_INDIRECT:
					{
						IndirectItem &indirect
							= *static_cast<IndirectItem*>(&fItem);
						fItemSize = min(indirect.CountBlocks()
											* (off_t)fBlockSize,
										fStreamSize - fItemOffset);
						break;
					}
					case TYPE_DIRECT:
						fItemSize = min((off_t)fItem.GetLen(),
										fStreamSize - fItemOffset);
						break;
					case TYPE_STAT_DATA:
					case TYPE_DIRENTRY:
					case TYPE_ANY:
					default:
						// simply skip items of other kinds
						break;
				}
			}
		}
	}
//	return error;
RETURN_ERROR(error);
}

// _ReadIndirectItem
status_t
StreamReader::_ReadIndirectItem(off_t offset, void *buffer, size_t bufferSize)
{
//PRINT(("StreamReader::_ReadIndirectItem(%Ld, %p, %lu)\n", offset, buffer, bufferSize));
	status_t error = B_OK;
	IndirectItem &indirect = *static_cast<IndirectItem*>(&fItem);
	// skip items until the offset is reached
	uint32 skipItems = 0;
	if (offset > 0) {
		skipItems = uint32(offset / fBlockSize);
		skipItems = min(skipItems, indirect.CountBlocks());	// not necessary
	}
//PRINT(("  skipItems: %lu\n", skipItems));
	for (uint32 i = skipItems;
		 error == B_OK && bufferSize > 0 && i < indirect.CountBlocks();
		 i++) {
//PRINT(("    child %lu\n", i));
		// get the block
		Block *block = NULL;
		error = GetTree()->GetBlock(indirect.BlockNumberAt(i), &block);
		if (error == B_OK) {
			// copy the data into the buffer
			off_t blockOffset = i * (off_t)fBlockSize;
			uint32 localOffset = max_c(0LL, offset - blockOffset);
			uint32 toRead = min_c(fBlockSize - localOffset, bufferSize);
			memcpy(buffer,
				reinterpret_cast<uint8*>(block->GetData()) + localOffset,
				toRead);

			block->Put();
			bufferSize -= toRead;
			buffer = (uint8*)buffer + toRead;
		} else {
			FATAL(("failed to get block %" B_PRIu64 "\n",
				indirect.BlockNumberAt(i)));
			error = B_IO_ERROR;
		}
	}
//PRINT(("StreamReader::_ReadIndirectItem() done: %s\n", strerror(error)))
	return error;
}

// _ReadDirectItem
status_t
StreamReader::_ReadDirectItem(off_t offset, void *buffer, size_t bufferSize)
{
//PRINT(("StreamReader::_ReadDirectItem(%Ld, %p, %lu)\n", offset, buffer, bufferSize));
	// copy the data into the buffer
	memcpy(buffer,
		reinterpret_cast<uint8*>(fItem.GetData()) + offset, bufferSize);
	return B_OK;
}

