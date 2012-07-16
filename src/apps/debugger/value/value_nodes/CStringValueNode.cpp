/*
 * Copyright 2010, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "CStringValueNode.h"

#include <new>

#include "Architecture.h"
#include "StringValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// #pragma mark - CStringValueNode


CStringValueNode::CStringValueNode(ValueNodeChild* nodeChild,
	Type* type)
	:
	ChildlessValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


CStringValueNode::~CStringValueNode()
{
	fType->ReleaseReference();
}


Type*
CStringValueNode::GetType() const
{
	return fType;
}


status_t
CStringValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	TRACE_LOCALS("  TYPE_ADDRESS (C string)\n");

	// get the value type
	type_code valueType;
	if (valueLoader->GetArchitecture()->AddressSize() == 4) {
		valueType = B_UINT32_TYPE;
		TRACE_LOCALS("    -> 32 bit\n");
	} else {
		valueType = B_UINT64_TYPE;
		TRACE_LOCALS("    -> 64 bit\n");
	}

	// load the value data

	BVariant addressData;
	BString valueData;
	status_t error = B_OK;
	size_t maxSize = 255;
	if (dynamic_cast<AddressType*>(fType) != NULL) {
		error = valueLoader->LoadValue(location, valueType, false,
			addressData);
		if (error != B_OK)
			return error;
	} else {
		addressData.SetTo(location->PieceAt(0).address);
		maxSize = dynamic_cast<ArrayType*>(fType)
			->DimensionAt(0)->CountElements();
	}

	ValuePieceLocation piece;
	piece.SetToMemory(addressData.ToUInt64());

	ValueLocation* stringLocation = new(std::nothrow) ValueLocation(
		valueLoader->GetArchitecture()->IsBigEndian(), piece);

	if (stringLocation == NULL)
		return B_NO_MEMORY;

	BReference<ValueLocation> locationReference(stringLocation, true);

	error = valueLoader->LoadStringValue(addressData, maxSize, valueData);
	if (error != B_OK)
		return error;

	// create the type object
	Value* value = new(std::nothrow) StringValue(valueData);
	if (value == NULL)
		return B_NO_MEMORY;

	NodeChild()->SetLocation(stringLocation, B_OK);
	_location = locationReference.Detach();
	_value = value;
	return B_OK;
}
