/*
 * Copyright 2011, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "BMessageValueNode.h"

#include <new>

#include <AutoDeleter.h>
#include <Message.h>
#include <MessagePrivate.h>

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
	CompoundType* baseType = dynamic_cast<CompoundType*>(
		fType->ResolveRawType(false));
	AddressType* addressType = dynamic_cast<AddressType*>(fType);
	if (addressType) {
		baseType = dynamic_cast<CompoundType*>(addressType->BaseType()
			->ResolveRawType(false));
		error = valueLoader->LoadValue(location, valueType, false,
			addressData);
	} else
		addressData.SetTo(location->PieceAt(0).address);



	TRACE_LOCALS("BMessage: Address: 0x%" B_PRIx64 "\n",
		addressData.ToUInt64());

	ValueLocation* memberLocation = NULL;

	BVariant headerAddress;
	BVariant fieldAddress;
	BVariant dataAddress;
	BVariant what;

	for (int32 i = 0; i < baseType->CountDataMembers(); i++) {
		DataMember* member = baseType->DataMemberAt(i);
		if (strcmp(member->Name(), "fHeader") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			if (error != B_OK) {
				TRACE_LOCALS("BMessageValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of header member: %s\n",
						strerror(error));
				delete memberLocation;
				return error;
			}

			error = valueLoader->LoadValue(memberLocation, valueType,
				false, headerAddress);
			delete memberLocation;
			if (error != B_OK)
				return error;
		} else if (strcmp(member->Name(), "what") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			if (error != B_OK) {
				TRACE_LOCALS("BMessageValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of header member: %s\n",
						strerror(error));
				delete memberLocation;
				return error;
			}
			error = valueLoader->LoadValue(memberLocation, valueType,
				false, what);
			delete memberLocation;
			if (error != B_OK)
				return error;
		} else if (strcmp(member->Name(), "fFields") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			if (error != B_OK) {
				TRACE_LOCALS("BMessageValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of field member: %s\n",
						strerror(error));
				delete memberLocation;
				return error;
			}
			error = valueLoader->LoadValue(memberLocation, valueType,
				false, fieldAddress);
			delete memberLocation;
			if (error != B_OK)
				return error;
		} else if (strcmp(member->Name(), "fData") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			if (error != B_OK) {
				TRACE_LOCALS("BMessageValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of data member: %s\n",
						strerror(error));
				delete memberLocation;
				return error;
			}
			error = valueLoader->LoadValue(memberLocation, valueType,
				false, dataAddress);
			delete memberLocation;
			if (error != B_OK)
				return error;
		}
		memberLocation = NULL;
	}

	TRACE_LOCALS("BMessage: what: 0x%" B_PRIx32 ", result: %ld\n",
		what.ToUInt32(), error);

	uint8 headerBuffer[sizeof(BMessage::message_header)];
	error = valueLoader->LoadRawValue(headerAddress, sizeof(headerBuffer),
		headerBuffer);
	TRACE_LOCALS("BMessage: Header Address: 0x%" B_PRIx64 ", result: %ld\n",
		headerAddress.ToUInt64(), error);
	if (error != B_OK)
		return error;

	BMessage::message_header* header = (BMessage::message_header*)headerBuffer;
	header->what = what.ToUInt32();
	size_t fieldsSize = header->field_count * sizeof(BMessage::field_header);
	size_t totalMessageSize = sizeof(BMessage::message_header)
		+ fieldsSize + header->data_size;

	uint8* messageBuffer = new(std::nothrow)uint8[totalMessageSize];
	if (messageBuffer == NULL)
		return B_NO_MEMORY;

	memset(messageBuffer, 0, totalMessageSize);

	ArrayDeleter<uint8> deleter(messageBuffer);

	// This more or less follows the logic of BMessage::Flatten
	// in order to reconstruct the flattened buffer ; thus if the
	// internal representation of that changes, this will have to
	// follow suit
	uint8* currentBuffer = messageBuffer;
	memcpy(currentBuffer, header,
		sizeof(BMessage::message_header));

	if (fieldsSize > 0) {
		currentBuffer += sizeof(BMessage::message_header);
		error = valueLoader->LoadRawValue(fieldAddress, fieldsSize,
			currentBuffer);
		TRACE_LOCALS("BMessage: Field Header Address: 0x%" B_PRIx64
			", result: %ld\n",	headerAddress.ToUInt64(), error);
		if (error != B_OK)
			return error;

		headerAddress.SetTo(dataAddress);
		currentBuffer += fieldsSize;
		error = valueLoader->LoadRawValue(dataAddress, header->data_size,
			currentBuffer);
		TRACE_LOCALS("BMessage: Data Address: 0x%" B_PRIx64 ", result: %ld\n",
			headerAddress.ToUInt64(), error);
		if (error != B_OK)
			return B_ERROR;
	}

	BMessage message;
	error = message.Unflatten((const char *)messageBuffer);
	TRACE_LOCALS("BMessage: Unflatten result: %s\n", strerror(error));
	if (error != B_OK)
		return error;

	message.PrintToStream();

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
