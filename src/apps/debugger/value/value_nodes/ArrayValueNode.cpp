/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ArrayValueNode.h"

#include <new>

#include "Architecture.h"
#include "ArrayIndexPath.h"
#include "IntegerValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// maximum number of array elements to show by default
static const uint64 kMaxArrayElementCount = 10;


// #pragma mark - AbstractArrayValueNode


AbstractArrayValueNode::AbstractArrayValueNode(ValueNodeChild* nodeChild,
	ArrayType* type, int32 dimension)
	:
	ValueNode(nodeChild),
	fType(type),
	fDimension(dimension)
{
	fType->AcquireReference();
}


AbstractArrayValueNode::~AbstractArrayValueNode()
{
	fType->ReleaseReference();

	for (int32 i = 0; AbstractArrayValueNodeChild* child = fChildren.ItemAt(i);
			i++) {
		child->ReleaseReference();
	}
}


Type*
AbstractArrayValueNode::GetType() const
{
	return fType;
}


status_t
AbstractArrayValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	// get the location
	ValueLocation* location = NodeChild()->Location();
	if (location == NULL)
		return B_BAD_VALUE;

	location->AcquireReference();
	_location = location;
	_value = NULL;
	return B_OK;
}


status_t
AbstractArrayValueNode::CreateChildren()
{
	if (!fChildren.IsEmpty())
		return B_OK;

	TRACE_LOCALS("TYPE_ARRAY\n");

	int32 dimensionCount = fType->CountDimensions();
	bool isFinalDimension = fDimension + 1 == dimensionCount;

	ArrayDimension* dimension = fType->DimensionAt(fDimension);
	uint64 elementCount = dimension->CountElements();
	if (elementCount == 0 || elementCount > kMaxArrayElementCount)
		elementCount = kMaxArrayElementCount;

	// create children for the array elements
	for (int32 i = 0; i < (int32)elementCount; i++) {
		BString name(Name());
		name << '[' << i << ']';
		if (name.Length() <= Name().Length())
			return B_NO_MEMORY;

		AbstractArrayValueNodeChild* child;
		if (isFinalDimension) {
			child = new(std::nothrow) ArrayValueNodeChild(this, name, i,
				fType->BaseType());
		} else {
			child = new(std::nothrow) InternalArrayValueNodeChild(this, name, i,
				fType);
		}

		if (child == NULL || !fChildren.AddItem(child)) {
			delete child;
			return B_NO_MEMORY;
		}

		child->SetContainer(fContainer);
	}

	if (fContainer != NULL)
		fContainer->NotifyValueNodeChildrenCreated(this);

	return B_OK;
}


int32
AbstractArrayValueNode::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
AbstractArrayValueNode::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}


// #pragma mark - ArrayValueNode


ArrayValueNode::ArrayValueNode(ValueNodeChild* nodeChild, ArrayType* type)
	:
	AbstractArrayValueNode(nodeChild, type, 0)
{
}


ArrayValueNode::~ArrayValueNode()
{
}


// #pragma mark - InternalArrayValueNode


InternalArrayValueNode::InternalArrayValueNode(ValueNodeChild* nodeChild,
	ArrayType* type, int32 dimension)
	:
	AbstractArrayValueNode(nodeChild, type, dimension)
{
}


InternalArrayValueNode::~InternalArrayValueNode()
{
}


// #pragma mark - AbstractArrayValueNodeChild


AbstractArrayValueNodeChild::AbstractArrayValueNodeChild(
	AbstractArrayValueNode* parent, const BString& name, int64 elementIndex)
	:
	fParent(parent),
	fName(name),
	fElementIndex(elementIndex)
{
}


AbstractArrayValueNodeChild::~AbstractArrayValueNodeChild()
{
}


const BString&
AbstractArrayValueNodeChild::Name() const
{
	return fName;
}


ValueNode*
AbstractArrayValueNodeChild::Parent() const
{
	return fParent;
}


// #pragma mark - ArrayValueNodeChild


ArrayValueNodeChild::ArrayValueNodeChild(AbstractArrayValueNode* parent,
	const BString& name, int64 elementIndex, Type* type)
	:
	AbstractArrayValueNodeChild(parent, name, elementIndex),
	fType(type)
{
	fType->AcquireReference();
}


ArrayValueNodeChild::~ArrayValueNodeChild()
{
	fType->ReleaseReference();
}


Type*
ArrayValueNodeChild::GetType() const
{
	return fType;
}


status_t
ArrayValueNodeChild::ResolveLocation(ValueLoader* valueLoader,
	ValueLocation*& _location)
{
	// get the parent (== array) location
	ValueLocation* parentLocation = fParent->Location();
	if (parentLocation == NULL)
		return B_BAD_VALUE;

	// create an array index path
	ArrayType* arrayType = fParent->GetArrayType();
	int32 dimensionCount = arrayType->CountDimensions();

	// add dummy indices first -- we'll replace them on our way back through
	// our ancestors
	ArrayIndexPath indexPath;
	for (int32 i = 0; i < dimensionCount; i++) {
		if (!indexPath.AddIndex(0))
			return B_NO_MEMORY;
	}

	AbstractArrayValueNodeChild* child = this;
	for (int32 i = dimensionCount - 1; i >= 0; i--) {
		indexPath.SetIndexAt(i, child->ElementIndex());

		child = dynamic_cast<AbstractArrayValueNodeChild*>(
			child->ArrayParent()->NodeChild());
	}

	// resolve the element location
	ValueLocation* location;
	status_t error = arrayType->ResolveElementLocation(indexPath,
		*parentLocation, location);
	if (error != B_OK) {
		TRACE_LOCALS("ArrayValueNodeChild::ResolveLocation(): "
			"ResolveElementLocation() failed: %s\n", strerror(error));
		return error;
	}

	_location = location;
	return B_OK;
}


// #pragma mark - InternalArrayValueNodeChild


InternalArrayValueNodeChild::InternalArrayValueNodeChild(
	AbstractArrayValueNode* parent, const BString& name, int64 elementIndex,
	ArrayType* type)
	:
	AbstractArrayValueNodeChild(parent, name, elementIndex),
	fType(type)
{
	fType->AcquireReference();
}


InternalArrayValueNodeChild::~InternalArrayValueNodeChild()
{
	fType->ReleaseReference();
}


Type*
InternalArrayValueNodeChild::GetType() const
{
	return fType;
}


bool
InternalArrayValueNodeChild::IsInternal() const
{
	return true;
}


status_t
InternalArrayValueNodeChild::CreateInternalNode(ValueNode*& _node)
{
	ValueNode* node = new(std::nothrow) InternalArrayValueNode(this, fType,
		fParent->Dimension() + 1);
	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;
	return B_OK;
}


status_t
InternalArrayValueNodeChild::ResolveLocation(ValueLoader* valueLoader,
	ValueLocation*& _location)
{
	// This is an internal child node for a non-final dimension -- just clone
	// the parent's location.
	ValueLocation* parentLocation = fParent->Location();
	if (parentLocation == NULL)
		return B_BAD_VALUE;

	parentLocation->AcquireReference();
	_location = parentLocation;

	return B_OK;
}
