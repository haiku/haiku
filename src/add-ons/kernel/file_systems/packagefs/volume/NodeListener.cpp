/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "NodeListener.h"


// #pragma mark - OldNodeAttributes


OldNodeAttributes::~OldNodeAttributes()
{
}


void*
OldNodeAttributes::IndexCookieForAttribute(const char* name) const
{
	return NULL;
}


// #pragma mark - NodeListener


NodeListener::NodeListener()
	:
	fPrevious(this),
	fNext(this),
	fNode(NOT_LISTENING_NODE)
{
}


NodeListener::~NodeListener()
{
}


void
NodeListener::NodeAdded(Node* node)
{
}


void
NodeListener::NodeRemoved(Node* node)
{
}


void
NodeListener::NodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
}
