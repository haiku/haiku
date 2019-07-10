/*
 * Copyright 2003-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TWO_KEY_AVL_TREE_H
#define TWO_KEY_AVL_TREE_H


#include <slab/Slab.h>
#include <util/AVLTreeMap.h>


// #pragma mark - TwoKeyAVLTreeKey


template<typename PrimaryKey, typename SecondaryKey>
class TwoKeyAVLTreeKey {
public:
	inline TwoKeyAVLTreeKey(const PrimaryKey& primary,
		const SecondaryKey& secondary)
		:
		primary(primary),
		secondary(secondary),
		use_secondary(true)
	{
	}

	inline TwoKeyAVLTreeKey(const PrimaryKey* primary)
		:
		primary(primary),
		secondary(NULL),
		use_secondary(false)
	{
	}

	PrimaryKey		primary;
	SecondaryKey	secondary;
	bool			use_secondary;
};


// #pragma mark - TwoKeyAVLTreeKeyCompare


template<typename PrimaryKey, typename SecondaryKey,
	typename PrimaryKeyCompare, typename SecondaryKeyCompare>
class TwoKeyAVLTreeKeyCompare {
private:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;

public:
	inline TwoKeyAVLTreeKeyCompare(const PrimaryKeyCompare& primary,
								   const SecondaryKeyCompare& secondary)
		:
		fPrimaryKeyCompare(primary),
		fSecondaryKeyCompare(secondary)
	{
	}

	inline int operator()(const Key& a, const Key& b) const
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


// #pragma mark - TwoKeyAVLTreeGetKey


template<typename Value, typename PrimaryKey, typename SecondaryKey,
	typename GetPrimaryKey, typename GetSecondaryKey>
class TwoKeyAVLTreeGetKey {
private:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;

public:
	TwoKeyAVLTreeGetKey(const GetPrimaryKey& getPrimary,
		const GetSecondaryKey& getSecondary)
		:
		fGetPrimaryKey(getPrimary),
		fGetSecondaryKey(getSecondary)
	{
	}

	inline Key operator()(const Value& a) const
	{
		return Key(fGetPrimaryKey(a), fGetSecondaryKey(a));
	}

private:
	GetPrimaryKey	fGetPrimaryKey;
	GetSecondaryKey	fGetSecondaryKey;
};


// #pragma mark - TwoKeyAVLTreeStandardCompare


template<typename Value>
class TwoKeyAVLTreeStandardCompare {
public:
	inline int operator()(const Value& a, const Value& b) const
	{
		if (a < b)
			return -1;
		else if (a > b)
			return 1;
		return 0;
	}
};


// #pragma mark - TwoKeyAVLTreeStandardGetKey


template<typename Value, typename Key>
class TwoKeyAVLTreeStandardGetKey {
public:
	inline const Key& operator()(const Value& a) const
	{
		return a;
	}

	inline Key& operator()(Value& a) const
	{
		return a;
	}
};


// #pragma mark - TwoKeyAVLTreeNodeStrategy


template <typename PrimaryKey, typename SecondaryKey, typename Value,
	typename PrimaryKeyCompare, typename SecondaryKeyCompare,
	typename GetPrimaryKey, typename GetSecondaryKey>
class TwoKeyAVLTreeNodeStrategy {
public:
	typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;

	TwoKeyAVLTreeNodeStrategy(
		const PrimaryKeyCompare& primaryKeyCompare = PrimaryKeyCompare(),
		const SecondaryKeyCompare& secondaryKeyCompare = SecondaryKeyCompare(),
		const GetPrimaryKey& getPrimaryKey = GetPrimaryKey(),
		const GetSecondaryKey& getSecondaryKey = GetSecondaryKey())
		:
		fPrimaryKeyCompare(primaryKeyCompare),
		fSecondaryKeyCompare(secondaryKeyCompare),
		fGetPrimaryKey(getPrimaryKey),
		fGetSecondaryKey(getSecondaryKey)
	{
		fObjectCache = create_object_cache("packagefs TwoKeyAVLTreeNodes", sizeof(Node), 8,
			NULL, NULL, NULL);
		fObjectCacheRefs = new int32(1);
	}
	TwoKeyAVLTreeNodeStrategy(const TwoKeyAVLTreeNodeStrategy& other)
		:
		fPrimaryKeyCompare(other.fPrimaryKeyCompare),
		fSecondaryKeyCompare(other.fSecondaryKeyCompare),
		fGetPrimaryKey(other.fGetPrimaryKey),
		fGetSecondaryKey(other.fGetSecondaryKey),
		fObjectCache(other.fObjectCache),
		fObjectCacheRefs(other.fObjectCacheRefs)
	{
		atomic_add(fObjectCacheRefs, 1);
	}
	~TwoKeyAVLTreeNodeStrategy()
	{
		atomic_add(fObjectCacheRefs, -1);
		if (atomic_get(fObjectCacheRefs) == 0) {
			delete_object_cache(fObjectCache);
			delete fObjectCacheRefs;
		}
	}

	struct Node : AVLTreeNode {
		static void* operator new(size_t size, object_cache* cache) {
			if (size != sizeof(Node) || !cache)
				panic("unexpected size passed to operator new!");
			return object_cache_alloc(cache, 0);
		}

		Node(const Value& value)
			:
			AVLTreeNode(),
			value(value)
		{
		}

		Value	value;
	};

	inline Node* Allocate(const Key& key, const Value& value) const
	{
		return new(fObjectCache) Node(value);
	}

	inline void Free(Node* node) const
	{
		if (node == NULL)
			return;

		// There is no way to overload operator delete with extra parameters.
		node->~Node();
		object_cache_free(fObjectCache, node, 0);
	}

	// internal use (not part of the strategy)
	inline Key GetValueKey(const Value& value) const
	{
		return Key(fGetPrimaryKey(value), fGetSecondaryKey(value));
	}

	inline Key GetKey(Node* node) const
	{
		return GetValueKey(node->value);
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
		return _CompareKeys(a, GetKey(const_cast<Node*>(b)));
	}

	inline int CompareNodes(const Node* a, const Node* b) const
	{
		return _CompareKeys(GetKey(const_cast<Node*>(a)),
			GetKey(const_cast<Node*>(b)));
	}

private:
	inline int _CompareKeys(const Key& a, const Key& b) const
	{
		int result = fPrimaryKeyCompare(a.primary, b.primary);
		if (result == 0 && a.use_secondary && b.use_secondary)
			result = fSecondaryKeyCompare(a.secondary, b.secondary);
		return result;
	}

	PrimaryKeyCompare	fPrimaryKeyCompare;
	SecondaryKeyCompare	fSecondaryKeyCompare;
	GetPrimaryKey		fGetPrimaryKey;
	GetSecondaryKey		fGetSecondaryKey;

	object_cache*		fObjectCache;
	int32*				fObjectCacheRefs;
};


// for convenience
#define TWO_KEY_AVL_TREE_TEMPLATE_LIST template<typename Value, \
	typename PrimaryKey, typename PrimaryKeyCompare, \
	typename GetPrimaryKey, typename SecondaryKey, \
	typename SecondaryKeyCompare, typename GetSecondaryKey>
#define TWO_KEY_AVL_TREE_CLASS_NAME TwoKeyAVLTree<Value, PrimaryKey, \
	PrimaryKeyCompare, GetPrimaryKey, SecondaryKey, \
	SecondaryKeyCompare, GetSecondaryKey>


// #pragma mark - TwoKeyAVLTree


template<typename Value, typename PrimaryKey,
	typename PrimaryKeyCompare, typename GetPrimaryKey,
	typename SecondaryKey = Value,
	typename SecondaryKeyCompare = TwoKeyAVLTreeStandardCompare<SecondaryKey>,
	typename GetSecondaryKey
		= TwoKeyAVLTreeStandardGetKey<Value, SecondaryKey> >
class TwoKeyAVLTree {
public:
			typedef TwoKeyAVLTreeKey<PrimaryKey, SecondaryKey> Key;
			typedef TwoKeyAVLTreeNodeStrategy<PrimaryKey, SecondaryKey, Value,
				PrimaryKeyCompare, SecondaryKeyCompare, GetPrimaryKey,
				GetSecondaryKey> NodeStrategy;
			typedef AVLTreeMap<Key, Value, NodeStrategy> TreeMap;

			typedef typename TreeMap::Iterator	TreeMapIterator;
			typedef typename NodeStrategy::Node Node;

			class Iterator;

public:
								TwoKeyAVLTree();
								TwoKeyAVLTree(
									const PrimaryKeyCompare& primaryCompare,
									const GetPrimaryKey& getPrimary,
									const SecondaryKeyCompare& secondaryCompare,
									const GetSecondaryKey& getSecondary);
								~TwoKeyAVLTree();

	inline	int					CountItems() const	{ return fTreeMap.Count(); }

			Node*				Previous(Node* node) const;
			Node*				Next(Node* node) const;

			Value*				FindFirst(const PrimaryKey& key,
									Iterator* iterator = NULL);
			Value*				FindFirstClosest(const PrimaryKey& key,
									bool less, Iterator* iterator = NULL);
			Value*				FindLast(const PrimaryKey& key,
									Iterator* iterator = NULL);
	inline	Value*				Find(const PrimaryKey& primaryKey,
									const SecondaryKey& secondaryKey,
									Iterator* iterator = NULL);

	inline	void				GetIterator(Iterator* iterator);
	inline	void				GetIterator(Node* node, Iterator* iterator);

	inline	status_t			Insert(const Value& value,
									Iterator* iterator);
	inline	status_t			Insert(const Value& value,
									Node** _node = NULL);
	inline	status_t			Remove(const PrimaryKey& primaryKey,
									const SecondaryKey& secondaryKey);
	inline	status_t			Remove(Node* node);

private:
			Node*				_FindFirst(const PrimaryKey& key,
									Node** _parent) const;

private:
			TreeMap				fTreeMap;
			PrimaryKeyCompare	fPrimaryKeyCompare;
			GetPrimaryKey		fGetPrimaryKey;
};


// #pragma mark - Iterator


TWO_KEY_AVL_TREE_TEMPLATE_LIST
class TWO_KEY_AVL_TREE_CLASS_NAME::Iterator {
public:
	typedef typename TWO_KEY_AVL_TREE_CLASS_NAME::TreeMapIterator
		TreeMapIterator;

	inline Iterator()
		:
		fIterator()
	{
	}

	inline ~Iterator()
	{
	}

	inline Value* Current()
	{
		return fIterator.CurrentValuePointer();
	}

	inline Node* CurrentNode()
	{
		return fIterator.CurrentNode();
	}

	inline Value* Next()
	{
		fIterator.Next();
		return Current();
	}

	inline Value* Previous()
	{
		fIterator.Previous();
		return Current();
	}

	inline void Remove()
	{
		fIterator.Remove();
	}

private:
	friend class TWO_KEY_AVL_TREE_CLASS_NAME;

	Iterator(const Iterator& other)
		:
		fIterator(other.fIterator)
	{
	}

	Iterator& operator=(const Iterator& other)
	{
		fIterator = other.fIterator;
	}

	Iterator(const TreeMapIterator& iterator)
		:
		fIterator()
	{
	}

	inline void _SetTo(const TreeMapIterator& iterator)
	{
		fIterator = iterator;
	}

private:
	TWO_KEY_AVL_TREE_CLASS_NAME::TreeMapIterator fIterator;
};


TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::TwoKeyAVLTree()
	:
	fTreeMap(NodeStrategy(PrimaryKeyCompare(), SecondaryKeyCompare(),
		GetPrimaryKey(), GetSecondaryKey())),
	fPrimaryKeyCompare(PrimaryKeyCompare()),
	fGetPrimaryKey(GetPrimaryKey())
{
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::TwoKeyAVLTree(
	const PrimaryKeyCompare& primaryCompare, const GetPrimaryKey& getPrimary,
	const SecondaryKeyCompare& secondaryCompare,
	const GetSecondaryKey& getSecondary)
	:
	fTreeMap(NodeStrategy(primaryCompare, secondaryCompare, getPrimary,
		getSecondary)),
	fPrimaryKeyCompare(primaryCompare),
	fGetPrimaryKey(getPrimary)
{
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
TWO_KEY_AVL_TREE_CLASS_NAME::~TwoKeyAVLTree()
{
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
typename TWO_KEY_AVL_TREE_CLASS_NAME::Node*
TWO_KEY_AVL_TREE_CLASS_NAME::Previous(Node* node) const
{
	return fTreeMap.Previous(node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
typename TWO_KEY_AVL_TREE_CLASS_NAME::Node*
TWO_KEY_AVL_TREE_CLASS_NAME::Next(Node* node) const
{
	return fTreeMap.Next(node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value*
TWO_KEY_AVL_TREE_CLASS_NAME::FindFirst(const PrimaryKey& key,
	Iterator* iterator)
{
	const NodeStrategy& strategy = fTreeMap.GetNodeStrategy();

	Node* node = _FindFirst(key, NULL);
	if (node == NULL)
		return NULL;

	if (iterator != NULL)
		iterator->_SetTo(fTreeMap.GetIterator(node));

	return &strategy.GetValue(node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value*
TWO_KEY_AVL_TREE_CLASS_NAME::FindFirstClosest(const PrimaryKey& key, bool less,
	Iterator* iterator)
{
	const NodeStrategy& strategy = fTreeMap.GetNodeStrategy();

	Node* parent = NULL;
	Node* node = _FindFirst(key, &parent);
	if (node == NULL) {
		// not found -- try to get the closest node
		if (parent == NULL)
			return NULL;

		node = parent;
		int expectedCmp = less ? 1 : -1;
		int cmp = fPrimaryKeyCompare(key,
			fGetPrimaryKey(strategy.GetValue(strategy.GetNode(node))));

		if (cmp != expectedCmp) {
			// The node's value is less although we were asked for a greater
			// value, or the other way around. We need to iterate to the next
			// node in the respective direction. If there is no node, we fail.
			node = less ? Previous(node) : Next(node);
			if (node == NULL)
				return NULL;
		}
	}

	if (iterator != NULL)
		iterator->_SetTo(fTreeMap.GetIterator(node));

	return &strategy.GetValue(node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value*
TWO_KEY_AVL_TREE_CLASS_NAME::FindLast(const PrimaryKey& key,
	Iterator* iterator)
{
	const NodeStrategy& strategy = fTreeMap.GetNodeStrategy();
	Node* node = fTreeMap.RootNode();

	while (node) {
		int cmp = fPrimaryKeyCompare(key, fGetPrimaryKey(
			strategy.GetValue(node)));
		if (cmp == 0) {
			// found a matching node, now get the right-most node with that key
			while (node->right && fPrimaryKeyCompare(key,
				   	fGetPrimaryKey(strategy.GetValue(
						strategy.GetNode(node->right)))) == 0) {
				node = strategy.GetNode(node->right);
			}
			if (iterator)
				iterator->_SetTo(fTreeMap.GetIterator(node));
			return &strategy.GetValue(node);
		}

		if (cmp < 0)
			node = strategy.GetNode(node->left);
		else
			node = strategy.GetNode(node->right);
	}
	return NULL;
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
Value*
TWO_KEY_AVL_TREE_CLASS_NAME::Find(const PrimaryKey& primaryKey,
	const SecondaryKey& secondaryKey, Iterator* iterator)
{
	TreeMapIterator it = fTreeMap.Find(Key(primaryKey, secondaryKey));
	if (iterator)
		iterator->_SetTo(it);
	return it.CurrentValuePointer();
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
void
TWO_KEY_AVL_TREE_CLASS_NAME::GetIterator(Iterator* iterator)
{
	TreeMapIterator it = fTreeMap.GetIterator();
	it.Next();
		// Our iterator needs to point to the first entry already.
	iterator->_SetTo(it);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
void
TWO_KEY_AVL_TREE_CLASS_NAME::GetIterator(Node* node, Iterator* iterator)
{
	iterator->_SetTo(fTreeMap.GetIterator(node));
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Insert(const Value& value, Iterator* iterator)
{
	NodeStrategy& strategy
		= const_cast<NodeStrategy&>(fTreeMap.GetNodeStrategy());

	TreeMapIterator it;
	status_t status = fTreeMap.Insert(strategy.GetValueKey(value), value, &it);
	if (status != B_OK || !iterator)
		return status;

	iterator->_SetTo(it);
	return B_OK;
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Insert(const Value& value, Node** _node)
{
	NodeStrategy& strategy
		= const_cast<NodeStrategy&>(fTreeMap.GetNodeStrategy());

	return fTreeMap.Insert(strategy.GetValueKey(value), value, _node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Remove(const PrimaryKey& primaryKey,
	const SecondaryKey& secondaryKey)
{
	return fTreeMap.Remove(Key(primaryKey, secondaryKey));
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
status_t
TWO_KEY_AVL_TREE_CLASS_NAME::Remove(Node* node)
{
	return fTreeMap.Remove(node);
}


TWO_KEY_AVL_TREE_TEMPLATE_LIST
typename TWO_KEY_AVL_TREE_CLASS_NAME::Node*
TWO_KEY_AVL_TREE_CLASS_NAME::_FindFirst(const PrimaryKey& key,
	Node** _parent) const
{
	const NodeStrategy& strategy = fTreeMap.GetNodeStrategy();
	Node* node = fTreeMap.RootNode();
	Node* parent = NULL;

	while (node) {
		int cmp = fPrimaryKeyCompare(key, fGetPrimaryKey(
			strategy.GetValue(node)));
		if (cmp == 0) {
			// found a matching node, now get the left-most node with that key
			while (node->left && fPrimaryKeyCompare(key,
				   	fGetPrimaryKey(strategy.GetValue(
						strategy.GetNode(node->left)))) == 0) {
				node = strategy.GetNode(node->left);
			}

			return node;
		}

		parent = node;

		if (cmp < 0)
			node = strategy.GetNode(node->left);
		else
			node = strategy.GetNode(node->right);
	}

	if (_parent != NULL)
		*_parent = parent;

	return NULL;
}


#endif	// TWO_KEY_AVL_TREE_H
