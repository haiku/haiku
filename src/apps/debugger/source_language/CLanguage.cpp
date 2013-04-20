/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CLanguage.h"


CLanguage::CLanguage()
{
}


CLanguage::~CLanguage()
{
}


const char*
CLanguage::Name() const
{
	return "C";
}


bool
CLanguage::IsModifierValid(char modifier) const
{
	if (modifier == '*')
		return true;

	return false;
}
