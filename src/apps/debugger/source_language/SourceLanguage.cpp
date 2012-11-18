/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
SourceLanguage::ParseTypeExpression(const BString &expression,
	TeamTypeInformation* info, Type*& _resultType) const
{
	return B_NOT_SUPPORTED;
}
