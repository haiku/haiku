/*
 * Copyright 2012, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "BListValueNode.h"

#include <new>

#include <AutoDeleter.h>

#include "AddressValueNode.h"
#include "Architecture.h"
#include "StringValue.h"
#include "Tracing.h"
#include "Type.h"
#include "TypeLookupConstraints.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// maximum number of array elements to show by default
static const int64 kMaxArrayElementCount = 20;


//#pragma mark - BListValueNode::BListElementNodeChild


class BListValueNode::BListElementNodeChild : public ValueNodeChild {
public:
								BListElementNodeChild(BListValueNode* parent,
									int64 elementIndex, Type* type);
	virtual						~BListElementNodeChild();

	virtual	const BString&		Name() const { return fName; };
	virtual	Type*				GetType() const { return fType; };
	virtual	ValueNode*			Parent() const { return fParent; };

	virtual status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
	Type*						fType;
	BListValueNode*				fParent;
	int64						fElementIndex;
	BString						fName;
};


BListValueNode::BListElementNodeChild::BListElementNodeChild(
	BListValueNode* parent, int64 elementIndex, Type* type)
	:
	ValueNodeChild(),
	fType(type),
	fParent(parent),
	fElementIndex(elementIndex),
	fName()
{
	fType->AcquireReference();
	fParent->AcquireReference();
	fName.SetToFormat("[%" B_PRId64 "]", fElementIndex);
}


BListValueNode::BListElementNodeChild::~BListElementNodeChild()
{
	fType->ReleaseReference();
	fParent->ReleaseReference();
}


status_t
BListValueNode::BListElementNodeChild::ResolveLocation(
	ValueLoader* valueLoader, ValueLocation*& _location)
{
	uint8 addressSize = valueLoader->GetArchitecture()->AddressSize();
	ValueLocation* location = new(std::nothrow) ValueLocation();
	if (location == NULL)
		return B_NO_MEMORY;


	uint64 listAddress = fParent->fDataLocation.ToUInt64();
	listAddress += fElementIndex * addressSize;

	ValuePieceLocation piece;
	piece.SetToMemory(listAddress);
	piece.SetSize(addressSize);
	location->AddPiece(piece);

	_location = location;
	return B_OK;
}


//#pragma mark - BListItemCountNodeChild

class BListValueNode::BListItemCountNodeChild : public ValueNodeChild {
public:
								BListItemCountNodeChild(BVariant location,
									BListValueNode* parent, Type* type);
	virtual						~BListItemCountNodeChild();

	virtual	const BString&		Name() const { return fName; };
	virtual	Type*				GetType() const { return fType; };
	virtual	ValueNode*			Parent() const { return fParent; };

	virtual status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
	Type*						fType;
	BListValueNode*				fParent;
	BVariant					fLocation;
	BString						fName;
};


BListValueNode::BListItemCountNodeChild::BListItemCountNodeChild(
	BVariant location, BListValueNode* parent, Type* type)
	:
	ValueNodeChild(),
	fType(type),
	fParent(parent),
	fLocation(location),
	fName("Capacity")
{
	fType->AcquireReference();
	fParent->AcquireReference();
}


BListValueNode::BListItemCountNodeChild::~BListItemCountNodeChild()
{
	fType->ReleaseReference();
	fParent->ReleaseReference();
}


status_t
BListValueNode::BListItemCountNodeChild::ResolveLocation(
	ValueLoader* valueLoader, ValueLocation*& _location)
{
	ValueLocation* location = new(std::nothrow) ValueLocation();
	if (location == NULL)
		return B_NO_MEMORY;

	ValuePieceLocation piece;
	piece.SetToMemory(fLocation.ToUInt64());
	piece.SetSize(sizeof(int32));
	location->AddPiece(piece);

	_location = location;
	return B_OK;
}


//#pragma mark - BListValueNode

BListValueNode::BListValueNode(ValueNodeChild* nodeChild,
	Type* type)
	:
	ValueNode(nodeChild),
	fType(type),
	fLoader(NULL),
	fItemCountType(NULL),
	fItemCount(0)
{
	fType->AcquireReference();
}


BListValueNode::~BListValueNode()
{
	fType->ReleaseReference();
	for (int32 i = 0; i < fChildren.CountItems(); i++)
		fChildren.ItemAt(i)->ReleaseReference();

	if (fItemCountType != NULL)
		fItemCountType->ReleaseReference();

	delete fLoader;
}


Type*
BListValueNode::GetType() const
{
	return fType;
}


status_t
BListValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
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
	CompoundType* baseType = dynamic_cast<CompoundType*>(fType);

	if (baseType->CountTemplateParameters() != 0) {
		// for BObjectList we need to walk up
		// the hierarchy: BObjectList -> _PointerList_ -> BList
		if (baseType->CountBaseTypes() == 0)
			return B_BAD_DATA;

		baseType = dynamic_cast<CompoundType*>(baseType->BaseTypeAt(0)
			->GetType());
		if (baseType == NULL || baseType->Name() != "_PointerList_")
			return B_BAD_DATA;

		if (baseType->CountBaseTypes() == 0)
			return B_BAD_DATA;

		baseType = dynamic_cast<CompoundType*>(baseType->BaseTypeAt(0)
			->GetType());
		if (baseType == NULL || baseType->Name() != "BList")
			return B_BAD_DATA;

	}

	for (int32 i = 0; i < baseType->CountDataMembers(); i++) {
		DataMember* member = baseType->DataMemberAt(i);
		if (strcmp(member->Name(), "fObjectList") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			BReference<ValueLocation> locationRef(memberLocation, true);
			if (error != B_OK) {
				TRACE_LOCALS(
					"BListValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of header member: %s\n",
					strerror(error));
				return error;
			}

			error = valueLoader->LoadValue(memberLocation, valueType,
				false, fDataLocation);
			if (error != B_OK)
				return error;
		} else if (strcmp(member->Name(), "fItemCount") == 0) {
			error = baseType->ResolveDataMemberLocation(member,
				*location, memberLocation);
			BReference<ValueLocation> locationRef(memberLocation, true);
			if (error != B_OK) {
				TRACE_LOCALS(
					"BListValueNode::ResolvedLocationAndValue(): "
					"failed to resolve location of header member: %s\n",
					strerror(error));
				return error;
			}

			fItemCountType = member->GetType();
			fItemCountType->AcquireReference();

			fItemCountLocation = memberLocation->PieceAt(0).address;

			BVariant listSize;
			error = valueLoader->LoadValue(memberLocation, valueType,
				false, listSize);
			if (error != B_OK)
				return error;

			fItemCount = listSize.ToInt32();
			TRACE_LOCALS(
				"BListValueNode::ResolvedLocationAndValue(): "
				"detected list size %" B_PRId32 "\n",
				fItemCount);
		}
		memberLocation = NULL;
	}

	if (fLoader == NULL) {
		fLoader = new(std::nothrow)ValueLoader(*valueLoader);
		if (fLoader == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
BListValueNode::CreateChildren()
{
	if (fChildrenCreated)
		return B_OK;

	if (fLocationResolutionState != B_OK)
		return fLocationResolutionState;

	if (fItemCountType != NULL) {
		BListItemCountNodeChild* countChild = new(std::nothrow)
			BListItemCountNodeChild(fItemCountLocation, this, fItemCountType);

		if (countChild == NULL)
			return B_NO_MEMORY;

		countChild->SetContainer(fContainer);
		fChildren.AddItem(countChild);
	}

	BReference<Type> addressTypeRef;
	Type* type = NULL;
	CompoundType* objectType = dynamic_cast<CompoundType*>(fType);
	if (objectType->CountTemplateParameters() != 0) {
		AddressType* addressType = NULL;
		status_t result = objectType->TemplateParameterAt(0)->GetType()
			->CreateDerivedAddressType(DERIVED_TYPE_POINTER, addressType);
		if (result != B_OK)
			return result;

		type = addressType;
		addressTypeRef.SetTo(type, true);
	} else {
		BString typeName;
		TypeLookupConstraints constraints;
		constraints.SetTypeKind(TYPE_ADDRESS);
		constraints.SetBaseTypeName("void");
		status_t result = fLoader->LookupTypeByName(typeName, constraints, type);
		if (result != B_OK)
			return result;
	}

	for (int32 i = 0; i < fItemCount && i < kMaxArrayElementCount; i++)
	{
		BListElementNodeChild* child =
			new(std::nothrow) BListElementNodeChild(this, i, type);
		if (child == NULL)
			return B_NO_MEMORY;
		child->SetContainer(fContainer);
		fChildren.AddItem(child);
	}

	fChildrenCreated = true;

	if (fContainer != NULL)
		fContainer->NotifyValueNodeChildrenCreated(this);

	return B_OK;
}


int32
BListValueNode::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
BListValueNode::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}
