/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CppLanguage.h"


CppLanguage::CppLanguage()
{
}


CppLanguage::~CppLanguage()
{
}


const char*
CppLanguage::Name() const
{
	return "C++";
}


bool
CppLanguage::IsModifierValid(char modifier) const
{
	if (modifier == '*' || modifier == '&')
		return true;

	return false;
}
