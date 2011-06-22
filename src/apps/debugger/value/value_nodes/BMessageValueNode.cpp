/*
 * Copyright 2011, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "BMessageValueNode.h"

#include <new>

#include <Message.h>

#include "Architecture.h"
#include "StringValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// #pragma mark - BMessageValueNode


BMessageValueNode::BMessageValueNode(ValueNodeChild* nodeChild,
	Type* type)
	:
	ValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


BMessageValueNode::~BMessageValueNode()
{
	fType->ReleaseReference();
}


Type*
BMessageValueNode::GetType() const
{
	return fType;
}


status_t
BMessageValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	TRACE_LOCALS("  TYPE_ADDRESS (BMessage)\n");

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
	status_t error = B_OK;
	if (dynamic_cast<AddressType*>(fType) != NULL) {
		error = valueLoader->LoadValue(location, valueType, false,
			addressData);
	} else
		addressData.SetTo(location->PieceAt(0).address);


	TRACE_LOCALS("Address: 0x%" B_PRIx64 "\n", addressData.ToUInt64());

	uint8 buffer[sizeof(BMessage)];
	error = valueLoader->LoadRawValue(addressData, sizeof(BMessage), buffer);

#ifdef TRACE_LOCALS
	if (error == B_OK) {
		BMessage* message = (BMessage*)buffer;
		TRACE_LOCALS("Loaded BMessage('%c%c%c%c')\n",
			char((message->what >> 24) & 0xff),
			char((message->what >> 16) & 0xff),
			char((message->what >> 8) & 0xff),
			char(message->what & 0xff));
	}
#endif

	return B_BAD_VALUE;
}


status_t
BMessageValueNode::CreateChildren()
{
	return B_OK;
}


int32
BMessageValueNode::CountChildren() const
{
	return 0;
}


ValueNodeChild*
BMessageValueNode::ChildAt(int32 index) const
{
	return NULL;
}
