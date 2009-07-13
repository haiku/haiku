/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef C_LANGUAGE_H
#define C_LANGUAGE_H


#include "CLanguageFamily.h"


class CLanguage : public CLanguageFamily {
public:
								CLanguage();
	virtual						~CLanguage();

	virtual	const char*			Name() const;
};


#endif	// C_LANGUAGE_H
