/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BListTypeHandler.h"

#include <new>

#include "BListValueNode.h"
#include "Type.h"


BListTypeHandler::~BListTypeHandler()
{
}


float
BListTypeHandler::SupportsType(Type* type)
{
	if (dynamic_cast<CompoundType*>(type) != NULL
		&& (type->Name() == "BList"
			|| type->Name().Compare("BObjectList", 11) == 0))
		return 1.0f;

	return 0.0f;
}


status_t
BListTypeHandler::CreateValueNode(ValueNodeChild* nodeChild, Type* type,
	ValueNode*& _node)
{
	if (SupportsType(type) == 0.0f)
		return B_BAD_VALUE;

	ValueNode* node = new(std::nothrow) BListValueNode(nodeChild,
		type);

	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;

	return B_OK;
}
