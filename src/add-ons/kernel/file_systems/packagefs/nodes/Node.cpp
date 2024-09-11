/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Node.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"
#include "EmptyAttributeDirectoryCookie.h"


DEFINE_INLINE_REFERENCEABLE_METHODS(Node, fReferenceable);


Node::Node(ino_t id)
	:
	fID(id),
	fParent(NULL),
	fName(),
	fFlags(0)
{
}


Node::~Node()
{
}


status_t
Node::Init(const String& name)
{
	fName = name;
	fFlags = 0;
	return B_OK;
}


void
Node::SetID(ino_t id)
{
	fID = id;
}


void
Node::_SetParent(Directory* parent)
{
	fParent = parent;
}


status_t
Node::VFSInit(dev_t deviceID)
{
	fFlags |= NODE_FLAG_KNOWN_TO_VFS;
	return B_OK;
}


void
Node::VFSUninit()
{
	fFlags &= ~(uint32)NODE_FLAG_KNOWN_TO_VFS;
}


uid_t
Node::UserID() const
{
	return 0;
}


gid_t
Node::GroupID() const
{
	return 0;
}


status_t
Node::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	AttributeDirectoryCookie* cookie
		= new(std::nothrow) EmptyAttributeDirectoryCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
Node::OpenAttribute(const StringKey& name, int openMode,
	AttributeCookie*& _cookie)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
Node::IndexAttribute(AttributeIndexer* indexer)
{
	return B_NOT_SUPPORTED;
}


void*
Node::IndexCookieForAttribute(const StringKey& name) const
{
	return NULL;
}
