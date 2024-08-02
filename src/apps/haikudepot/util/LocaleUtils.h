/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_UTILS_H
#define LOCALE_UTILS_H


#include <String.h>


#include "Language.h"
#include "LanguageRepository.h"


class BCollator;


class LocaleUtils {

public:
	static	BCollator*		GetSharedCollator();

	static	BString			TimestampToDateTimeString(uint64 millis);
	static	BString			TimestampToDateString(uint64 millis);

	static	BString			CreateTranslatedIAmMinimumAgeSlug(int minimumAge);

	static	LanguageRef		DeriveDefaultLanguage(LanguageRepository* repository);

	static	void			SetForcedSystemDefaultLanguageID(const BString& id);
		// exposed for testing

private:

	static	LanguageRef		_FindBestMatchingLanguage(LanguageRepository* repository,
								const char* code, const char* countryCode,
								const char* scriptCode);

	static	LanguageRef		_DeriveSystemDefaultLanguage();

	static	int32			_IndexOfBestMatchingLanguage(LanguageRepository* repository,
								const char* code, const char* countryCode,
								const char* scriptCode);

	static	void			_GetCollator(BCollator* collator);

private:
	static	BCollator*		sSharedCollator;

	static	BString			sForcedSystemDefaultLanguageID;
};


#endif // LOCALE_UTILS_H
