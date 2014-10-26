/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LANGUAGE_H
#define SOURCE_LANGUAGE_H


#include <Referenceable.h>


class BString;
class SyntaxHighlighter;
class TeamTypeInformation;
class Type;
class Value;


class SourceLanguage : public BReferenceable {
public:
	virtual						~SourceLanguage();

	virtual	const char*			Name() const = 0;

	virtual	SyntaxHighlighter*	GetSyntaxHighlighter() const;
									// returns a reference,
									// may return NULL, if not available

	virtual status_t			ParseTypeExpression(const BString& expression,
									TeamTypeInformation* info,
									Type*& _resultType) const;

	virtual	status_t			EvaluateExpression(const BString& expression,
									Value*& _output);
};


#endif	// SOURCE_LANGUAGE_H
