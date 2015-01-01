/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H


#include <String.h>

#include <Referenceable.h>


class LineDataSource;
class TeamTypeInformation;


enum syntax_highlight_type {
	SYNTAX_HIGHLIGHT_NONE = 0,
	SYNTAX_HIGHLIGHT_KEYWORD,
	SYNTAX_HIGHLIGHT_PREPROCESSOR_KEYWORD,
	SYNTAX_HIGHLIGHT_IDENTIFIER,
	SYNTAX_HIGHLIGHT_OPERATOR,
	SYNTAX_HIGHLIGHT_TYPE,
	SYNTAX_HIGHLIGHT_NUMERIC_LITERAL,
	SYNTAX_HIGHLIGHT_STRING_LITERAL,
	SYNTAX_HIGHLIGHT_COMMENT
};


class SyntaxHighlightInfo {
public:
	virtual						~SyntaxHighlightInfo();

	virtual	int32				GetLineHighlightRanges(int32 line,
									int32* _columns,
									syntax_highlight_type* _types,
									int32 maxCount) = 0;
										// Returns number of filled in
										// (column, type) pairs.
										// Default is (0, SYNTAX_HIGHLIGHT_NONE)
										// and can be omitted.
};


class SyntaxHighlighter : public BReferenceable {
public:
	virtual						~SyntaxHighlighter();

	virtual	status_t			ParseText(LineDataSource* source,
									TeamTypeInformation* typeInfo,
									SyntaxHighlightInfo*& _info) = 0;
										// caller owns the returned info
};


#endif	// SYNTAX_HIGHLIGHTER_H
