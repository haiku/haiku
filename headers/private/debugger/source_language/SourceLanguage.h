/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LANGUAGE_H
#define SOURCE_LANGUAGE_H


#include <Referenceable.h>


class BString;
class ExpressionResult;
class SyntaxHighlighter;
class TeamTypeInformation;
class Type;
class ValueNode;
class ValueNodeManager;


class SourceLanguage : public BReferenceable {
public:
	virtual						~SourceLanguage();

	virtual	const char*			Name() const = 0;

	virtual	SyntaxHighlighter*	GetSyntaxHighlighter() const;
									// returns a reference,
									// may return NULL, if not available

	virtual	status_t			EvaluateExpression(const BString& expression,
									ValueNodeManager* manager,
									TeamTypeInformation* info,
									ExpressionResult*& _output,
									ValueNode*& _neededNode);
};


#endif	// SOURCE_LANGUAGE_H
