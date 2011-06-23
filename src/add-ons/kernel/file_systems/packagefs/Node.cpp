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
	fName(NULL)
{
	rw_lock_init(&fLock, "packagefs node");
}


Node::~Node()
{
PRINT("%p->Node::~Node()\n", this);
	free(fName);
	rw_lock_destroy(&fLock);
}


status_t
Node::Init(Directory* parent, const char* name)
{
	fParent = parent;
	fName = strdup(name);
	if (fName == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}
