/*
 * Copyright 2019-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_UTILS_H
#define LOCALE_UTILS_H


#include <vector>

#include <String.h>

#include "Language.h"


class BCollator;


class LocaleUtils {

public:
	static	BCollator*		GetSharedCollator();

	static	BString			TimestampToDateTimeString(uint64 millis);
	static	BString			TimestampToDateString(uint64 millis);

	static	BString			CreateTranslatedIAmMinimumAgeSlug(int minimumAge);

	static	LanguageRef		DeriveDefaultLanguage(const std::vector<LanguageRef>& languages);
	static	std::vector<LanguageRef>
							WellKnownLanguages();

	static	void			SetForcedSystemDefaultLanguageID(const BString& id);
		// exposed for testing

private:

	static	LanguageRef		_FindBestMatchingLanguage(const std::vector<LanguageRef>& languages,
								const char* code, const char* countryCode,
								const char* scriptCode);

	static	LanguageRef		_DeriveSystemDefaultLanguage();

	static	int32			_IndexOfBestMatchingLanguage(const std::vector<LanguageRef>& languages,
								const char* code, const char* countryCode,
								const char* scriptCode);

	static	void			_GetCollator(BCollator* collator);

private:
	static	BCollator*		sSharedCollator;

	static	BString			sForcedSystemDefaultLanguageID;
};


#endif // LOCALE_UTILS_H
