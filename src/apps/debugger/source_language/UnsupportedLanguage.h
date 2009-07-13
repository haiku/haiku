/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNSUPPORTED_LANGUAGE_H
#define UNSUPPORTED_LANGUAGE_H


#include "SourceLanguage.h"


class UnsupportedLanguage : public SourceLanguage {
public:
	virtual	const char*			Name() const;
};


#endif	// UNSUPPORTED_LANGUAGE_H
