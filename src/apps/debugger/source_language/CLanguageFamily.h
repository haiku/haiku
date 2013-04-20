/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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

	virtual status_t 			ParseTypeExpression(const BString& expression,
									TeamTypeInformation* lookup,
									Type*& _resultType) const;

protected:
	virtual	bool				IsModifierValid(char modifier) const = 0;
};


#endif	// C_LANGUAGE_FAMILY_H
