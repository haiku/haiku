/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EnumerationValueNode.h"

#include <new>

#include "EnumerationValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"


EnumerationValueNode::EnumerationValueNode(ValueNodeChild* nodeChild,
	EnumerationType* type)
	:
	ChildlessValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


EnumerationValueNode::~EnumerationValueNode()
{
	fType->ReleaseReference();
}


Type*
EnumerationValueNode::GetType() const
{
	return fType;
}


status_t
EnumerationValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	TRACE_LOCALS("  TYPE_ENUMERATION\n");

	// get the value type
	type_code valueType = 0;

	// If a base type is known, try that.
	if (PrimitiveType* baseType = dynamic_cast<PrimitiveType*>(
			fType->BaseType())) {
		valueType = baseType->TypeConstant();
		if (!BVariant::TypeIsInteger(valueType))
			valueType = 0;
	}

	// If we don't have a value type yet, guess it from the type size.
	if (valueType == 0) {
		// TODO: This is C source language specific!
		switch (fType->ByteSize()) {
			case 1:
				valueType = B_INT8_TYPE;
				break;
			case 2:
				valueType = B_INT16_TYPE;
				break;
			case 4:
			default:
				valueType = B_INT32_TYPE;
				break;
			case 8:
				valueType = B_INT64_TYPE;
				break;
		}
	}

	// load the value data
	BVariant valueData;
	status_t error = valueLoader->LoadValue(location, valueType, true,
		valueData);
	if (error != B_OK)
		return error;

	// create the type object
	Value* value = new(std::nothrow) EnumerationValue(fType, valueData);
	if (value == NULL)
		return B_NO_MEMORY;

	location->AcquireReference();
	_location = location;
	_value = value;
	return B_OK;
}
