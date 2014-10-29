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
class ValueNode;
class ValueNodeManager;


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
									type_code type, ValueNodeManager* manager,
									Value*& _output, ValueNode*& _neededNode);
};


#endif	// SOURCE_LANGUAGE_H
