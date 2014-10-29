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
SourceLanguage::ParseTypeExpression(const BString& expression,
	TeamTypeInformation* info, Type*& _resultType) const
{
	return B_NOT_SUPPORTED;
}


status_t
SourceLanguage::EvaluateExpression(const BString& expression,
	type_code type, ValueNodeManager* manager, Value*& _resultValue,
	ValueNode*& _neededNode)
{
	return B_NOT_SUPPORTED;
}
