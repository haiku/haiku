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
	if (fResultType != NULL)
		fResultType->ReleaseReference();
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
	fExpression = expression;
	if (fResultType != NULL)
		fResultType->ReleaseReference();

	fResultType = resultType;
	if (fResultType != NULL)
		fResultType->AcquireReference();
}
