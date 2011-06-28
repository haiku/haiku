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


// #pragma mark - BMessageWhatNodeChild


class BMessageWhatNodeChild : public ValueNodeChild {
public:
	BMessageWhatNodeChild(BMessageValueNode* parent, DataMember* member,
		Type* type)
		:
		ValueNodeChild(),
		fMember(member),
		fName("what"),
		fParent(parent),
		fType(type)
	{
		fParent->AcquireReference();
		fType->AcquireReference();
	}

	virtual ~BMessageWhatNodeChild()
	{
		fParent->ReleaseReference();
		fType->ReleaseReference();
	}

	virtual const BString& Name() const
	{
		return fName;
	}

	virtual Type* GetType() const
	{
		return fType;
	}

	virtual ValueNode* Parent() const
	{
		return fParent;
	}

	virtual status_t ResolveLocation(ValueLoader* valueLoader,
		ValueLocation*& _location)
	{
		ValueLocation* parentLocation = fParent->Location();
		ValueLocation* location;
		CompoundType* type = dynamic_cast<CompoundType*>(fParent->GetType());
		if (type == NULL)
			return B_BAD_VALUE;

		status_t error = type->ResolveDataMemberLocation(fMember,
			*parentLocation, location);
		if (error != B_OK)
			return error;

		_location = location;
		return B_OK;
	}

private:
	DataMember*			fMember;
	BString				fName;
	BMessageValueNode*	fParent;
	Type*				fType;
};


// #pragma mark - BMessageValueNode


BMessageValueNode::BMessageValueNode(ValueNodeChild* nodeChild,
	Type* type)
	:
	ValueNode(nodeChild),
	fType(type),
	fValid(false),
	fMessage()
{
	fType->AcquireReference();
}


BMessageValueNode::~BMessageValueNode()
{
	fType->ReleaseReference();
	for (int32 i = 0; i < fChildren.CountItems(); i++)
		fChildren.ItemAt(i)->ReleaseReference();
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
		BVariant address;
		baseType = dynamic_cast<CompoundType*>(addressType->BaseType()
			->ResolveRawType(false));
		error = valueLoader->LoadValue(location, valueType, false,
			address);
		if (error != B_OK)
			return error;

		ValuePieceLocation pieceLocation;
		pieceLocation.SetToMemory(address.ToUInt64());
		location->SetPieceAt(0, pieceLocation);
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

	fValid = true;
	CreateChildren();

	return B_OK;
}


status_t
BMessageValueNode::CreateChildren()
{
	// delay child creation until our location / value has been resolved
	// since we otherwise have no idea as to what value nodes to present
	if (!fValid)
		return B_BAD_VALUE;

	if (!fChildren.IsEmpty())
		return B_OK;

	Type* whatType = NULL;

	CompoundType* baseType = dynamic_cast<CompoundType*>(
		fType->ResolveRawType(false));
	if (baseType == NULL)
		return B_OK;

	DataMember* member;
	for (int32 i = 0; i < baseType->CountDataMembers(); i++) {
		member = baseType->DataMemberAt(i);
		if (strcmp(member->Name(), "what") == 0) {
			whatType = member->GetType();
			break;
		}
	}

	ValueNodeChild* whatNode =
		new(std::nothrow) BMessageWhatNodeChild(this, member, whatType);
	if (whatNode == NULL)
		return B_NO_MEMORY;

	whatNode->SetContainer(fContainer);
	fChildren.AddItem(whatNode);

	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0;
		fMessage.GetInfo(B_ANY_TYPE, i, &name, &type,
			&count) == B_OK; i++) {
		// TODO: split FieldHeaderNode into two variants in order to
		// present fields with a count of 1 without subindices.
		BMessageFieldHeaderNodeChild* node = new(std::nothrow)
			BMessageFieldHeaderNodeChild(this, name, type, count);
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
BMessageValueNode::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
BMessageValueNode::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}


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
	BMessageValueNode* parent, const BString &name, type_code type,
	int32 count)
	:
	ValueNodeChild(),
	fName(name),
	fType(parent->GetType()),
	fParent(parent),
	fFieldType(type),
	fFieldCount(count)
{
	fParent->AcquireReference();
	fType->AcquireReference();
}


BMessageValueNode::BMessageFieldHeaderNodeChild::~BMessageFieldHeaderNodeChild()
{
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
		BMessageFieldHeaderNode(this, fParent, fName, fFieldType,
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
