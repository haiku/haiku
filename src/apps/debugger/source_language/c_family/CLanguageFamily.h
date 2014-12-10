/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef C_LANGUAGE_FAMILY_H
#define C_LANGUAGE_FAMILY_H


#include "SourceLanguage.h"


class CLanguageFamily : public SourceLanguage {
public:
								CLanguageFamily();
	virtual						~CLanguageFamily();

	virtual	SyntaxHighlighter*	GetSyntaxHighlighter() const;

	virtual	status_t			EvaluateExpression(const BString& expression,
									ValueNodeManager* manager,
									TeamTypeInformation* info,
									ExpressionResult*& _output,
									ValueNode*& _neededNode);
};


#endif	// C_LANGUAGE_FAMILY_H
