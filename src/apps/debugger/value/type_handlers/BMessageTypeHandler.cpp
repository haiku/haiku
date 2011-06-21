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
	AddressType* addressType = dynamic_cast<AddressType*>(type);
	CompoundType* baseType = dynamic_cast<CompoundType*>(type);
	ModifiedType* modifiedType = NULL;
	if (addressType != NULL && addressType->AddressKind()
		== DERIVED_TYPE_POINTER) {
			baseType = dynamic_cast<CompoundType*>(
				addressType->BaseType());
		if (baseType == NULL) {
			modifiedType = dynamic_cast<ModifiedType*>(
				addressType->BaseType());
		}
	}

	if (baseType == NULL && modifiedType == NULL)
		return 0.0f;
	else if (modifiedType != NULL) {
		baseType = dynamic_cast<CompoundType*>(
			modifiedType->ResolveRawType(false));
		if (baseType == NULL)
			return 0.0f;
	}

	if (baseType->ResolveRawType(true)->Name() == "BMessage")
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
