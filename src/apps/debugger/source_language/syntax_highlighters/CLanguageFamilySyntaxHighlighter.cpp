/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CLanguageFamilySyntaxHighlighter.h"

#include <new>

#include "CLanguageFamilySyntaxHighlightInfo.h"


CLanguageFamilySyntaxHighlighter::CLanguageFamilySyntaxHighlighter()
	:
	SyntaxHighlighter()
{
}


CLanguageFamilySyntaxHighlighter::~CLanguageFamilySyntaxHighlighter()
{
}


status_t
CLanguageFamilySyntaxHighlighter::ParseText(LineDataSource* source,
	SyntaxHighlightInfo*& _info)
{
	_info = new(std::nothrow) CLanguageFamilySyntaxHighlightInfo(source);
	if (_info == NULL)
		return B_NO_MEMORY;

	return B_OK;
}
