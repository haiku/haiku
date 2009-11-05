/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AddressValueNode.h"

#include <new>

#include "AddressValue.h"
#include "Architecture.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// #pragma mark - AddressValueNode


AddressValueNode::AddressValueNode(ValueNodeChild* nodeChild,
	AddressType* type)
	:
	ValueNode(nodeChild),
	fType(type),
	fChild(NULL)
{
	fType->AcquireReference();
}


AddressValueNode::~AddressValueNode()
{
	if (fChild != NULL)
		fChild->ReleaseReference();
	fType->ReleaseReference();
}


Type*
AddressValueNode::GetType() const
{
	return fType;
}


status_t
AddressValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	TRACE_LOCALS("  TYPE_ADDRESS\n");

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
	BVariant valueData;
	status_t error = valueLoader->LoadValue(location, valueType, false,
		valueData);
	if (error != B_OK)
		return error;

	// create the type object
	Value* value = new(std::nothrow) AddressValue(valueData);
	if (value == NULL)
		return B_NO_MEMORY;

	location->AcquireReference();
	_location = location;
	_value = value;
	return B_OK;
}


status_t
AddressValueNode::CreateChildren()
{
	if (fChild != NULL)
		return B_OK;

	// construct name
	BString name = "*";
	name << Name();

	// create the child
	fChild = new(std::nothrow) AddressValueNodeChild(this, name,
		fType->BaseType());
	if (fChild == NULL)
		return B_NO_MEMORY;

	fChild->SetContainer(fContainer);

	if (fContainer != NULL)
		fContainer->NotifyValueNodeChildrenCreated(this);

	return B_OK;
}


int32
AddressValueNode::CountChildren() const
{
	return fChild != NULL ? 1 : 0;
}


ValueNodeChild*
AddressValueNode::ChildAt(int32 index) const
{
	return index == 0 ? fChild : NULL;
}


// #pragma mark - AddressValueNodeChild


AddressValueNodeChild::AddressValueNodeChild(AddressValueNode* parent,
	const BString& name, Type* type)
	:
	fParent(parent),
	fName(name),
	fType(type)
{
	fType->AcquireReference();
}


AddressValueNodeChild::~AddressValueNodeChild()
{
	fType->ReleaseReference();
}


const BString&
AddressValueNodeChild::Name() const
{
	return fName;
}


Type*
AddressValueNodeChild::GetType() const
{
	return fType;
}


ValueNode*
AddressValueNodeChild::Parent() const
{
	return fParent;
}


status_t
AddressValueNodeChild::ResolveLocation(ValueLoader* valueLoader,
	ValueLocation*& _location)
{
	// The parent's value is an address pointing to this component.
	AddressValue* parentValue = dynamic_cast<AddressValue*>(
		fParent->GetValue());
	if (parentValue == NULL)
		return B_BAD_VALUE;

	// resolve the location
	ValueLocation* location;
	status_t error = fType->ResolveObjectDataLocation(parentValue->ToUInt64(),
		location);
	if (error != B_OK) {
		TRACE_LOCALS("AddressValueNodeChild::ResolveLocation(): "
			"ResolveObjectDataLocation() failed: %s\n", strerror(error));
		return error;
	}

	_location = location;
	return B_OK;
}
