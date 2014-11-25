/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CLanguageFamilySyntaxHighlightInfo.h"

#include "LineDataSource.h"


CLanguageFamilySyntaxHighlightInfo::CLanguageFamilySyntaxHighlightInfo(
	LineDataSource* source)
	:
	SyntaxHighlightInfo(),
	fHighlightSource(source)
{
	fHighlightSource->AcquireReference();
}


CLanguageFamilySyntaxHighlightInfo::~CLanguageFamilySyntaxHighlightInfo()
{
	fHighlightSource->ReleaseReference();
}


int32
CLanguageFamilySyntaxHighlightInfo::GetLineHighlightRanges(int32 line,
	int32* _columns, syntax_highlight_type* _types, int32 maxCount)
{
	// TODO: implement
	return 0;
}
