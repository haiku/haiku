/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ExpressionInfo.h"

#include "Type.h"


ExpressionInfo::ExpressionInfo()
	:
	fExpression(),
	fResultType(NULL)
{
}


ExpressionInfo::ExpressionInfo(const ExpressionInfo& other)
	:
	fExpression(other.fExpression),
	fResultType(other.fResultType)
{
	if (fResultType != NULL)
		fResultType->AcquireReference();
}


ExpressionInfo::~ExpressionInfo()
{
	SetResultType(NULL);
}


ExpressionInfo::ExpressionInfo(const BString& expression, Type* resultType)
	:
	fExpression(expression),
	fResultType(resultType)
{
	if (resultType != NULL)
		resultType->AcquireReference();
}


void
ExpressionInfo::SetTo(const BString& expression, Type* resultType)
{
	SetExpression(expression);
	SetResultType(resultType);
}


void
ExpressionInfo::SetExpression(const BString& expression)
{
	fExpression = expression;
}


void
ExpressionInfo::SetResultType(Type* resultType)
{
	if (fResultType != NULL)
		fResultType->ReleaseReference();

	fResultType = resultType;
	if (fResultType != NULL)
		fResultType->AcquireReference();
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
ExpressionInfo::NotifyExpressionEvaluated(status_t result, Value* value)
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
