// TwoKeyAVLTree.h
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

#ifndef TWO_KEY_AVL_TREE_H
#define TWO_KEY_AVL_TREE_H

#include "AVLTree.h"

// TwoKeyAVLTreeKey
template<typename PrimaryKey, typename SecondaryKey>
class TwoKeyAVLTreeKey {
public:
	inline TwoKeyAVLTreeKey(const PrimaryKey &primary,
							const SecondaryKey &secondary)
		: primary(primary),
		  secondary(secondary),
		  use_secondary(true)
	{
	}

	inline TwoKeyAVLTreeKey(const PrimaryKey *primary)
		: primary(primary),
		  secondary(NULL),
		  use_secondary(false)
	{
	}

	PrimaryKey		primary;
	SecondaryKey	secondary;
	bool			use_secondary;
};

// TwoKeyAVLTreeKeyCompare
template<typename PrimaryKey, typename SecondaryKey,
		 typename PrimaryKeyCompare, typename SecondaryKeyCompare>
class TwoKeyAVLTreeKeyCompare {
private:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;

public:
	inline TwoKeyAVLTreeKeyCompare(const PrimaryKeyCompare &primary,
								   const SecondaryKeyCompare &secondary)
		: fPrimaryKeyCompare(primary), fSecondaryKeyCompare(secondary) {}

	inline int operator()(const Key &a, const Key &b) const
	{
		int result = fPrimaryKeyCompare(a.primary, b.primary);
		if (result == 0 && a.use_secondary && b.use_secondary)
			result = fSecondaryKeyCompare(a.secondary, b.secondary);
		return result;
	}

private:
	PrimaryKeyCompare	fPrimaryKeyCompare;
	SecondaryKeyCompare	fSecondaryKeyCompare;
};

// TwoKeyAVLTreeGetKey
template<typename Value, typename PrimaryKey, typename SecondaryKey,
		 typename GetPrimaryKey, typename GetSecondaryKey>
class TwoKeyAVLTreeGetKey
{
private:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;

public:
	TwoKeyAVLTreeGetKey(const GetPrimaryKey &getPrimary,
						const GetSecondaryKey &getSecondary)
		: fGetPrimaryKey(getPrimary),
		  fGetSecondaryKey(getSecondary)
	{
	}

	inline Key operator()(const Value &a) const
	{
		return Key(fGetPrimaryKey(a), fGetSecondaryKey(a));
	}

private:
	GetPrimaryKey	fGetPrimaryKey;
	GetSecondaryKey	fGetSecondaryKey;
};

// for convenience
#define TWO_KEY_AVL_TREE_TEMPLATE_LIST template<typename Value, \
	typename PrimaryKey, typename PrimaryKeyCompare, \
	typename GetPrimaryKey, typename SecondaryKey, typename Node, \
	typename SecondaryKeyCompare, typename GetSecondaryKey, \
	typename NodeAllocator, typename GetValue>
#define TWO_KEY_AVL_TREE_CLASS_NAME TwoKeyAVLTree<Value, PrimaryKey, \
	PrimaryKeyCompare, GetPrimaryKey, SecondaryKey, Node, \
	SecondaryKeyCompare, GetSecondaryKey, NodeAllocator, GetValue>

// TwoKeyAVLTree
template<typename Value, typename PrimaryKey,
		 typename PrimaryKeyCompare, typename GetPrimaryKey,
		 typename SecondaryKey = Value,
		 typename Node = AVLTreeStandardNode<Value>,
		 typename SecondaryKeyCompare = AVLTreeStandardCompare<SecondaryKey>,
		 typename GetSecondaryKey = AVLTreeStandardGetKey<Value, SecondaryKey>,
		 typename NodeAllocator = AVLTreeStandardNodeAllocator<Value, Node>,
		 typename GetValue = AVLTreeStandardGetValue<Value, Node> >
