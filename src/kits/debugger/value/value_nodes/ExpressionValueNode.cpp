/*
 * Copyright 2014, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "ExpressionValueNode.h"

#include <new>

#include "Type.h"


// #pragma mark - ExpressionValueNode


ExpressionValueNode::ExpressionValueNode(ExpressionValueNodeChild* nodeChild,
	Type* type)
	:
	ChildlessValueNode(nodeChild),
	fType(type)
{
	fType->AcquireReference();
}


ExpressionValueNode::~ExpressionValueNode()
{
	fType->ReleaseReference();
}


Type*
ExpressionValueNode::GetType() const
{
	return fType;
}


status_t
ExpressionValueNode::ResolvedLocationAndValue(ValueLoader* valueLoader,
	ValueLocation*& _location, Value*& _value)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark - ExpressionValueNodeChild


ExpressionValueNodeChild::ExpressionValueNodeChild(const BString& expression,
	Type* resultType)
	:
	fExpression(expression),
	fResultType(resultType)
{
	fResultType->AcquireReference();
}


ExpressionValueNodeChild::~ExpressionValueNodeChild()
{
	fResultType->ReleaseReference();
}


const BString&
ExpressionValueNodeChild::Name() const
{
	return fExpression;
}


Type*
ExpressionValueNodeChild::GetType() const
{
	return fResultType;
}


ValueNode*
ExpressionValueNodeChild::Parent() const
{
	return NULL;
}


status_t
ExpressionValueNodeChild::ResolveLocation(ValueLoader* valueLoader,
	ValueLocation*& _location)
{
	_location = NULL;
	return B_OK;
}
