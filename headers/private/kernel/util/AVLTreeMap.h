/*
 * Copyright 2003-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AVL_TREE_MAP_H
#define _AVL_TREE_MAP_H


#include <OS.h>
#include <util/kernel_cpp.h>

#include <util/MallocFreeAllocator.h>


// maximal height of a tree
static const int kMaxAVLTreeHeight = 32;

class AVLTreeIterator;


// AVLTreeNode
struct AVLTreeNode {
	AVLTreeNode*	parent;
	AVLTreeNode*	left;
	AVLTreeNode*	right;
	int				balance_factor;
};


// AVLTreeCompare
class AVLTreeCompare {
public:
	virtual						~AVLTreeCompare();

	virtual	int					CompareKeyNode(const void* key,
									const AVLTreeNode* node) = 0;
	virtual	int					CompareNodes(const AVLTreeNode* node1,
									const AVLTreeNode* node2) = 0;
};


// AVLTree
class AVLTree {
public:
								AVLTree(AVLTreeCompare* compare);
								~AVLTree();

	inline	int					Count() const	{ return fNodeCount; }
	inline	bool				IsEmpty() const	{ return (fNodeCount == 0); }
			void				MakeEmpty();

	inline	AVLTreeNode*		Root() const	{ return fRoot; }

			AVLTreeNode*		LeftMost(AVLTreeNode* node) const;
			AVLTreeNode*		RightMost(AVLTreeNode* node) const;

			AVLTreeNode*		Previous(AVLTreeNode* node) const;
			AVLTreeNode*		Next(AVLTreeNode* node) const;

	inline	AVLTreeIterator		GetIterator() const;
	inline	AVLTreeIterator		GetIterator(AVLTreeNode* node) const;

			AVLTreeNode*		Find(const void* key);
			AVLTreeNode*		FindClose(const void* key, bool less);

			status_t			Insert(AVLTreeNode* element);
			AVLTreeNode*		Remove(const void* key);
			bool				Remove(AVLTreeNode* element);

private:
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
			void				_RotateRight(AVLTreeNode** nodeP);
			void				_RotateLeft(AVLTreeNode** nodeP);

			// insert
			int					_BalanceInsertLeft(AVLTreeNode** node);
			int					_BalanceInsertRight(AVLTreeNode** node);
			int					_Insert(AVLTreeNode* nodeToInsert);

			// remove
			int					_BalanceRemoveLeft(AVLTreeNode** node);
			int					_BalanceRemoveRight(AVLTreeNode** node);
			int					_RemoveRightMostChild(AVLTreeNode** node,
									AVLTreeNode** foundNode);
			int					_Remove(AVLTreeNode* node);

			AVLTreeNode*		fRoot;
			int					fNodeCount;
			AVLTreeCompare*		fCompare;
};


// AVLTreeIterator
class AVLTreeIterator {
public:
	inline AVLTreeIterator()
		: fParent(NULL),
		  fCurrent(NULL),
		  fNext(NULL)
	{
	}

	inline AVLTreeIterator(const AVLTreeIterator& other)
		: fParent(other.fParent),
		  fCurrent(other.fCurrent),
		  fNext(other.fNext)
	{
	}

	inline AVLTreeNode* Current() const
	{
		return fCurrent;
	}

	inline bool HasNext() const
	{
		return fNext;
	}

	inline AVLTreeNode* Next()
	{
		fCurrent = fNext;

		if (fNext)
			fNext = fParent->Next(fNext);

		return fCurrent;
	}

	inline AVLTreeNode* Previous()
	{
		if (fCurrent) {
			fNext = fCurrent;
			fCurrent = fParent->Previous(fCurrent);
		} else if (fNext)
			fCurrent = fParent->Previous(fNext);

		return fCurrent;
	}

	inline AVLTreeNode* Remove()
	{
		if (!fCurrent)
			return NULL;

		AVLTreeNode* node = fCurrent;
		fCurrent = NULL;

		return (const_cast<AVLTree*>(fParent)->Remove(node) ? node : NULL);
	}

	inline AVLTreeIterator& operator=(const AVLTreeIterator& other)
	{
		fParent = other.fParent;
		fCurrent = other.fCurrent;
		fNext = other.fNext;
		return *this;
	}

private:
	inline AVLTreeIterator(const AVLTree* parent, AVLTreeNode* current,
			AVLTreeNode* next)
		: fParent(parent),
		  fCurrent(current),
		  fNext(next)
	{
	}

protected:
	friend class AVLTree;

	const AVLTree*	fParent;
	AVLTreeNode*	fCurrent;
	AVLTreeNode*	fNext;
};


// GetIterator
inline AVLTreeIterator
AVLTree::GetIterator() const
{
	return AVLTreeIterator(this, NULL, LeftMost(fRoot));
}


// GetIterator
inline AVLTreeIterator
AVLTree::GetIterator(AVLTreeNode* node) const
{
	return AVLTreeIterator(this, node, Next(node));
}


// #pragma mark - AVLTreeMap and friends


// strategies
namespace AVLTreeMapStrategy {
	// key orders
	template<typename Value> class Ascending;
	template<typename Value> class Descending;

	//! Automatic node strategy (works like STL containers do)
	template <typename Key, typename Value,
			  typename KeyOrder = Ascending<Key>,
			  template <typename> class Allocator = MallocFreeAllocator>
	class Auto;

/*!	NodeStrategy must implement this interface:
	typename Node;
	inline Node* Allocate(const Key& key, const Value& value)
	inline void Free(Node* node)
	inline Key GetKey(Node* node) const
	inline Value& GetValue(Node* node) const
	inline AVLTreeNode* GetAVLTreeNode(Node* node) const
	inline Node* GetNode(AVLTreeNode* node) const
	inline int CompareKeyNode(const Key& a, const Node* b)
	inline int CompareNodes(const Node* a, const Node* b)
*/
}