class TwoKeyAVLTree : private AVLTree<Value,
	TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey>, Node,
	TwoKeyAVLTreeKeyCompare<PrimaryKey, SecondaryKey, PrimaryKeyCompare,
							SecondaryKeyCompare>,
	TwoKeyAVLTreeGetKey<Value, PrimaryKey, SecondaryKey, GetPrimaryKey,
						GetSecondaryKey>,
	NodeAllocator, GetValue> {
private:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;
	typedef TwoKeyAVLTreeKeyCompare<PrimaryKey, SecondaryKey,
									PrimaryKeyCompare, SecondaryKeyCompare>
			KeyCompare;
	typedef TwoKeyAVLTreeGetKey<Value, PrimaryKey, SecondaryKey, GetPrimaryKey,
								GetSecondaryKey>
			GetKey;
	typedef AVLTree<Value, Key, Node, KeyCompare, GetKey, NodeAllocator,
					GetValue> BaseClass;
public:
	typedef typename BaseClass::Iterator Iterator;

public:
	TwoKeyAVLTree();
	TwoKeyAVLTree(const PrimaryKeyCompare &primaryCompare,
				  const GetPrimaryKey &getPrimary,
				  const SecondaryKeyCompare &secondaryCompare,
				  const GetSecondaryKey &getSecondary,
				  const NodeAllocator &allocator,
				  const GetValue &getValue);
	~TwoKeyAVLTree();

	inline int CountItems() const	{ return BaseClass::CountItems(); }

	Value *FindFirst(const PrimaryKey &key, Iterator *iterator = NULL);
	Value *FindLast(const PrimaryKey &key, Iterator *iterator = NULL);
	inline Value *Find(const PrimaryKey &primaryKey,
					   const SecondaryKey &secondaryKey,
					   Iterator *iterator = NULL);

	inline void GetIterator(Iterator *iterator, bool reverse = false);

	inline status_t Insert(const Value &value, Iterator *iterator = NULL);
	inline status_t Remove(const PrimaryKey &primaryKey,
						   const SecondaryKey &secondaryKey);
	inline void Remove(Iterator &iterator);

private:
	PrimaryKeyCompare	fPrimaryKeyCompare;
	GetPrimaryKey		fGetPrimaryKey;
};


// constructor
TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::TwoKeyAVLTree()
	: BaseClass(KeyCompare(PrimaryKeyCompare(), SecondaryKeyCompare()),
						   GetKey(GetPrimaryKey(), GetSecondaryKey()),
						   NodeAllocator(), GetValue())
{
}

// constructor
TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::TwoKeyAVLTree(
	const PrimaryKeyCompare &primaryCompare, const GetPrimaryKey &getPrimary,
	const SecondaryKeyCompare &secondaryCompare,
	const GetSecondaryKey &getSecondary, const NodeAllocator &allocator,
	const GetValue &getValue)
	: BaseClass(KeyCompare(primaryCompare, secondaryCompare),
						   GetKey(getPrimary, getSecondary),
						   allocator, getValue),
	  fPrimaryKeyCompare(primaryCompare),
	  fGetPrimaryKey(getPrimary)
	  
{
}

// destructor
TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::~TwoKeyAVLTree()
{
}

// FindFirst
TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value *
TWO_KEY_AVL_TREE_CLASS_NAME::FindFirst(const PrimaryKey &key,
									   Iterator *iterator)
{
	Node *node = BaseClass::fRoot;
	while (node) {
		int cmp = fPrimaryKeyCompare(key, fGetPrimaryKey(fGetValue(node)));
		if (cmp == 0) {
			// found a matching node, now get the left-most node with that key
			while (node->left && fPrimaryKeyCompare(key,
				   		fGetPrimaryKey(fGetValue(node->left))) == 0) {
				node = node->left;
			}
			if (iterator)
				_InitIterator(iterator, node);
			return &fGetValue(node);
		}
		if (cmp < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;
}

// FindLast
TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value *
TWO_KEY_AVL_TREE_CLASS_NAME::FindLast(const PrimaryKey &key,
									  Iterator *iterator)
{
	Node *node = BaseClass::fRoot;
	while (node) {
		int cmp = fPrimaryKeyCompare(key, fGetPrimaryKey(fGetValue(node)));
		if (cmp == 0) {
			// found a matching node, now get the right-most node with that key
			while (node->right && fPrimaryKeyCompare(key,
				   		fGetPrimaryKey(fGetValue(node->right))) == 0) {
				node = node->right;
			}
			if (iterator)
				_InitIterator(iterator, node);
			return &fGetValue(node);
		}
		if (cmp < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;
}

// Find
TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value *
TWO_KEY_AVL_TREE_CLASS_NAME::Find(const PrimaryKey &primaryKey,
								  const SecondaryKey &secondaryKey,
								  Iterator *iterator)
{
	return BaseClass::Find(Key(primaryKey, secondaryKey), iterator);
}

// GetIterator
TWO_KEY_AVL_TREE_TEMPLATE_LIST
void
TWO_KEY_AVL_TREE_CLASS_NAME::GetIterator(Iterator *iterator, bool reverse)
{
	BaseClass::GetIterator(iterator, reverse);
}

// Insert
TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Insert(const Value &value, Iterator *iterator)
{
	return BaseClass::Insert(value, iterator);
}

// Remove
TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Remove(const PrimaryKey &primaryKey,
									const SecondaryKey &secondaryKey)
{
	return BaseClass::Remove(Key(primaryKey, secondaryKey));
}

// Remove
TWO_KEY_AVL_TREE_TEMPLATE_LIST
void
TWO_KEY_AVL_TREE_CLASS_NAME::Remove(Iterator &iterator)
{
	BaseClass::Remove(iterator);
}

#endif	// TWO_KEY_AVL_TREE_H
