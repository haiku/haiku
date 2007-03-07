// AVLTree.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <new>

#include <stdio.h>

#include <OS.h>

#include "Misc.h"

using std::nothrow;

// maximal height of a tree
static const int kMaxAVLTreeHeight = 32;

// AVLTreeStandardCompare
template<typename Value>
class AVLTreeStandardCompare
{
public:
	inline int operator()(const Value &a, const Value &b) const
	{
		if (a < b)
			return -1;
		else if (a > b)
			return 1;
		return 0;
	}
};

// AVLTreeStandardGetKey
template<typename Value, typename Key>
class AVLTreeStandardGetKey
{
public:
	inline const Key &operator()(const Value &a) const
	{
		return a;
	}

	inline Key &operator()(Value &a) const
	{
		return a;
	}
};

// AVLTreeStandardNode
template<typename Value>
struct AVLTreeStandardNode {
	AVLTreeStandardNode(const Value &a)
		: value(a),
		  parent(NULL),
		  left(NULL),
		  right(NULL),
		  balance_factor(0)
	{
	}

	Value						value;
	AVLTreeStandardNode<Value>	*parent;
	AVLTreeStandardNode<Value>	*left;
	AVLTreeStandardNode<Value>	*right;
	int							balance_factor;
};

// AVLTreeStandardNodeAllocator
template<typename Value, typename Node>
class AVLTreeStandardNodeAllocator
{
public:
	inline Node *Allocate(const Value &a) const
	{
		return new(nothrow) AVLTreeStandardNode<Value>(a);
	}

	inline void Free(Node *node) const
	{
		delete node;
	}
};

// AVLTreeStandardGetValue
template<typename Value, typename Node>
class AVLTreeStandardGetValue
{
public:
	inline Value &operator()(Node *node) const
	{
		return node->value;
	}
};

// for convenience
#define AVL_TREE_TEMPLATE_LIST template<typename Value, typename Key, \
	typename Node, typename KeyCompare, typename GetKey, \
	typename NodeAllocator, typename GetValue>
#define AVL_TREE_CLASS_NAME AVLTree<Value, Key, Node, KeyCompare, GetKey, \
									NodeAllocator, GetValue>

// AVLTree
template<typename Value, typename Key = Value,
		 typename Node = AVLTreeStandardNode<Value>,
		 typename KeyCompare = AVLTreeStandardCompare<Key>,
		 typename GetKey = AVLTreeStandardGetKey<Value, Key>,
		 typename NodeAllocator = AVLTreeStandardNodeAllocator<Value, Node>,
		 typename GetValue = AVLTreeStandardGetValue<Value, Node> >
class AVLTree {
public:
	class Iterator;

public:
// The Node parameter must implement this interface.
//	struct Node {
//		Node	*parent;
//		Node	*left;
//		Node	*right;
//		int		balance_factor;
//	};

	AVLTree();
	AVLTree(const KeyCompare &keyCompare, const GetKey &getKey,
			const NodeAllocator &allocator, const GetValue &getValue);
	~AVLTree();

	inline int CountItems() const	{ return fNodeCount; }

	Value *Find(const Key &key, Iterator *iterator = NULL);
	Value *FindClose(const Key &key, bool less, Iterator *iterator = NULL);
	void GetIterator(Iterator *iterator, bool reverse = false);

	status_t Insert(const Value &value, Iterator *iterator = NULL);
	status_t Remove(const Key &key);
	void Remove(Iterator &iterator);

	// debugging
	int Check(Node *node = NULL, int level = 0, bool levelsOnly = false) const;

protected:
	enum {
		NOT_FOUND		= -3,
		DUPLICATE		= -2,
		NO_MEMORY		= -1,
		OK				= 0,
		HEIGHT_CHANGED	= 1,

		LEFT			= -1,
		BALANCED		= 0,
		RIGHT			= 1,
	};

	// rotations
	void _RotateRight(Node **nodeP);
	void _RotateLeft(Node **nodeP);

