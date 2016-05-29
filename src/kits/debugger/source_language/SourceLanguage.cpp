/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SourceLanguage.h"


SourceLanguage::~SourceLanguage()
{
}


SyntaxHighlighter*
SourceLanguage::GetSyntaxHighlighter() const
{
	return NULL;
}


status_t
SourceLanguage::EvaluateExpression(const BString& expression,
	ValueNodeManager* manager, TeamTypeInformation* info,
	ExpressionResult*& _resultValue, ValueNode*& _neededNode)
{
	return B_NOT_SUPPORTED;
}
