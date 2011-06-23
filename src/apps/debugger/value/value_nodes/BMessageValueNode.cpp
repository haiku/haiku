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


// minimal replica of BMessage's class structure for
// extracting fields
class BMessageValue {
	public:
		uint32			what;

		virtual			~BMessageValue();

		struct message_header;
		struct field_header;

		message_header*	fHeader;
		field_header*	fFields;
		uint8*			fData;

		uint32			fFieldsAvailable;
		size_t			fDataAvailable;

		mutable	BMessage* fOriginal;

		BMessage*		fQueueLink;
			// fQueueLink is used by BMessageQueue to build a linked list

		void*			fArchivingPointer;

		uint32			fReserved[8];

		virtual	void	_ReservedMessage1();
		virtual	void	_ReservedMessage2();
		virtual	void	_ReservedMessage3();
};


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


	TRACE_LOCALS("BMessage: Address: 0x%" B_PRIx64 "\n",
		addressData.ToUInt64());

	uint8 classBuffer[sizeof(BMessage)];
	error = valueLoader->LoadRawValue(addressData, sizeof(BMessage),
		classBuffer);
	if (error != B_OK)
		return error;

	BVariant headerAddress;
	BMessageValue* value = (BMessageValue*)classBuffer;
	headerAddress.SetTo((target_addr_t)value->fHeader);

	uint8 headerBuffer[sizeof(BMessage::message_header)];
	error = valueLoader->LoadRawValue(headerAddress, sizeof(headerBuffer),
		headerBuffer);
	TRACE_LOCALS("BMessage: Header Address: 0x%" B_PRIx64 ", result: %ld\n",
		headerAddress.ToUInt64(), error);
	if (error != B_OK)
		return error;

	BMessage::message_header* header = (BMessage::message_header*)headerBuffer;
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
	header->what = value->what;
	uint8* currentBuffer = messageBuffer;
	memcpy(currentBuffer, header,
		sizeof(BMessage::message_header));

	if (fieldsSize > 0) {
		headerAddress.SetTo((target_addr_t)value->fFields);
		currentBuffer += sizeof(BMessage::message_header);
		error = valueLoader->LoadRawValue(headerAddress, fieldsSize,
			currentBuffer);
		TRACE_LOCALS("BMessage: Field Header Address: 0x%" B_PRIx64
			", result: %ld\n",	headerAddress.ToUInt64(), error);
		if (error != B_OK)
			return error;

		headerAddress.SetTo((target_addr_t)value->fData);
		currentBuffer += fieldsSize;
		error = valueLoader->LoadRawValue(headerAddress, header->data_size,
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
