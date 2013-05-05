/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CompoundValueNode.h"

#include <new>

#include "Architecture.h"
#include "IntegerValue.h"
#include "Tracing.h"
#include "Type.h"
#include "ValueLoader.h"
#include "ValueLocation.h"
#include "ValueNodeContainer.h"


// #pragma mark - Child


class CompoundValueNode::Child : public ValueNodeChild {
public:
	Child(CompoundValueNode* parent, const BString& name)
		:
		fParent(parent),
		fName(name)
	{
	}

	virtual const BString& Name() const
	{
		return fName;
	}

	virtual ValueNode* Parent() const
	{
		return fParent;
	}

protected:
	CompoundValueNode*	fParent;
	BString				fName;
};


// #pragma mark - BaseTypeChild


class CompoundValueNode::BaseTypeChild : public Child {
public:
	BaseTypeChild(CompoundValueNode* parent, BaseType* baseType)
		:
		Child(parent, baseType->GetType()->Name()),
		fBaseType(baseType)
	{
		fBaseType->AcquireReference();
	}

	virtual ~BaseTypeChild()
	{
		fBaseType->ReleaseReference();
	}

	virtual Type* GetType() const
	{
		return fBaseType->GetType();
	}

	virtual status_t ResolveLocation(ValueLoader* valueLoader,
		ValueLocation*& _location)
	{
		// The parent's location refers to the location of the complete
		// object. We want to extract the location of a member.
		ValueLocation* parentLocation = fParent->Location();
		if (parentLocation == NULL)
			return B_BAD_VALUE;

		ValueLocation* location;
		status_t error = fParent->fType->ResolveBaseTypeLocation(fBaseType,
			*parentLocation, location);
		if (error != B_OK) {
			TRACE_LOCALS("CompoundValueNode::BaseTypeChild::ResolveLocation(): "
				"ResolveBaseTypeLocation() failed: %s\n", strerror(error));
			return error;
		}

		_location = location;
		return B_OK;
	}

private:
	BaseType*	fBaseType;
};


// #pragma mark - MemberChild


class CompoundValueNode::MemberChild : public Child {
public:
	MemberChild(CompoundValueNode* parent, DataMember* member)
		:
		Child(parent, member->Name()),
		fMember(member)
	{
		fMember->AcquireReference();
	}

	virtual ~MemberChild()
	{
		fMember->ReleaseReference();
	}

	virtual Type* GetType() const
	{
		return fMember->GetType();
	}

	virtual status_t ResolveLocation(ValueLoader* valueLoader,
		ValueLocation*& _location)
	{
		// The parent's location refers to the location of the complete
		// object. We want to extract the location of a member.
		ValueLocation* parentLocation = fParent->Location();
		if (parentLocation == NULL)
			return B_BAD_VALUE;

		ValueLocation* location;
		status_t error = fParent->fType->ResolveDataMemberLocation(fMember,
			*parentLocation, location);
		if (error != B_OK) {
			TRACE_LOCALS("CompoundValueNode::MemberChild::ResolveLocation(): "
				"ResolveDataMemberLocation() failed: %s\n", strerror(error));
			return error;
		}

		_location = location;
		return B_OK;
	}

private:
	DataMember*	fMember;
};


// #pragma mark - CompoundValueNode


CompoundValueNode::CompoundValueNode(ValueNodeChild* nodeChild,
	CompoundType* type)
	:
	ValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


CompoundValueNode::~CompoundValueNode()
{
	fType->ReleaseReference();

	for (int32 i = 0; Child* child = fChildren.ItemAt(i); i++)
		child->ReleaseReference();
}


Type*
CompoundValueNode::GetType() const
{
	return fType;
}


status_t
CompoundValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
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
CompoundValueNode::CreateChildren()
{
	if (!fChildren.IsEmpty())
		return B_OK;

	// base types
	for (int32 i = 0; BaseType* baseType = fType->BaseTypeAt(i); i++) {
		TRACE_LOCALS("  base %" B_PRId32 "\n", i);

		BaseTypeChild* child = new(std::nothrow) BaseTypeChild(this, baseType);
		if (child == NULL || !fChildren.AddItem(child)) {
			delete child;
			return B_NO_MEMORY;
		}

		child->SetContainer(fContainer);
	}

	// members
	for (int32 i = 0; DataMember* member = fType->DataMemberAt(i); i++) {
		TRACE_LOCALS("  member %" B_PRId32 ": \"%s\"\n", i, member->Name());

		MemberChild* child = new(std::nothrow) MemberChild(this, member);
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
CompoundValueNode::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
CompoundValueNode::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}
