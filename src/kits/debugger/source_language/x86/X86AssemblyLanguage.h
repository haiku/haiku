/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef X86_ASSEMBLY_LANGUAGE_H
#define X86_ASSEMBLY_LANGUAGE_H


#include "SourceLanguage.h"


class X86AssemblyLanguage : public SourceLanguage {
public:
								X86AssemblyLanguage();
	virtual						~X86AssemblyLanguage();

	virtual	const char*			Name() const;

	virtual	SyntaxHighlighter*	GetSyntaxHighlighter() const;
};


#endif	// X86_ASSEMBLY_LANGUAGE_H
