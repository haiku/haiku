/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHTER_H
#define C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHTER_H


#include "SyntaxHighlighter.h"


class CLanguageFamilySyntaxHighlighter : public SyntaxHighlighter {
public:
								CLanguageFamilySyntaxHighlighter();
	virtual						~CLanguageFamilySyntaxHighlighter();

	virtual	status_t			ParseText(LineDataSource* source,
									TeamTypeInformation* typeInfo,
									SyntaxHighlightInfo*& _info);
										// caller owns the returned info
};


#endif	// C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHTER_H
