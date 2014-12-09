/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ExpressionInfo.h"

#include "Type.h"
#include "Value.h"
#include "ValueNode.h"


// #pragma mark - ExpressionResult


ExpressionResult::ExpressionResult()
	:
	fResultKind(EXPRESSION_RESULT_KIND_UNKNOWN),
	fPrimitiveValue(NULL),
	fValueNodeValue(NULL),
	fTypeResult(NULL)
{
}


ExpressionResult::~ExpressionResult()
{
	if (fPrimitiveValue != NULL)
		fPrimitiveValue->ReleaseReference();

	if (fValueNodeValue != NULL)
		fValueNodeValue->ReleaseReference();

	if (fTypeResult != NULL)
		fTypeResult->ReleaseReference();
}


void
ExpressionResult::SetToPrimitive(Value* value)
{
	_Unset();

	fPrimitiveValue = value;
	if (fPrimitiveValue != NULL) {
		fPrimitiveValue->AcquireReference();
		fResultKind = EXPRESSION_RESULT_KIND_PRIMITIVE;
	}
}


void
ExpressionResult::SetToValueNode(ValueNodeChild* child)
{
	_Unset();

	fValueNodeValue = child;
	if (fValueNodeValue != NULL) {
		fValueNodeValue->AcquireReference();
		fResultKind = EXPRESSION_RESULT_KIND_VALUE_NODE;
	}

	// if the child has a node with a resolved value, store
	// it as a primitive, so the consumer of the expression
	// can use it as-is if desired.

	ValueNode* node = child->Node();
	if (node == NULL)
		return;

	fPrimitiveValue = node->GetValue();
	if (fPrimitiveValue != NULL)
		fPrimitiveValue->AcquireReference();
}


void
ExpressionResult::SetToType(Type* type)
{
	_Unset();

	fTypeResult = type;
	if (fTypeResult != NULL) {
		fTypeResult->AcquireReference();
		fResultKind = EXPRESSION_RESULT_KIND_TYPE;
	}
}


void
ExpressionResult::_Unset()
{
	if (fPrimitiveValue != NULL) {
		fPrimitiveValue->ReleaseReference();
		fPrimitiveValue = NULL;
	}

	if (fValueNodeValue != NULL) {
		fValueNodeValue->ReleaseReference();
		fValueNodeValue = NULL;
	}

	if (fTypeResult != NULL) {
		fTypeResult->ReleaseReference();
		fTypeResult = NULL;
	}

	fResultKind = EXPRESSION_RESULT_KIND_UNKNOWN;
}


// #pragma mark - ExpressionInfo


ExpressionInfo::ExpressionInfo()
	:
	fExpression()
{
}


ExpressionInfo::ExpressionInfo(const ExpressionInfo& other)
	:
	fExpression(other.fExpression)
{
}


ExpressionInfo::~ExpressionInfo()
{
}


ExpressionInfo::ExpressionInfo(const BString& expression)
	:
	fExpression(expression)
{
}


void
ExpressionInfo::SetTo(const BString& expression)
{
	fExpression = expression;
}


void
ExpressionInfo::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
ExpressionInfo::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


void
ExpressionInfo::NotifyExpressionEvaluated(status_t result,
	ExpressionResult* value)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ExpressionEvaluated(this, result, value);
	}
}


// #pragma mark - ExpressionInfo::Listener


ExpressionInfo::Listener::~Listener()
{
}
