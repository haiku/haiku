/*
 * Copyright 2011, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "BMessageValueNode.h"

#include <new>

#include <AutoDeleter.h>
#include <MessagePrivate.h>

#include "Architecture.h"
#include "StringValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// #pragma mark - BMessageValueNode::BMessageFieldHeaderNode

BMessageValueNode::BMessageFieldHeaderNode::BMessageFieldHeaderNode(
	BMessageFieldHeaderNodeChild *child, BMessageValueNode* parent,
	const BString &name, type_code type, int32 count)
	:
	ValueNode(child),
	fName(name),
	fType(parent->GetType()),
	fParent(parent),
	fFieldType(type),
	fFieldCount(count)
{
	fParent->AcquireReference();
	fType->AcquireReference();
}


BMessageValueNode::BMessageFieldHeaderNode::~BMessageFieldHeaderNode()
{
	fParent->ReleaseReference();
	fType->ReleaseReference();
}


Type*
BMessageValueNode::BMessageFieldHeaderNode::GetType() const
{
	return fType;
}


status_t
BMessageValueNode::BMessageFieldHeaderNode::CreateChildren()
{
	return B_OK;
}


int32
BMessageValueNode::BMessageFieldHeaderNode::CountChildren() const
{
	return 0;
}

ValueNodeChild*
BMessageValueNode::BMessageFieldHeaderNode::ChildAt(int32 index) const
{
	return NULL;
}


status_t
BMessageValueNode::BMessageFieldHeaderNode::ResolvedLocationAndValue(
	ValueLoader* loader, ValueLocation *& _location, Value*& _value)
{
	_location = fParent->Location();
	_value = NULL;

	return B_OK;
}


// #pragma mark - BMessageValueNode::BMessageFieldHeaderNodeChild


BMessageValueNode::BMessageFieldHeaderNodeChild::BMessageFieldHeaderNodeChild(
	BMessageFieldNode* parent, BMessageValueNode* messageNode,
	const BString &name, type_code type, int32 count)
	:
	ValueNodeChild(),
	fName(name),
	fType(parent->GetType()),
	fMessageNode(messageNode),
	fParent(parent),
	fFieldType(type),
	fFieldCount(count)
{
	fMessageNode->AcquireReference();
	fParent->AcquireReference();
	fType->AcquireReference();
}


BMessageValueNode::BMessageFieldHeaderNodeChild::~BMessageFieldHeaderNodeChild()
{
	fMessageNode->ReleaseReference();
	fParent->ReleaseReference();
	fType->ReleaseReference();
}


const BString&
BMessageValueNode::BMessageFieldHeaderNodeChild::Name() const
{
	return fName;
}


Type*
BMessageValueNode::BMessageFieldHeaderNodeChild::GetType() const
{
	return fType;
}


ValueNode*
BMessageValueNode::BMessageFieldHeaderNodeChild::Parent() const
{
	return fParent;
}


bool
BMessageValueNode::BMessageFieldHeaderNodeChild::IsInternal() const
{
	return true;
}


status_t
BMessageValueNode::BMessageFieldHeaderNodeChild::CreateInternalNode(
	ValueNode*& _node)
{
	BMessageFieldHeaderNode* node = new(std::nothrow)
		BMessageFieldHeaderNode(this, fMessageNode, fName, fFieldType,
			fFieldCount);
	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;
	return B_OK;
}

status_t
BMessageValueNode::BMessageFieldHeaderNodeChild::ResolveLocation(
	ValueLoader* valueLoader, ValueLocation*& _location)
{
	_location = fParent->Location();

	return B_OK;
}

status_t
BMessageValueNode::BMessageFieldHeaderNodeChild::CreateChildren()
{
	return B_OK;
}

int32
BMessageValueNode::BMessageFieldHeaderNodeChild::CountChildren() const
{
	return 0;
}

ValueNodeChild*
BMessageValueNode::BMessageFieldHeaderNodeChild::ChildAt(int32 index) const
{
	return NULL;
}


// #pragma mark - BMessageValueNode::BMessageFieldNode


BMessageValueNode::BMessageFieldNode::BMessageFieldNode(
	BMessageFieldNodeChild *child, BMessageValueNode* parent)
	:
	ValueNode(child),
	fType(parent->GetType()),
	fParent(parent)
{
	fParent->AcquireReference();
	fType->AcquireReference();
}


BMessageValueNode::BMessageFieldNode::~BMessageFieldNode()
{
	fParent->ReleaseReference();
	fType->ReleaseReference();
}


Type*
BMessageValueNode::BMessageFieldNode::GetType() const
{
	return fType;
}


status_t
BMessageValueNode::BMessageFieldNode::CreateChildren()
{
	if (!fChildren.IsEmpty())
		return B_OK;

	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0;
		fParent->fMessage.GetInfo(B_ANY_TYPE, i, &name, &type,
			&count) == B_OK; i++) {
		BMessageFieldHeaderNodeChild* node = new(std::nothrow)
			BMessageFieldHeaderNodeChild(this, fParent, name, type, count);
		if (node == NULL)
			return B_NO_MEMORY;

		node->SetContainer(fContainer);
		fChildren.AddItem(node);
	}

	if (fContainer != NULL)
		fContainer->NotifyValueNodeChildrenCreated(this);

	return B_OK;
}


int32
BMessageValueNode::BMessageFieldNode::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
BMessageValueNode::BMessageFieldNode::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}


status_t
BMessageValueNode::BMessageFieldNode::ResolvedLocationAndValue(
	ValueLoader* loader, ValueLocation*& _location, Value*& _value)
{
	_location = fParent->Location();
	_value = NULL;

	return B_OK;
}


// #pragma mark - BMessageValueNode::BMessageFieldNodeChild


BMessageValueNode::BMessageFieldNodeChild::BMessageFieldNodeChild(
	BMessageValueNode* parent)
	:
	ValueNodeChild(),
	fType(parent->GetType()),
	fParent(parent),
	fName("Fields")
{
	fParent->AcquireReference();
	fType->AcquireReference();
}


BMessageValueNode::BMessageFieldNodeChild::~BMessageFieldNodeChild()
{
	fParent->ReleaseReference();
	fType->ReleaseReference();
}


const BString&
BMessageValueNode::BMessageFieldNodeChild::Name() const
{
	return fName;
}


Type*
BMessageValueNode::BMessageFieldNodeChild::GetType() const
{
	return fType;
}


ValueNode*
BMessageValueNode::BMessageFieldNodeChild::Parent() const
{
	return fParent;
}


bool
BMessageValueNode::BMessageFieldNodeChild::IsInternal() const
{
	return true;
}


status_t
BMessageValueNode::BMessageFieldNodeChild::CreateInternalNode(
	ValueNode*& _node)
{
	ValueNode* node = new(std::nothrow) BMessageFieldNode(this, fParent);
	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;
	return B_OK;
}


status_t
BMessageValueNode::BMessageFieldNodeChild::ResolveLocation(
	ValueLoader* valueLoader, ValueLocation*& _location)
{
	_location = fParent->Location();

	return B_OK;
}


// #pragma mark - BMessageValueNode


BMessageValueNode::BMessageValueNode(ValueNodeChild* nodeChild,
	Type* type)
	:
	ValueNode(nodeChild),
	fType(type),
	fMessage(),
	fFields(NULL)
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

	status_t error = B_OK;
	CompoundType* baseType = dynamic_cast<CompoundType*>(
		fType->ResolveRawType(false));
	AddressType* addressType = dynamic_cast<AddressType*>(fType);
	if (addressType != NULL) {
		baseType = dynamic_cast<CompoundType*>(addressType->BaseType()
			->ResolveRawType(false));
	}

	_location = location;
	_value = NULL;

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

	error = fMessage.Unflatten((const char *)messageBuffer);
	TRACE_LOCALS("BMessage: Unflatten result: %s\n", strerror(error));
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
BMessageValueNode::CreateChildren()
{
	if (fFields == NULL) {
		fFields = new(std::nothrow) BMessageFieldNodeChild(this);
		if (fFields == NULL)
			return B_NO_MEMORY;

		fFields->SetContainer(fContainer);
	}

	if (fContainer != NULL)
		fContainer->NotifyValueNodeChildrenCreated(this);

	return B_OK;
}


int32
BMessageValueNode::CountChildren() const
{
	if (fFields != NULL)
		return 1;
	return 0;
}


ValueNodeChild*
BMessageValueNode::ChildAt(int32 index) const
{
	if (index == 0)
		return fFields;

	return NULL;
}