// for convenience
#define _AVL_TREE_MAP_TEMPLATE_LIST template<typename Key, typename Value, \
	typename NodeStrategy>
#define _AVL_TREE_MAP_CLASS_NAME AVLTreeMap<Key, Value, NodeStrategy>


// AVLTreeMap
template<typename Key, typename Value,
		 typename NodeStrategy = AVLTreeMapStrategy::Auto<Key, Value> >
class AVLTreeMap : protected AVLTreeCompare {
private:
	typedef typename NodeStrategy::Node	Node;
	typedef _AVL_TREE_MAP_CLASS_NAME	Class;

public:
	class Iterator;
	class ConstIterator;

public:
								AVLTreeMap(const NodeStrategy& strategy
									= NodeStrategy());
	virtual						~AVLTreeMap();

	inline	int					Count() const	{ return fTree.Count(); }
	inline	bool				IsEmpty() const	{ return fTree.IsEmpty(); }
	inline	void				MakeEmpty();

			Node*				RootNode() const;

	inline	Iterator			GetIterator();
	inline	ConstIterator		GetIterator() const;

	inline	Iterator			GetIterator(Node* node);
	inline	ConstIterator		GetIterator(Node* node) const;

			Iterator			Find(const Key& key);
			Iterator			FindClose(const Key& key, bool less);

			status_t			Insert(const Key& key, const Value& value,
									Iterator* iterator);
			status_t			Remove(const Key& key);

			const NodeStrategy&	GetNodeStrategy() const	{ return fStrategy; }

protected:
	// AVLTreeCompare
	virtual	int					CompareKeyNode(const void* key,
									const AVLTreeNode* node);
	virtual	int					CompareNodes(const AVLTreeNode* node1,
									const AVLTreeNode* node2);

			void				_FreeTree(AVLTreeNode* node);

