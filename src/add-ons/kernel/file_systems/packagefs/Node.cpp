/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Node.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"


Node::Node(ino_t id)
	:
	fID(id),
	fParent(NULL),
	fName(NULL),
	fFlags(0)
{
	rw_lock_init(&fLock, "packagefs node");
}


Node::~Node()
{
	if ((fFlags & NODE_FLAG_OWNS_NAME) != 0)
		free(fName);

	rw_lock_destroy(&fLock);
}


status_t
Node::Init(Directory* parent, const char* name, uint32 flags)
{
	fParent = parent;
	fFlags = flags;

	if ((flags & NODE_FLAG_CONST_NAME) != 0
		|| (flags & NODE_FLAG_KEEP_NAME) != 0) {
		fName = const_cast<char*>(name);
	} else {
		fName = strdup(name);
		if (fName == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		fFlags |= NODE_FLAG_OWNS_NAME;
	}

	return B_OK;
}


status_t
Node::VFSInit(dev_t deviceID)
{
	return B_OK;
}


void
Node::VFSUninit()
{
}


void
Node::SetID(ino_t id)
{
	fID = id;
}


void
Node::SetParent(Directory* parent)
{
	fParent = parent;
}
