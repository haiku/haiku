/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H


#include <String.h>

#include <Referenceable.h>


enum syntax_highlight_type {
	SYNTAX_HIGHLIGHT_NONE,
	SYNTAX_HIGHLIGHT_KEYWORD
	// TODO:...
};


class SyntaxHighlightSource : public BReferenceable {
	virtual						~SyntaxHighlightSource();

	virtual	int32				CountLines() const = 0;
	virtual	void				GetLineAt(int32 index, BString& _line) const
									= 0;
};


class SyntaxHighlightInfo {
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

	virtual	status_t			ParseText(SyntaxHighlightSource* source,
									SyntaxHighlightInfo*& _info) = 0;
										// caller owns the returned info
};


#endif	// SYNTAX_HIGHLIGHTER_H