	// strategy shortcuts
	inline	Node*				_Allocate(const Key& key, const Value& value);
	inline	void				_Free(Node* node);
	inline	Key&				_GetKey(Node* node) const;
	inline	Value&				_GetValue(Node* node) const;
	inline	AVLTreeNode*		_GetAVLTreeNode(const Node* node) const;
	inline	Node*				_GetNode(const AVLTreeNode* node) const;
	inline	int					_CompareKeyNode(const Key& a, const Node* b);
	inline	int					_CompareNodes(const Node* a, const Node* b);

protected:
			friend class Iterator;
			friend class ConstIterator;

			AVLTree				fTree;
			NodeStrategy		fStrategy;

public:
	// Iterator
	// (need to implement it here, otherwise gcc 2.95.3 chokes)
	class Iterator : public ConstIterator {
	public:
		inline Iterator()
			: ConstIterator()
		{
		}
	
		inline Iterator(const Iterator& other)
			: ConstIterator(other)
		{
		}
	
		inline void Remove()
		{
			if (AVLTreeNode* node = ConstIterator::fTreeIterator.Remove()) {
				_AVL_TREE_MAP_CLASS_NAME* parent
					= const_cast<_AVL_TREE_MAP_CLASS_NAME*>(
						ConstIterator::fParent);
				parent->_Free(parent->_GetNode(node));
			}
		}
	
	private:
		inline Iterator(_AVL_TREE_MAP_CLASS_NAME* parent,
			const AVLTreeIterator& treeIterator)
			: ConstIterator(parent, treeIterator)
		{
		}
	
		friend class _AVL_TREE_MAP_CLASS_NAME;
	};
};


// ConstIterator
_AVL_TREE_MAP_TEMPLATE_LIST
class _AVL_TREE_MAP_CLASS_NAME::ConstIterator {
public:
	inline ConstIterator()
		: fParent(NULL),
		  fTreeIterator()
	{
	}

	inline ConstIterator(const ConstIterator& other)
		: fParent(other.fParent),
		  fTreeIterator(other.fTreeIterator)
	{
	}

	inline bool HasCurrent() const
	{
		return fTreeIterator.Current();
	}

	inline Key CurrentKey()
	{
		if (AVLTreeNode* node = fTreeIterator.Current())
			return fParent->_GetKey(fParent->_GetNode(node));
		return Key();
	}

	inline Value Current()
	{
		if (AVLTreeNode* node = fTreeIterator.Current())
			return fParent->_GetValue(fParent->_GetNode(node));
		return Value();
	}

	inline Value* CurrentValuePointer()
	{
		if (AVLTreeNode* node = fTreeIterator.Current())
			return &fParent->_GetValue(fParent->_GetNode(node));
		return NULL;
	}

	inline bool HasNext() const
	{
		return fTreeIterator.HasNext();
	}

	inline Value Next()
	{
		if (AVLTreeNode* node = fTreeIterator.Next())
			return fParent->_GetValue(fParent->_GetNode(node));
		return Value();
	}

	inline Value Previous()
	{
		if (AVLTreeNode* node = fTreeIterator.Previous())
			return fParent->_GetValue(fParent->_GetNode(node));
		return Value();
	}

	inline ConstIterator& operator=(const ConstIterator& other)
	{
		fParent = other.fParent;
		fTreeIterator = other.fTreeIterator;
		return *this;
	}

protected:
	inline ConstIterator(const _AVL_TREE_MAP_CLASS_NAME* parent,
		const AVLTreeIterator& treeIterator)
	{
		fParent = parent;
		fTreeIterator = treeIterator;
	}

	friend class _AVL_TREE_MAP_CLASS_NAME;

	const _AVL_TREE_MAP_CLASS_NAME*	fParent;
	AVLTreeIterator					fTreeIterator;
};


// constructor
_AVL_TREE_MAP_TEMPLATE_LIST
_AVL_TREE_MAP_CLASS_NAME::AVLTreeMap(const NodeStrategy& strategy)
	: fTree(this),
	  fStrategy(strategy)
{
}


// destructor
_AVL_TREE_MAP_TEMPLATE_LIST
_AVL_TREE_MAP_CLASS_NAME::~AVLTreeMap()
{
}


