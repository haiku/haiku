/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "X86AssemblyLanguage.h"


X86AssemblyLanguage::X86AssemblyLanguage()
{
}


X86AssemblyLanguage::~X86AssemblyLanguage()
{
}


const char*
X86AssemblyLanguage::Name() const
{
	return "x86 assembly";
}


SyntaxHighlighter*
X86AssemblyLanguage::GetSyntaxHighlighter() const
{
	// none available yet
	return NULL;
}
