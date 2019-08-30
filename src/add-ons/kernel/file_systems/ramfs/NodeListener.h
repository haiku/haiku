/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef NODE_LISTENER_H
#define NODE_LISTENER_H

class Node;

// listening flags
enum {
	NODE_LISTEN_ANY_NODE	= 0x01,
	NODE_LISTEN_ADDED		= 0x02,
	NODE_LISTEN_REMOVED		= 0x04,
	NODE_LISTEN_ALL			= NODE_LISTEN_ADDED | NODE_LISTEN_REMOVED,
};

class NodeListener {
public:
	NodeListener();
	virtual ~NodeListener();

	virtual void NodeAdded(Node *node);
	virtual void NodeRemoved(Node *node);
};

#endif	// NODE_LISTENER_H
