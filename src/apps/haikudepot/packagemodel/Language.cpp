/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Language.h"


#include "Logger.h"
#include "StringUtils.h"


/*! @param serverName is the name of the language on the server.
 */

Language::Language(const char* language, const BString& serverName, bool isPopular)
	:
	BLanguage(language),
	fServerName(serverName),
	fIsPopular(isPopular)
{
}


Language::Language(const Language& other)
	:
	BLanguage(other.ID()),
	fServerName(other.fServerName),
	fIsPopular(other.fIsPopular)
{
}


bool
Language::operator==(const Language& other) const
{
	return Compare(other) == 0;
}


bool
Language::operator!=(const Language& other) const
{
	return !(*this == other);
}


bool
Language::operator<(const Language& other) const
{
	return Compare(other) < 0;
}


status_t
Language::GetName(BString& name, const BLanguage* displayLanguage) const
{
	status_t result = BLanguage::GetName(name, displayLanguage);

	if (result == B_OK && (name.IsEmpty() || name == Code()))
		name.SetTo(fServerName);

	return result;
}


int
Language::Compare(const Language& other) const
{
	int result = StringUtils::NullSafeCompare(Code(), other.Code());
	if (0 == result)
		result = StringUtils::NullSafeCompare(CountryCode(), other.CountryCode());
	if (0 == result)
		result = StringUtils::NullSafeCompare(ScriptCode(), other.ScriptCode());
	return result;
}


bool
IsLanguageRefLess(const LanguageRef& l1, const LanguageRef& l2)
{
	if (!l1.IsSet() || !l2.IsSet())
		debugger("illegal state in language less");
	return *(l1.Get()) < *(l2.Get());
}
