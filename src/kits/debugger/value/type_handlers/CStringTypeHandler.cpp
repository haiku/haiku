/*
 * Copyright 2010, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "CStringTypeHandler.h"

#include <new>

#include <stdio.h>

#include "CStringValueNode.h"
#include "Type.h"


CStringTypeHandler::~CStringTypeHandler()
{
}


float
CStringTypeHandler::SupportsType(Type* type)
{
	AddressType* addressType = dynamic_cast<AddressType*>(type);
	ArrayType* arrayType = dynamic_cast<ArrayType*>(type);
	PrimitiveType* baseType = NULL;
	ModifiedType* modifiedType = NULL;
	if (addressType != NULL && addressType->AddressKind()
		== DERIVED_TYPE_POINTER) {
			baseType = dynamic_cast<PrimitiveType*>(
				addressType->BaseType());
		if (baseType == NULL) {
			modifiedType = dynamic_cast<ModifiedType*>(
				addressType->BaseType());
		}
	} else if (arrayType != NULL && arrayType->CountDimensions() == 1) {
		baseType = dynamic_cast<PrimitiveType*>(
				arrayType->BaseType());
		if (baseType == NULL) {
			modifiedType = dynamic_cast<ModifiedType*>(
				arrayType->BaseType());
		}
	}

	if (baseType == NULL && modifiedType == NULL)
		return 0.0f;
	else if (modifiedType != NULL) {
		baseType = dynamic_cast<PrimitiveType*>(
			modifiedType->ResolveRawType(false));
		if (baseType == NULL)
			return 0.0f;
	}

	if (baseType->TypeConstant() == B_UINT8_TYPE
		|| baseType->TypeConstant() == B_INT8_TYPE)
		return 0.8f;

	return 0.0f;
}


status_t
CStringTypeHandler::CreateValueNode(ValueNodeChild* nodeChild, Type* type,
	ValueNode*& _node)
{
	if (SupportsType(type) == 0.0f)
		return B_BAD_VALUE;

	ValueNode* node = new(std::nothrow) CStringValueNode(nodeChild,
		type);

	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;

	return B_OK;
}