	// insert
	int _BalanceInsertLeft(Node **node);
	int _BalanceInsertRight(Node **node);
	int _Insert(const Value &value, Node **node, Iterator *iterator);

	// remove
	int _BalanceRemoveLeft(Node **node);
	int _BalanceRemoveRight(Node **node);
	int _RemoveRightMostChild(Node **node, Node **foundNode);
	int _Remove(const Key &key, Node **node);
	int _Remove(Node *node);

	void _FreeTree(Node *node);

	// debugging
	void _DumpNode(Node *node) const;

	// iterator support
	inline void _InitIterator(Iterator *iterator, Node *node,
							  bool reverse = false, bool initialize = false);


protected:
	friend class Iterator;

	Node			*fRoot;
	int				fNodeCount;
	KeyCompare		fKeyCompare;
	GetKey			fGetKey;
	NodeAllocator	fAllocator;
	GetValue		fGetValue;
};

// Iterator
AVL_TREE_TEMPLATE_LIST
class AVL_TREE_CLASS_NAME::Iterator {
public:
	Iterator()
		: fTree(NULL),
		  fCurrent(NULL),
		  fReverse(false)
	{
	}

	Iterator(AVL_TREE_CLASS_NAME *tree)
		: fTree(NULL),
		  fCurrent(NULL),
		  fReverse(false)
	{
		if (tree)
			tree->GetIterator(this);
	}

	~Iterator()
	{
	}

	inline Value *GetCurrent()
	{
		return (fTree && fCurrent ? &fTree->fGetValue(fCurrent) : NULL);
	}

	inline Value *GetPrevious()
	{
		if (fReverse)
			fCurrent = _GetNextNode(fCurrent);
		else
			fCurrent = _GetPreviousNode(fCurrent);
		return GetCurrent();
	}

	inline Value *GetNext()
	{
		if (fReverse)
			fCurrent = _GetPreviousNode(fCurrent);
		else
			fCurrent = _GetNextNode(fCurrent);
		return GetCurrent();
	}

	inline void Remove()
	{
		if (fTree)
			fTree->Remove(*this);
	}

	inline void SetReverse(bool reverse)
	{
		fReverse = reverse;
	}

private:
	friend class AVL_TREE_CLASS_NAME;

	Iterator(const Iterator&);
	Iterator & operator=(const Iterator&);

private:
	inline void _SetTo(AVL_TREE_CLASS_NAME *tree, Node *node,
					   bool reverse = false, bool initialize = false)
	{
		fTree = tree;
		fCurrent = node;
		fReverse = reverse;
		// initialize to first/last node, if desired
		if (initialize && fTree) {
			fCurrent = fTree->fRoot;
			if (fCurrent) {
				if (fReverse) {
					while (fCurrent->right)
						fCurrent = fCurrent->right;
				} else {
					while (fCurrent->left)
						fCurrent = fCurrent->left;
				}
			}
		}
	}

	static inline Node *_GetPreviousNode(Node *node)
	{
		if (node) {
			// The previous node cannot be in the right subtree.
			if (node->left) {
				// We have a left subtree, so go to the right-most node.
				node = node->left;
				while (node->right)
					node = node->right;
			} else {
				// No left subtree: Backtrack our path and stop, where we
				// took the right branch.
				Node *previous;
				do {
					previous = node;
					node = node->parent;
				} while (node && previous == node->left);
			}
		}
		return node;
	}

	static inline Node *_GetNextNode(Node *node)
	{
		if (node) {
			// The next node cannot be in the left subtree.
			if (node->right) {
				// We have a right subtree, so go to the left-most node.
				node = node->right;
				while (node->left)
					node = node->left;
			} else {
				// No right subtree: Backtrack our path and stop, where we
				// took the left branch.
				Node *previous;
				do {
					previous = node;
					node = node->parent;
				} while (node && previous == node->right);
			}
		}
		return node;
	}

	inline AVL_TREE_CLASS_NAME *_GetList() const	{ return fTree; }
	inline Node *_GetCurrentNode() const			{ return fCurrent; }

private:
	AVL_TREE_CLASS_NAME	*fTree;
	Node				*fCurrent;
	bool				fReverse;
};