// MakeEmpty
_AVL_TREE_MAP_TEMPLATE_LIST
inline void
_AVL_TREE_MAP_CLASS_NAME::MakeEmpty()
{
	AVLTreeNode* root = fTree.Root();
	_FreeTree(root);
}


// RootNode
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::Node*
_AVL_TREE_MAP_CLASS_NAME::RootNode() const
{
	if (AVLTreeNode* root = fTree.Root())
		return _GetNode(root);
	return NULL;
}


// GetIterator
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::Iterator
_AVL_TREE_MAP_CLASS_NAME::GetIterator()
{
	return Iterator(this, fTree.GetIterator());
}


// GetIterator
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::ConstIterator
_AVL_TREE_MAP_CLASS_NAME::GetIterator() const
{
	return ConstIterator(this, fTree.GetIterator());
}


// GetIterator
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::Iterator
_AVL_TREE_MAP_CLASS_NAME::GetIterator(Node* node)
{
	return Iterator(this, fTree.GetIterator(_GetAVLTreeNode(node)));
}


// GetIterator
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::ConstIterator
_AVL_TREE_MAP_CLASS_NAME::GetIterator(Node* node) const
{
	return ConstIterator(this, fTree.GetIterator(_GetAVLTreeNode(node)));
}


// Find
_AVL_TREE_MAP_TEMPLATE_LIST
typename _AVL_TREE_MAP_CLASS_NAME::Iterator
_AVL_TREE_MAP_CLASS_NAME::Find(const Key& key)
{
	if (AVLTreeNode* node = fTree.Find(&key))
		return Iterator(this, fTree.GetIterator(node));
	return Iterator();
}


// FindClose
_AVL_TREE_MAP_TEMPLATE_LIST
typename _AVL_TREE_MAP_CLASS_NAME::Iterator
_AVL_TREE_MAP_CLASS_NAME::FindClose(const Key& key, bool less)
{
	if (AVLTreeNode* node = fTree.FindClose(&key, less))
		return Iterator(this, fTree.GetIterator(node));
	return Iterator();
}


// Insert
_AVL_TREE_MAP_TEMPLATE_LIST
status_t
_AVL_TREE_MAP_CLASS_NAME::Insert(const Key& key, const Value& value,
	Iterator* iterator)
{
	// allocate a node
	Node* userNode = _Allocate(key, value);
	if (!userNode)
		return B_NO_MEMORY;

	// insert node
	AVLTreeNode* node = _GetAVLTreeNode(userNode);
	status_t error = fTree.Insert(node);
	if (error != B_OK) {
		_Free(userNode);
		return error;
	}

	if (iterator)
		*iterator = Iterator(this, fTree.GetIterator(node));

	return B_OK;
}


// Remove
_AVL_TREE_MAP_TEMPLATE_LIST
status_t
_AVL_TREE_MAP_CLASS_NAME::Remove(const Key& key)
{
	AVLTreeNode* node = fTree.Remove(&key);
	if (!node)
		return B_ENTRY_NOT_FOUND;

	_Free(_GetNode(node));
	return B_OK;
}


// CompareKeyNode
_AVL_TREE_MAP_TEMPLATE_LIST
int
_AVL_TREE_MAP_CLASS_NAME::CompareKeyNode(const void* key,
	const AVLTreeNode* node)
{
	return _CompareKeyNode(*(const Key*)key, _GetNode(node));
}


// CompareNodes
_AVL_TREE_MAP_TEMPLATE_LIST
int
_AVL_TREE_MAP_CLASS_NAME::CompareNodes(const AVLTreeNode* node1,
	const AVLTreeNode* node2)
{
	return _CompareNodes(_GetNode(node1), _GetNode(node2));
}


// _Allocate
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::Node*
_AVL_TREE_MAP_CLASS_NAME::_Allocate(const Key& key, const Value& value)
{
	return fStrategy.Allocate(key, value);
}


