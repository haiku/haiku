/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LANGUAGE_H
#define SOURCE_LANGUAGE_H


#include <Referenceable.h>


class SyntaxHighlighter;


class SourceLanguage : public BReferenceable {
public:
	virtual						~SourceLanguage();

	virtual	const char*			Name() const = 0;

	virtual	SyntaxHighlighter*	GetSyntaxHighlighter() const;
									// returns a reference,
									// may return NULL, if not available
};


#endif	// SOURCE_LANGUAGE_H
