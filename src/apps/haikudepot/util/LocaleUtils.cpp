/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LocaleUtils.h"

#include <stdlib.h>

#include <Catalog.h>
#include <Collator.h>
#include <DateTimeFormat.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <StringFormat.h>

#include "Logger.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocaleUtils"


BCollator* LocaleUtils::sSharedCollator = NULL;


/*static*/ BCollator*
LocaleUtils::GetSharedCollator()
{
	if (sSharedCollator == NULL) {
		sSharedCollator = new BCollator();
		GetCollator(sSharedCollator);
	}

	return sSharedCollator;
}


/*static*/ void
LocaleUtils::GetCollator(BCollator* collator)
{
	const BLocale* locale = BLocaleRoster::Default()->GetDefaultLocale();

	if (locale->GetCollator(collator) != B_OK)
		HDFATAL("unable to get the locale's collator");
}


/*static*/ BString
LocaleUtils::TimestampToDateTimeString(uint64 millis)
{
	if (millis == 0)
		return "?";

	BDateTimeFormat format;
	BString buffer;
	if (format.Format(buffer, millis / 1000, B_SHORT_DATE_FORMAT,
		B_SHORT_TIME_FORMAT) != B_OK)
		return "!";

	return buffer;
}


/*!	This is used in situations where the user is required to confirm that they
	are as old or older than some minimal age.  This is associated with agreeing
	to the user usage conditions.
*/

/*static*/ BString
LocaleUtils::CreateTranslatedIAmMinimumAgeSlug(int minimumAge)
{
	BString slug;
	static BStringFormat format(B_TRANSLATE("{0, plural,"
 		"one{I am at least one year old}"
		"other{I am # years of age or older}}"));
	format.Format(slug, minimumAge);
	return slug;
}