// AVLTree

// constructor
AVL_TREE_TEMPLATE_LIST
AVL_TREE_CLASS_NAME::AVLTree()
	: fRoot(NULL),
	  fNodeCount(0)/*,
	  fKeyCompare(),
	  fGetKey(),
	  fAllocator(),
	  fGetValue()*/
{
}

// constructor
AVL_TREE_TEMPLATE_LIST
AVL_TREE_CLASS_NAME::AVLTree(const KeyCompare &keyCompare,
	const GetKey &getKey, const NodeAllocator &allocator,
	const GetValue &getValue)
	: fRoot(NULL),
	  fNodeCount(0),
	  fKeyCompare(keyCompare),
	  fGetKey(getKey),
	  fAllocator(allocator),
	  fGetValue(getValue)
{
}

// destructor
AVL_TREE_TEMPLATE_LIST
AVL_TREE_CLASS_NAME::~AVLTree()
{
	_FreeTree(fRoot);
	fRoot = NULL;
}

// Find
AVL_TREE_TEMPLATE_LIST
Value *
AVL_TREE_CLASS_NAME::Find(const Key &key, Iterator *iterator)
{
	Node *node = fRoot;
	while (node) {
		int cmp = fKeyCompare(key, fGetKey(fGetValue(node)));
		if (cmp == 0) {
			if (iterator)
				iterator->_SetTo(this, node);
			return &fGetValue(node);
		}
		if (cmp < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;
}

// FindClose
AVL_TREE_TEMPLATE_LIST
Value *
AVL_TREE_CLASS_NAME::FindClose(const Key &key, bool less, Iterator *iterator)
{
	Node *node = fRoot;
	Node *parent = NULL;
	while (node) {
		int cmp = fKeyCompare(key, fGetKey(fGetValue(node)));
		if (cmp == 0)
			break;
		parent = node;
		if (cmp < 0)
			node = node->left;
		else
			node = node->right;
	}
	// not found: try to get close
	if (!node && parent) {
		node = parent;
		int expectedCmp = (less ? -1 : 1);
		int cmp = fKeyCompare(fGetKey(fGetValue(node)), key);
		if (cmp != expectedCmp) {
			// The node's value is less although for a greater value was asked,
			// or the other way around. We need to iterate to the next node in
			// the right directory. If there is no node, we fail.
			Iterator it;
			it._SetTo(this, node, less);
			if (it.GetNext())
				node = it._GetCurrentNode();
			else
				node = NULL;
		}
	}
	// set the result
	if (node) {
		if (iterator)
			iterator->_SetTo(this, node);
		return &fGetValue(node);
	}
	return NULL;
}

// GetIterator
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::GetIterator(Iterator *iterator, bool reverse)
{
	if (iterator)
		iterator->_SetTo(this, NULL, reverse, true);
}

// Insert
AVL_TREE_TEMPLATE_LIST
status_t
AVL_TREE_CLASS_NAME::Insert(const Value &value, Iterator *iterator)
{
	int result = _Insert(value, &fRoot, iterator);
	switch (result) {
		case OK:
		case HEIGHT_CHANGED:
			return B_OK;
		case NO_MEMORY:
			return B_NO_MEMORY;
		case DUPLICATE:
		default:
			return B_BAD_VALUE;
	}
}

// Remove
AVL_TREE_TEMPLATE_LIST
status_t
AVL_TREE_CLASS_NAME::Remove(const Key &key)
{
	// find node
	Node *node = fRoot;
	while (node) {
		int cmp = fKeyCompare(key, fGetKey(fGetValue(node)));
		if (cmp == 0)
			break;
		else {
			if (cmp < 0)
				node = node->left;
			else
				node = node->right;
		}
	}
	// remove it
	int result = _Remove(node);
	// set result
	switch (result) {
		case OK:
		case HEIGHT_CHANGED:
			return B_OK;
		case NOT_FOUND:
			return B_ENTRY_NOT_FOUND;
		default:
			return B_BAD_VALUE;
	}
}

// Remove
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::Remove(Iterator &iterator)
{
	if (Node *node = iterator._GetCurrentNode()) {
		iterator.GetNext();
		_Remove(node);
	}
}

// Check
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::Check(Node *node, int level, bool levelsOnly) const
{
	int height = 0;
	if (node) {
		// check root node parent
		if (node == fRoot && node->parent != NULL) {
			printf("Root node has parent: %p\n", node->parent);
			debugger("Root node has parent.");
		}
		// check children's parents
		if (node->left && node->left->parent != node) {
			printf("Left child of node has has wrong parent: %p, should be: "
				   "%p\n", node->left->parent, node);
			_DumpNode(node);
			debugger("Left child node has wrong parent.");
		}
		if (node->right && node->right->parent != node) {
			printf("Right child of node has has wrong parent: %p, should be: "
				   "%p\n", node->right->parent, node);
			_DumpNode(node);
			debugger("Right child node has wrong parent.");
		}
		// check heights
		int leftHeight = Check(node->left, level + 1);
		int rightHeight = Check(node->right, level + 1);
		if (node->balance_factor != rightHeight - leftHeight) {
			printf("Subtree %p at level %d has wrong balance factor: left "
				   "height: %d, right height: %d, balance factor: %d\n",
				   node, level, leftHeight, rightHeight, node->balance_factor);
			_DumpNode(node);
			debugger("Node has wrong balance factor.");
		}
		// check AVL property
		if (!levelsOnly && (leftHeight - rightHeight > 1
							|| leftHeight - rightHeight < -1)) {
			printf("Subtree %p at level %d violates the AVL property: left "
				   "height: %d, right height: %d\n", node, level, leftHeight,
				   rightHeight);
			_DumpNode(node);
			debugger("Node violates AVL property.");
		}
		height = max(leftHeight, rightHeight) + 1;
	}
	return height;
}

// _RotateRight
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::_RotateRight(Node **nodeP)
{
	// rotate the nodes
	Node *node = *nodeP;
	Node *left = node->left;
//printf("_RotateRight(): balance: node: %d, left: %d\n",
//node->balance_factor, left->balance_factor);
	*nodeP = left;
	left->parent = node->parent;
	node->left = left->right;
	if (left->right)
		left->right->parent = node;
	left->right = node;
	node->parent = left;
	// adjust the balance factors
	// former pivot
	if (left->balance_factor >= 0)
		node->balance_factor++;
	else
		node->balance_factor += 1 - left->balance_factor;
	// former left
	if (node->balance_factor <= 0)
		left->balance_factor++;
	else
		left->balance_factor += node->balance_factor + 1;
//printf("_RotateRight() end: balance: node: %d, left: %d\n",
//node->balance_factor, left->balance_factor);
}

// _RotateLeft
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::_RotateLeft(Node **nodeP)
{
	// rotate the nodes
	Node *node = *nodeP;
	Node *right = node->right;
//printf("_RotateLeft(): balance: node: %d, right: %d\n",
//node->balance_factor, right->balance_factor);
	*nodeP = right;
	right->parent = node->parent;
	node->right = right->left;
	if (right->left)
		right->left->parent = node;
	right->left = node;
	node->parent = right;
	// adjust the balance factors
	// former pivot
	if (right->balance_factor <= 0)
		node->balance_factor--;
	else
		node->balance_factor -= right->balance_factor + 1;
	// former right
	if (node->balance_factor >= 0)
		right->balance_factor--;
	else
		right->balance_factor += node->balance_factor - 1;
//printf("_RotateLeft() end: balance: node: %d, right: %d\n",
//node->balance_factor, right->balance_factor);
}

// _BalanceInsertLeft
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_BalanceInsertLeft(Node **node)
{
//printf("_BalanceInsertLeft()\n");
//_DumpNode(*node);
//Check(*node, 0, true);
	int result = HEIGHT_CHANGED;
	if ((*node)->balance_factor < LEFT) {
		// tree is left heavy
		Node **left = &(*node)->left;
		if ((*left)->balance_factor == LEFT) {
			// left left heavy
			_RotateRight(node);
		} else {
			// left right heavy
			_RotateLeft(left);
			_RotateRight(node);
		}
		result = OK;
	} else if ((*node)->balance_factor == BALANCED)
		result = OK;
//printf("_BalanceInsertLeft() done: %d\n", result);
	return result;
}

// _BalanceInsertRight
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_BalanceInsertRight(Node **node)
{
//printf("_BalanceInsertRight()\n");
//_DumpNode(*node);
//Check(*node, 0, true);
	int result = HEIGHT_CHANGED;
	if ((*node)->balance_factor > RIGHT) {
		// tree is right heavy
		Node **right = &(*node)->right;
		if ((*right)->balance_factor == RIGHT) {
			// right right heavy
			_RotateLeft(node);
		} else {
			// right left heavy
			_RotateRight(right);
			_RotateLeft(node);
		}
		result = OK;
	} else if ((*node)->balance_factor == BALANCED)
		result = OK;
//printf("_BalanceInsertRight() done: %d\n", result);
	return result;
}

// _Insert
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_Insert(const Value &value, Node **node,
							 Iterator *iterator)
{
	struct node_info {
		Node	**node;
		bool	left;
	};
	node_info stack[kMaxAVLTreeHeight];
	node_info *top = stack;
	const node_info *const bottom = stack;
	// find insertion point
	while (*node) {
		int cmp = fKeyCompare(fGetKey(value), fGetKey(fGetValue(*node)));
		if (cmp == 0)	// duplicate node
			return DUPLICATE;
		else {
			top->node = node;
			if (cmp < 0) {
				top->left = true;
				node = &(*node)->left;
			} else {
				top->left = false;
				node = &(*node)->right;
			}
			top++;
		}
	}
	// allocate and insert node
	*node = fAllocator.Allocate(value);
	if (*node) {
		(*node)->balance_factor = BALANCED;
		fNodeCount++;
	} else
		return NO_MEMORY;
	if (top != bottom)
		(*node)->parent = *top[-1].node;
	// init the iterator
	if (iterator)
		iterator->_SetTo(this, *node);
	// do the balancing
	int result = HEIGHT_CHANGED;
	while (result == HEIGHT_CHANGED && top != bottom) {
		top--;
		node = top->node;
		if (top->left) {
			// left
			(*node)->balance_factor--;
			result = _BalanceInsertLeft(node);
		} else {
			// right
			(*node)->balance_factor++;
			result = _BalanceInsertRight(node);
		}
	}
//Check(*node);
	return result;
}

// _BalanceRemoveLeft
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_BalanceRemoveLeft(Node **node)
{
//printf("_BalanceRemoveLeft()\n");
//_DumpNode(*node);
//Check(*node, 0, true);
	int result = HEIGHT_CHANGED;
	if ((*node)->balance_factor > RIGHT) {
		// tree is right heavy
		Node **right = &(*node)->right;
		if ((*right)->balance_factor == RIGHT) {
			// right right heavy
			_RotateLeft(node);
		} else if ((*right)->balance_factor == BALANCED) {
			// right none heavy
			_RotateLeft(node);
			result = OK;
		} else {
			// right left heavy
			_RotateRight(right);
			_RotateLeft(node);
		}
	} else if ((*node)->balance_factor == RIGHT)
		result = OK;
//printf("_BalanceRemoveLeft() done: %d\n", result);
	return result;
}

// _BalanceRemoveRight
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_BalanceRemoveRight(Node **node)
{
//printf("_BalanceRemoveRight()\n");
//_DumpNode(*node);
//Check(*node, 0, true);
	int result = HEIGHT_CHANGED;
	if ((*node)->balance_factor < LEFT) {
		// tree is left heavy
		Node **left = &(*node)->left;
		if ((*left)->balance_factor == LEFT) {
			// left left heavy
			_RotateRight(node);
		} else if ((*left)->balance_factor == BALANCED) {
			// left none heavy
			_RotateRight(node);
			result = OK;
		} else {
			// left right heavy
			_RotateLeft(left);
			_RotateRight(node);
		}
	} else if ((*node)->balance_factor == LEFT)
		result = OK;
//printf("_BalanceRemoveRight() done: %d\n", result);
	return result;
}

// _RemoveRightMostChild
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_RemoveRightMostChild(Node **node, Node **foundNode)
{
	Node **stack[kMaxAVLTreeHeight];
	Node ***top = stack;
	const Node *const *const *const bottom = stack;
	// find the child
	while ((*node)->right) {
		*top = node;
		top++;
		node = &(*node)->right;
	}
	// found the rightmost child: remove it
	// the found node might have a left child: replace the node with the
	// child
	*foundNode = *node;
	Node *left = (*node)->left;
	if (left)
		left->parent = (*node)->parent;
	*node = left;
	(*foundNode)->left = NULL;
	(*foundNode)->parent = NULL;
	// balancing
	int result = HEIGHT_CHANGED;
	while (result == HEIGHT_CHANGED && top != bottom) {
		top--;
		node = *top;
		(*node)->balance_factor--;
		result = _BalanceRemoveRight(node);
	}
	return result;
}

// _Remove
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_Remove(const Key &key, Node **node)
{
	struct node_info {
		Node	**node;
		bool	left;
	};
	node_info stack[kMaxAVLTreeHeight];
	node_info *top = stack;
	const node_info *const bottom = stack;
	// find node
	while (*node) {
		int cmp = fKeyCompare(key, fGetKey(fGetValue(*node)));
		if (cmp == 0)
			break;
		else {
			top->node = node;
			if (cmp < 0) {
				top->left = true;
				node = &(*node)->left;
			} else {
				top->left = false;
				node = &(*node)->right;
			}
			top++;
		}
	}
	if (!*node)
		return NOT_FOUND;
	// remove and free node
	int result = HEIGHT_CHANGED;
	Node *oldNode = *node;
	Node *replace = NULL;
	if ((*node)->left && (*node)->right) {
		// node has two children
		result = _RemoveRightMostChild(&(*node)->left, &replace);
		replace->parent = (*node)->parent;
		replace->left = (*node)->left;
		replace->right = (*node)->right;
		if ((*node)->left)	// check necessary, if (*node)->left == replace
			(*node)->left->parent = replace;
		(*node)->right->parent = replace;
		replace->balance_factor = (*node)->balance_factor;
		*node = replace;
		if (result == HEIGHT_CHANGED) {
			replace->balance_factor++;
			result = _BalanceRemoveLeft(node);
		}
	} else if ((*node)->left) {
		// node has only left child
		replace = (*node)->left;
		replace->parent = (*node)->parent;
		replace->balance_factor = (*node)->balance_factor + 1;
		*node = replace;
	} else if ((*node)->right) {
		// node has only right child
		replace = (*node)->right;
		replace->parent = (*node)->parent;
		replace->balance_factor = (*node)->balance_factor - 1;
		*node = replace;
	} else {
		// node has no child
		*node = NULL;
	}
	fAllocator.Free(oldNode);
	fNodeCount--;
	// do the balancing
	while (result == HEIGHT_CHANGED && top != bottom) {
		top--;
		node = top->node;
		if (top->left) {
			// left
			(*node)->balance_factor++;
			result = _BalanceRemoveLeft(node);
		} else {
			// right
			(*node)->balance_factor--;
			result = _BalanceRemoveRight(node);
		}
	}
//Check(*node);
	return result;
}

// _Remove
AVL_TREE_TEMPLATE_LIST
int
AVL_TREE_CLASS_NAME::_Remove(Node *node)
{
	if (!node)
		return NOT_FOUND;
	// remove and free node
	Node *parent = node->parent;
	bool isLeft = (parent && parent->left == node);
	Node **nodeP
		= (parent ? (isLeft ? &parent->left : &parent->right) : &fRoot);
	int result = HEIGHT_CHANGED;
	Node *replace = NULL;
	if (node->left && node->right) {
		// node has two children
		result = _RemoveRightMostChild(&node->left, &replace);
		replace->parent = parent;
		replace->left = node->left;
		replace->right = node->right;
		if (node->left)	// check necessary, if node->left == replace
			node->left->parent = replace;
		node->right->parent = replace;
		replace->balance_factor = node->balance_factor;
		*nodeP = replace;
		if (result == HEIGHT_CHANGED) {
			replace->balance_factor++;
			result = _BalanceRemoveLeft(nodeP);
		}
	} else if (node->left) {
		// node has only left child
		replace = node->left;
		replace->parent = parent;
		replace->balance_factor = node->balance_factor + 1;
		*nodeP = replace;
	} else if (node->right) {
		// node has only right child
		replace = node->right;
		replace->parent = node->parent;
		replace->balance_factor = node->balance_factor - 1;
		*nodeP = replace;
	} else {
		// node has no child
		*nodeP = NULL;
	}
	fAllocator.Free(node);
	fNodeCount--;
	// do the balancing
	while (result == HEIGHT_CHANGED && parent) {
		node = parent;
		parent = node->parent;
		bool oldIsLeft = isLeft;
		isLeft = (parent && parent->left == node);
		nodeP = (parent ? (isLeft ? &parent->left : &parent->right) : &fRoot);
		if (oldIsLeft) {
			// left
			node->balance_factor++;
			result = _BalanceRemoveLeft(nodeP);
		} else {
			// right
			node->balance_factor--;
			result = _BalanceRemoveRight(nodeP);
		}
	}
//Check(node);
	return result;
}

// _FreeTree
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::_FreeTree(Node *node)
{
	if (node) {
		_FreeTree(node->left);
		_FreeTree(node->right);
		fAllocator.Free(node);
	}
}

// _DumpNode
AVL_TREE_TEMPLATE_LIST
void
AVL_TREE_CLASS_NAME::_DumpNode(Node *node) const
{
	if (!node)
		return;

	enum node_type {
		ROOT,
		LEFT,
		RIGHT,
	};
	struct node_info {
		Node	*node;
		int		id;
		int		parent;
		int		level;
		int		type;
	};

	node_info *queue = new(nothrow) node_info[fNodeCount];
	if (!queue) {
		printf("_Dump(): Insufficient memory for allocating queue.\n");
	}
	node_info *front = queue;
	node_info *back = queue;
	back->node = node;
	back->id = 0;
	back->level = 0;
	back->type = ROOT;
	back++;
	int level = 0;
	int nextID = 1;
	while (front != back) {
		// pop front
		node_info *current = front;
		front++;
		// get to the correct level
		node = current->node;
		if (level < current->level) {
			printf("\n");
			level++;
		}
		// print node
		switch (current->type) {
			case ROOT:
				printf("[%d:%d]", current->id, node->balance_factor);
				break;
			case LEFT:
				printf("[%d:L:%d:%d]", current->id, current->parent,
					   node->balance_factor);
				break;
			case RIGHT:
				printf("[%d:R:%d:%d]", current->id, current->parent,
					   node->balance_factor);
				break;
		}
		// add child nodes
		if (node->left) {
			back->node = node->left;
			back->id = nextID++;
			back->parent = current->id;
			back->level = current->level + 1;
			back->type = LEFT;
			back++;
		}
		if (node->right) {
			back->node = node->right;
			back->id = nextID++;
			back->parent = current->id;
			back->level = current->level + 1;
			back->type = RIGHT;
			back++;
		}
	}
	printf("\n\n");
	delete[] queue;
}

// _InitIterator
AVL_TREE_TEMPLATE_LIST
inline
void
AVL_TREE_CLASS_NAME::_InitIterator(Iterator *iterator, Node *node,
								   bool reverse, bool initialize)
{
	iterator->_SetTo(this, node, reverse, initialize);
}

#endif	// AVL_TREE_H
