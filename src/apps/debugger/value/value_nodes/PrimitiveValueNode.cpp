/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PrimitiveValueNode.h"

#include <new>

#include "BoolValue.h"
#include "FloatValue.h"
#include "IntegerValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"


PrimitiveValueNode::PrimitiveValueNode(ValueNodeChild* nodeChild,
	PrimitiveType* type)
	:
	ChildlessValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


PrimitiveValueNode::~PrimitiveValueNode()
{
	fType->ReleaseReference();
}


Type*
PrimitiveValueNode::GetType() const
{
	return fType;
}


status_t
PrimitiveValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	// get the value type
	type_code valueType = fType->TypeConstant();
	if (!BVariant::TypeIsNumber(valueType) && valueType != B_BOOL_TYPE) {
		TRACE_LOCALS("  -> unknown type constant\n");
		return B_UNSUPPORTED;
	}

	bool shortValueIsFine = BVariant::TypeIsInteger(valueType)
		|| valueType == B_BOOL_TYPE;

	TRACE_LOCALS("  TYPE_PRIMITIVE: '%c%c%c%c'\n",
		int(valueType >> 24), int(valueType >> 16),
		int(valueType >> 8), int(valueType));

	// load the value data
	BVariant valueData;
	status_t error = valueLoader->LoadValue(location, valueType,
		shortValueIsFine, valueData);
	if (error != B_OK)
		return error;

	// create the type object
	Value* value;
	if (valueType == B_BOOL_TYPE)
		value = new(std::nothrow) BoolValue(valueData.ToBool());
	else if (BVariant::TypeIsInteger(valueType))
		value = new(std::nothrow) IntegerValue(valueData);
	else if (BVariant::TypeIsFloat(valueType))
		value = new(std::nothrow) FloatValue(valueData.ToDouble());
	else
		return B_UNSUPPORTED;

	if (value == NULL)
		return B_NO_MEMORY;

	location->AcquireReference();
	_location = location;
	_value = value;
	return B_OK;
}
