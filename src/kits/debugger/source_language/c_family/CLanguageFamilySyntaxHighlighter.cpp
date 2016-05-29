/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CLanguageFamilySyntaxHighlighter.h"

#include <new>

#include <AutoDeleter.h>

#include "CLanguageFamilySyntaxHighlightInfo.h"
#include "CLanguageTokenizer.h"


using CLanguage::Tokenizer;


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
	TeamTypeInformation* typeInfo, SyntaxHighlightInfo*& _info)
{
	Tokenizer* tokenizer = new(std::nothrow) Tokenizer();
	if (tokenizer == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<Tokenizer> deleter(tokenizer);

	_info = new(std::nothrow) CLanguageFamilySyntaxHighlightInfo(source,
		tokenizer, typeInfo);
	if (_info == NULL)
		return B_NO_MEMORY;

	deleter.Detach();
	return B_OK;
}
