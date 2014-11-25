/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H
#define C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H


#include "SyntaxHighlighter.h"


class CLanguageFamilySyntaxHighlightInfo : public SyntaxHighlightInfo {
public:
								CLanguageFamilySyntaxHighlightInfo(
									LineDataSource* source);
	virtual						~CLanguageFamilySyntaxHighlightInfo();

	virtual	int32				GetLineHighlightRanges(int32 line,
									int32* _columns,
									syntax_highlight_type* _types,
									int32 maxCount);

private:
	LineDataSource*				fHighlightSource;
};


#endif	// C_LANGUAGE_FAMILY_SYNTAX_HIGHLIGHT_INFO_H
