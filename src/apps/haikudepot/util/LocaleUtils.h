/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_UTILS_H
#define LOCALE_UTILS_H


#include <String.h>


class BCollator;


class LocaleUtils {

public:
	static	BCollator*		GetSharedCollator();
	static	BString			TimestampToDateTimeString(uint64 millis);

	static	BString			CreateTranslatedIAmMinimumAgeSlug(int minimumAge);

private:
	static	void			GetCollator(BCollator* collator);

private:
	static	BCollator*		sSharedCollator;
};


#endif // LOCALE_UTILS_H
