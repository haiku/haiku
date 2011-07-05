/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_LISTENER_H
#define NODE_LISTENER_H


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


class Node;


#define NOT_LISTENING_NODE	((Node*)~(addr_t)0)


class NodeListener {
public:
								NodeListener();
	virtual						~NodeListener();

	virtual	void				NodeAdded(Node* node);
	virtual	void				NodeRemoved(Node* node);
	virtual	void				NodeChanged(Node* node, uint32 statFields);

			void				StartedListening(Node* node)
									{ fNode = node; }
			void				StoppedListening()
									{ fNode = NOT_LISTENING_NODE; }
			bool				IsListening() const
									{ return fNode != NOT_LISTENING_NODE; }
			Node*				ListenedNode() const
									{ return fNode; }

	inline	void				AddNodeListener(NodeListener* listener);
	inline	NodeListener*		RemoveNodeListener();

			NodeListener*		PreviousNodeListener() const
									{ return fPrevious; }
			NodeListener*		NextNodeListener() const
									{ return fNext; }

			NodeListener*&		NodeListenerHashLink()
									{ return fHashLink; }

private:
			NodeListener*		fHashLink;
			NodeListener*		fPrevious;
			NodeListener*		fNext;
			Node*				fNode;
};


inline void
NodeListener::AddNodeListener(NodeListener* listener)
{
	listener->fPrevious = this;
	listener->fNext = fNext;

	fNext->fPrevious = listener;
	fNext = listener;
}


inline NodeListener*
NodeListener::RemoveNodeListener()
{
	if (fNext == this)
		return NULL;

	NodeListener* next = fNext;

	fPrevious->fNext = next;
	next->fPrevious = fPrevious;

	fPrevious = fNext = this;

	return next;
}



struct NodeListenerHashDefinition {
	typedef Node*			KeyType;
	typedef	NodeListener	ValueType;

	size_t HashKey(Node* key) const
	{
		return (size_t)key;
	}

	size_t Hash(const NodeListener* value) const
	{
		return HashKey(value->ListenedNode());
	}

	bool Compare(Node* key, const NodeListener* value) const
	{
		return key == value->ListenedNode();
	}

	NodeListener*& GetLink(NodeListener* value) const
	{
		return value->NodeListenerHashLink();
	}
};


typedef DoublyLinkedList<NodeListener> NodeListenerList;
typedef BOpenHashTable<NodeListenerHashDefinition> NodeListenerHashTable;


#endif	// NODE_LISTENER_H
