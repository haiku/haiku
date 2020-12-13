/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LRU_CACHE_H
#define LRU_CACHE_H


#include <HashMap.h>


namespace BPrivate {


template<typename Key, typename Value>
class LRUOrderingNode {
private:
	typedef LRUOrderingNode<Key, Value> LRUNode;

public:
	LRUOrderingNode()
		:
		fKey(),
		fValue(),
		fOlder(NULL),
		fNewer(NULL)
	{
	}

	LRUOrderingNode(const Key& key, const Value& value)
		:
		fKey(key),
		fValue(value),
		fOlder(NULL),
		fNewer(NULL)
	{
	}

	Key						fKey;
	Value					fValue;
	LRUNode*				fOlder;
	LRUNode*				fNewer;
};


/*!	\brief This is a hash map that maintains a limited number of entries.  Once
	this number of entries has been exceeded then it will start to discard
	entries.  The first entries to be discarded are the ones that are the least
	recently used; hence the prefix "LRU".
*/

template<typename Key, typename Value>
class LRUCache {
public:
	typedef LRUOrderingNode<Key, Value> LRUNode;

	LRUCache(int32 limit)
		:
		fNewestNode(NULL),
		fOldestNode(NULL),
		fLimit(limit)
	{
		if (fLimit < 0)
			fLimit = 0;
	}

	~LRUCache()
	{
		Clear();
	}

	status_t InitCheck() const
	{
		return fMap.InitCheck();
	}

	status_t Put(const Key& key, const Value& value)
	{
		LRUNode* node = fMap.Get(key);

		if (node != NULL) {
			if (node->fValue != value) {
				node->fValue = value;
				_DisconnectNodeAndMakeNewest(node);
			}
		} else {
			node = new(std::nothrow) LRUNode(key, value);
			if (node == NULL)
				return B_NO_MEMORY;
			status_t result = fMap.Put(key, node);
			if (result != B_OK) {
				delete node;
				return result;
			}
			_SetNewestNode(node);
			_PurgeExcess();
		}

		return B_OK;
	}

	Value Remove(const Key& key)
	{
		LRUNode* node = fMap.Get(key);
		if (node != NULL) {
			_DisconnectNode(node);
			Value result = node->fValue;
			fMap.Remove(key);
			delete node;
			return result;
		}
		return Value();
	}

	void Clear()
	{
		fMap.Clear();
		LRUNode* node = fNewestNode;
		while (node != NULL) {
			LRUNode *next = node->fOlder;
			delete node;
			node = next;
		}
	}

	Value Get(const Key& key)
	{
		LRUNode* node = fMap.Get(key);
		if (node != NULL) {
			_DisconnectNodeAndMakeNewest(node);
			return node->fValue;
		}
		return Value();
	}

	bool ContainsKey(const Key& key) const
	{
		return fMap.ContainsKey(key);
	}

	int32 Size() const
	{
		return fMap.Size();
	}

private:

	void _DisconnectNodeAndMakeNewest(LRUNode* node) {
		if (node != fNewestNode) {
			_DisconnectNode(node);
			node->fOlder = NULL;
			node->fNewer = NULL;
			_SetNewestNode(node);
		}
	}

	void _DisconnectNode(LRUNode* node)
	{
		LRUNode *older = node->fOlder;
		LRUNode *newer = node->fNewer;
		if (newer != NULL)
			newer->fOlder = older;
		if (older != NULL)
			older->fNewer = newer;
		if (fNewestNode == node)
			fNewestNode = older;
		if (fOldestNode == node)
			fOldestNode = newer;
	}

	void _SetNewestNode(LRUNode* node)
	{
		if (node != fNewestNode) {
			node->fOlder = fNewestNode;
			node->fNewer = NULL;
			if (fNewestNode != NULL)
				fNewestNode->fNewer = node;
			fNewestNode = node;
			if (fOldestNode == NULL)
				fOldestNode = node;
		}
	}

	void _PurgeOldestNode()
	{
		if (fOldestNode == NULL)
			debugger("attempt to purge oldest node but there is none to purge");
		Remove(fOldestNode->fKey);
	}

	void _PurgeExcess()
	{
		while(Size() > fLimit)
			_PurgeOldestNode();
	}

protected:
	HashMap<Key, LRUNode*>	fMap;
	LRUNode*				fNewestNode;
	LRUNode*				fOldestNode;

private:
	int32					fLimit;
};

}; // namespace BPrivate

using BPrivate::LRUCache;

#endif // LRU_CACHE_H
