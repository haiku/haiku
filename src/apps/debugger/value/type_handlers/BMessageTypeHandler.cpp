/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BMessageTypeHandler.h"

#include <new>

#include "BMessageValueNode.h"
#include "Type.h"


BMessageTypeHandler::~BMessageTypeHandler()
{
}


float
BMessageTypeHandler::SupportsType(Type* type)
{
	if (dynamic_cast<CompoundType*>(type) != NULL
		&& type->Name() == "BMessage")
		return 1.0f;

	return 0.0f;
}


status_t
BMessageTypeHandler::CreateValueNode(ValueNodeChild* nodeChild, Type* type,
	ValueNode*& _node)
{
	if (SupportsType(type) == 0.0f)
		return B_BAD_VALUE;

	ValueNode* node = new(std::nothrow) BMessageValueNode(nodeChild,
		type);

	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;

	return B_OK;
}
