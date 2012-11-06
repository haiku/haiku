/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPP_LANGUAGE_H
#define CPP_LANGUAGE_H


#include "CLanguageFamily.h"


class CppLanguage : public CLanguageFamily {
public:
								CppLanguage();
	virtual						~CppLanguage();

	virtual	const char*			Name() const;

	virtual status_t ParseTypeExpression(const BString &expression,
									TeamTypeInformation* lookup,
									Type*& _resultType) const;
};


#endif	// CPP_LANGUAGE_H
