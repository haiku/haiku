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
#include "TypeLookupConstraints.h"
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
	fLoader(NULL),
	fHeader(NULL),
	fFields(NULL),
	fData(NULL)
{
	fType->AcquireReference();
}


BMessageValueNode::~BMessageValueNode()
{
	fType->ReleaseReference();
	for (int32 i = 0; i < fChildren.CountItems(); i++)
		fChildren.ItemAt(i)->ReleaseReference();

	delete fLoader;
	delete fHeader;
	delete[] fFields;
	delete[] fData;
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
	_location = location;
	_value = NULL;

	ValueLocation* memberLocation = NULL;

	BVariant headerAddress;
	BVariant fieldAddress;
	BVariant what;

	CompoundType* baseType = dynamic_cast<CompoundType*>(fType);

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
				false, fDataLocation);
			delete memberLocation;
			if (error != B_OK)
				return error;
		}
		memberLocation = NULL;
	}

	TRACE_LOCALS("BMessage: what: 0x%" B_PRIx32 ", result: %ld\n",
		what.ToUInt32(), error);


	fHeader = new(std::nothrow) BMessage::message_header();
	if (fHeader == NULL)
		return B_NO_MEMORY;
	error = valueLoader->LoadRawValue(headerAddress, sizeof(
		BMessage::message_header), fHeader);
	TRACE_LOCALS("BMessage: Header Address: 0x%" B_PRIx64 ", result: %ld\n",
		headerAddress.ToUInt64(), error);
	if (error != B_OK)
		return error;
	fHeader->what = what.ToUInt32();

	size_t fieldsSize = fHeader->field_count * sizeof(
		BMessage::field_header);
	size_t totalSize = sizeof(BMessage::message_header) + fieldsSize + fHeader->data_size;
	uint8* messageBuffer = new(std::nothrow) uint8[totalSize];
	if (messageBuffer == NULL)
		return B_NO_MEMORY;
	memset(messageBuffer, 0, totalSize);
	memcpy(messageBuffer, fHeader, sizeof(BMessage::message_header));
	uint8* tempBuffer = messageBuffer + sizeof(BMessage::message_header);
	if (fieldsSize > 0) {
		fFields = new(std::nothrow)
			BMessage::field_header[fHeader->field_count];
		if (fFields == NULL)
			return B_NO_MEMORY;

		error = valueLoader->LoadRawValue(fieldAddress, fieldsSize,
			fFields);
		TRACE_LOCALS("BMessage: Field Header Address: 0x%" B_PRIx64
			", result: %ld\n",	headerAddress.ToUInt64(), error);
		if (error != B_OK)
			return error;

		fData = new(std::nothrow) uint8[fHeader->data_size];
		if (fData == NULL)
			return B_NO_MEMORY;

		error = valueLoader->LoadRawValue(fDataLocation, fHeader->data_size,
			fData);
		TRACE_LOCALS("BMessage: Data Address: 0x%" B_PRIx64
			", result: %ld\n",	fDataLocation.ToUInt64(), error);
		if (error != B_OK)
			return error;
		memcpy(tempBuffer, fFields, fieldsSize);
		tempBuffer += fieldsSize;
		memcpy(tempBuffer, fData, fHeader->data_size);
	}

	error = fMessage.Unflatten((const char*)messageBuffer);
	delete[] messageBuffer;
	if (error != B_OK)
		return error;

	if (fLoader == NULL) {
		fLoader = new(std::nothrow)ValueLoader(*valueLoader);
		if (fLoader == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
BMessageValueNode::CreateChildren()
{
	if (!fChildren.IsEmpty())
		return B_OK;

	DataMember* member = NULL;
	Type* whatType = NULL;

	CompoundType* messageType = dynamic_cast<CompoundType*>(fType);
	for (int32 i = 0; i < messageType->CountDataMembers(); i++) {
		member = messageType->DataMemberAt(i);
		if (strcmp(member->Name(), "what") == 0) {
			whatType = member->GetType();
			break;
		}
	}

	ValueNodeChild* whatNode
		= new(std::nothrow) BMessageWhatNodeChild(this, member, whatType);
	if (whatNode == NULL)
		return B_NO_MEMORY;

	whatNode->SetContainer(fContainer);
	fChildren.AddItem(whatNode);

	char* name;
	type_code type;
	int32 count;
	Type* fieldType = NULL;
	for (int32 i = 0; fMessage.GetInfo(B_ANY_TYPE, i, &name, &type,
		&count) == B_OK; i++) {
		fieldType = NULL;

		_GetTypeForTypeCode(type, fieldType);

		BMessageFieldNodeChild* node = new(std::nothrow)
			BMessageFieldNodeChild(this,
				fieldType != NULL ? fieldType : fType, name, type,
				count);
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


status_t
BMessageValueNode::_GetTypeForTypeCode(type_code type,
	Type*& _type)
{
	BString typeName;
	TypeLookupConstraints constraints;

	switch(type) {
		case B_BOOL_TYPE:
			typeName = "bool";
			constraints.SetTypeKind(TYPE_PRIMITIVE);
			break;

		case B_INT8_TYPE:
			typeName = "int8";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_UINT8_TYPE:
			typeName = "uint8";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_INT16_TYPE:
			typeName = "int16";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_UINT16_TYPE:
			typeName = "uint16";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_INT32_TYPE:
			typeName = "int32";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_UINT32_TYPE:
			typeName = "uint32";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_INT64_TYPE:
			typeName = "int64";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_UINT64_TYPE:
			typeName = "uint64";
			constraints.SetTypeKind(TYPE_TYPEDEF);
			break;

		case B_FLOAT_TYPE:
			typeName = "float";
			constraints.SetTypeKind(TYPE_PRIMITIVE);
			break;

		case B_DOUBLE_TYPE:
			typeName = "double";
			constraints.SetTypeKind(TYPE_PRIMITIVE);
			break;

		case B_MESSAGE_TYPE:
			typeName = "BMessage";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_MESSENGER_TYPE:
			typeName = "BMessenger";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_POINT_TYPE:
			typeName = "BPoint";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_POINTER_TYPE:
			typeName = "";
			constraints.SetTypeKind(TYPE_ADDRESS);
			constraints.SetBaseTypeName("void");
			break;

		case B_RECT_TYPE:
			typeName = "BRect";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_REF_TYPE:
			typeName = "entry_ref";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_RGB_COLOR_TYPE:
			typeName = "rgb_color";
			constraints.SetTypeKind(TYPE_COMPOUND);
			break;

		case B_STRING_TYPE:
			typeName = "";
			constraints.SetTypeKind(TYPE_ARRAY);
			constraints.SetBaseTypeName("char");
			break;

		default:
			return B_BAD_VALUE;
			break;
	}

	return fLoader->LookupTypeByName(typeName, constraints, _type);
}


status_t
BMessageValueNode::_FindField(const char* name, type_code type,
	BMessage::field_header** result) const
{
	if (name == NULL)
		return B_BAD_VALUE;

	if (fHeader == NULL)
		return B_NO_INIT;

	if (fHeader->field_count == 0 || fFields == NULL || fData == NULL)
		return B_NAME_NOT_FOUND;

	uint32 hash = _HashName(name) % fHeader->hash_table_size;
	int32 nextField = fHeader->hash_table[hash];

	while (nextField >= 0) {
		BMessage::field_header* field = &fFields[nextField];
		if ((field->flags & FIELD_FLAG_VALID) == 0)
			break;

		if (strncmp((const char*)(fData + field->offset), name,
			field->name_length) == 0) {
			if (type != B_ANY_TYPE && field->type != type)
				return B_BAD_TYPE;

			*result = field;
			return B_OK;
		}

		nextField = field->next_field;
	}

	return B_NAME_NOT_FOUND;
}


uint32
BMessageValueNode::_HashName(const char* name) const
{
	char ch;
	uint32 result = 0;

	while ((ch = *name++) != 0) {
		result = (result << 7) ^ (result >> 24);
		result ^= ch;
	}

	result ^= result << 12;
	return result;
}


status_t
BMessageValueNode::_FindDataLocation(const char* name, type_code type,
	int32 index, ValueLocation& location) const
{
	BMessage::field_header* field = NULL;
	int32 offset = 0;
	int32 size = 0;
	status_t result = _FindField(name, type, &field);
	if (result != B_OK)
		return result;

	if (index < 0 || (uint32)index >= field->count)
		return B_BAD_INDEX;

	if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0) {
		size = field->data_size / field->count;
		offset = field->offset + field->name_length + index * size;
	} else {
		offset = field->offset + field->name_length;
		uint8 *pointer = fData + field->offset + field->name_length;
		for (int32 i = 0; i < index; i++) {
			pointer += *(uint32*)pointer + sizeof(uint32);
			offset += *(uint32*)pointer + sizeof(uint32);
		}

		size = *(uint32*)pointer;
		offset += sizeof(uint32);
	}

	ValuePieceLocation piece;
	piece.SetToMemory(fDataLocation.ToUInt64() + offset);
	piece.SetSize(size);
	location.Clear();
	location.AddPiece(piece);

	return B_OK;
}


// #pragma mark - BMessageValueNode::BMessageFieldNode


BMessageValueNode::BMessageFieldNode::BMessageFieldNode(
	BMessageFieldNodeChild *child, BMessageValueNode* parent,
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
	Type* type = NULL;
	status_t error = fParent->_GetTypeForTypeCode(fFieldType, type);
	if (error != B_OK)
		return error;
	for (int32 i = 0; i < fFieldCount; i++) {
		BMessageFieldNodeChild* child = new(std::nothrow)
			BMessageFieldNodeChild(fParent, type, fName, fFieldType, fFieldCount, i);

		if (child == NULL)
			return B_NO_MEMORY;

		if (fContainer != NULL)
			child->SetContainer(fContainer);

		fChildren.AddItem(child);
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
	ValueLoader* loader, ValueLocation *& _location, Value*& _value)
{
	_location = NULL;
	_value = NULL;

	return B_OK;
}


// #pragma mark - BMessageValueNode::BMessageFieldNodeChild


BMessageValueNode::BMessageFieldNodeChild::BMessageFieldNodeChild(
	BMessageValueNode* parent, Type* nodeType, const BString &name,
	type_code type, int32 count, int32 index)
	:
	ValueNodeChild(),
	fName(name),
	fPresentationName(name),
	fType(nodeType),
	fParent(parent),
	fFieldType(type),
	fFieldCount(count),
	fFieldIndex(index)
{
	fParent->AcquireReference();
	fType->AcquireReference();

	if (fFieldIndex >= 0)
		fPresentationName.SetToFormat("[%ld]", fFieldIndex);
}


BMessageValueNode::BMessageFieldNodeChild::~BMessageFieldNodeChild()
{
	fParent->ReleaseReference();
	fType->ReleaseReference();
}


const BString&
BMessageValueNode::BMessageFieldNodeChild::Name() const
{
	return fPresentationName;
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
	return fFieldCount > 1 && fFieldIndex == -1;
}


status_t
BMessageValueNode::BMessageFieldNodeChild::CreateInternalNode(
	ValueNode*& _node)
{
	BMessageFieldNode* node = new(std::nothrow)
		BMessageFieldNode(this, fParent, fName, fFieldType, fFieldCount);
	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;
	return B_OK;
}


status_t
BMessageValueNode::BMessageFieldNodeChild::ResolveLocation(
	ValueLoader* valueLoader, ValueLocation*& _location)
{
	_location = new(std::nothrow)ValueLocation();

	if (_location == NULL)
		return B_NO_MEMORY;

	return fParent->_FindDataLocation(fName, fFieldType, fFieldIndex >= 0
		? fFieldIndex : 0, *_location);
}


