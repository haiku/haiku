/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LanguageRepository.h"

#include <algorithm>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "StringUtils.h"


LanguageRepository::LanguageRepository()
{
}


LanguageRepository::~LanguageRepository()
{
}


void
LanguageRepository::Clear()
{
	fLanguages.clear();
	HDINFO("did clear the languages");
}


bool
LanguageRepository::IsEmpty() const
{
	return fLanguages.empty();
}


int32
LanguageRepository::CountLanguages() const
{
	return fLanguages.size();
}


const LanguageRef
LanguageRepository::LanguageAtIndex(int32 index) const
{
	return fLanguages[index];
}


void
LanguageRepository::AddLanguage(const LanguageRef& value)
{
	int32 index = IndexOfLanguage(value->Code(), value->CountryCode(), value->ScriptCode());

	if (-1 == index) {
		std::vector<LanguageRef>::iterator itInsertionPt
			= std::lower_bound(fLanguages.begin(), fLanguages.end(), value, &IsLanguageBefore);
		fLanguages.insert(itInsertionPt, value);
		HDTRACE("did add the language [%s]", value->ID());
	} else {
		fLanguages[index] = value;
		HDTRACE("did replace the language [%s]", value->ID());
	}
}


int32
LanguageRepository::IndexOfLanguage(const char* code, const char* countryCode,
	const char* scriptCode) const
{
	size_t size = fLanguages.size();
	for (uint32 i = 0; i < size; i++) {
		const char* lCode = fLanguages[i]->Code();
		const char* lCountryCode = fLanguages[i]->CountryCode();
		const char* lScriptCode = fLanguages[i]->ScriptCode();

		if (0 == StringUtils::NullSafeCompare(code, lCode)
			&& 0 == StringUtils::NullSafeCompare(countryCode, lCountryCode)
			&& 0 == StringUtils::NullSafeCompare(scriptCode, lScriptCode)) {
			return i;
		}
	}

	return -1;
}