// _Free
_AVL_TREE_MAP_TEMPLATE_LIST
inline void
_AVL_TREE_MAP_CLASS_NAME::_Free(Node* node)
{
	fStrategy.Free(node);
}


// _GetKey
_AVL_TREE_MAP_TEMPLATE_LIST
inline Key&
_AVL_TREE_MAP_CLASS_NAME::_GetKey(Node* node) const
{
	return fStrategy.GetKey(node);
}


// _GetValue
_AVL_TREE_MAP_TEMPLATE_LIST
inline Value&
_AVL_TREE_MAP_CLASS_NAME::_GetValue(Node* node) const
{
	return fStrategy.GetValue(node);
}


// _GetAVLTreeNode
_AVL_TREE_MAP_TEMPLATE_LIST
inline AVLTreeNode*
_AVL_TREE_MAP_CLASS_NAME::_GetAVLTreeNode(const Node* node) const
{
	return fStrategy.GetAVLTreeNode(const_cast<Node*>(node));
}


// _GetNode
_AVL_TREE_MAP_TEMPLATE_LIST
inline typename _AVL_TREE_MAP_CLASS_NAME::Node*
_AVL_TREE_MAP_CLASS_NAME::_GetNode(const AVLTreeNode* node) const
{
	return fStrategy.GetNode(const_cast<AVLTreeNode*>(node));
}


// _CompareKeyNode
_AVL_TREE_MAP_TEMPLATE_LIST
inline int
_AVL_TREE_MAP_CLASS_NAME::_CompareKeyNode(const Key& a, const Node* b)
{
	return fStrategy.CompareKeyNode(a, b);
}


// _CompareNodes
_AVL_TREE_MAP_TEMPLATE_LIST
inline int
_AVL_TREE_MAP_CLASS_NAME::_CompareNodes(const Node* a, const Node* b)
{
	return fStrategy.CompareNodes(a, b);
}


// _FreeTree
_AVL_TREE_MAP_TEMPLATE_LIST
void
_AVL_TREE_MAP_CLASS_NAME::_FreeTree(AVLTreeNode* node)
{
	if (node) {
		_FreeTree(node->left);
		_FreeTree(node->right);
		_Free(_GetNode(node));
	}
}


// #pragma mark - strategy parameters

// Ascending
namespace AVLTreeMapStrategy {
template<typename Value>
class Ascending {
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
}


// Descending
namespace AVLTreeMapStrategy {
template<typename Value>
class Descending {
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
}


// #pragma mark - strategies


// Auto
namespace AVLTreeMapStrategy {
template <typename Key, typename Value, typename KeyOrder,
		  template <typename> class Allocator>
class Auto {
public:
	struct Node : AVLTreeNode {
		Node(const Key &key, const Value &value)
			: AVLTreeNode(),
			  key(key),
			  value(value)
		{
		}

		Key		key;
		Value	value;
	};

	inline Node* Allocate(const Key& key, const Value& value)
	{
		Node* result = fAllocator.Allocate();
		if (result)
			fAllocator.Construct(result, key, value);
		return result;
	}

	inline void Free(Node* node)
	{
		fAllocator.Destruct(node);
		fAllocator.Deallocate(node);
	}

	inline Key& GetKey(Node* node) const
	{
		return node->key;
	}

	inline Value& GetValue(Node* node) const
	{
		return node->value;
	}

	inline AVLTreeNode* GetAVLTreeNode(Node* node) const
	{
		return node;
	}

	inline Node* GetNode(AVLTreeNode* node) const
	{
		return static_cast<Node*>(node);
	}

	inline int CompareKeyNode(const Key& a, const Node* b) const
	{
		return fCompare(a, _GetKey(b));
	}

	inline int CompareNodes(const Node* a, const Node* b) const
	{
		return fCompare(_GetKey(a), _GetKey(b));
	}

private:
	KeyOrder		fCompare;
	Allocator<Node>	fAllocator;
};
}

#endif	// _AVL_TREE_MAP_H
