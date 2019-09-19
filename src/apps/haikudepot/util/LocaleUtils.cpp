/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LocaleUtils.h"

#include <stdlib.h>
#include <unicode/datefmt.h>
#include <unicode/dtptngen.h>
#include <unicode/smpdtfmt.h>

#include <Collator.h>
#include <ICUWrapper.h>
#include <Locale.h>
#include <LocaleRoster.h>


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

	if (B_OK != locale->GetCollator(collator)) {
		debugger("unable to get the locale's collator");
		exit(EXIT_FAILURE);
	}
}


/*! There was some difficulty in getting BDateTime and friends to
    work for the purposes of this application.  Data comes in as millis since
    the epoc relative to GMT0.  These need to be displayed in the local time
    zone, but the timezone aspect never seems to be quite right with BDateTime!
    For now, to avoid this work over-spilling into a debug of the date-time
    classes in Haiku, I am adding this method that uses ICU directly in order
    to get something basic working for now.  Later this should be migrated to
    use the BDateTime etc... classes from Haiku once these problems have been
    ironed out.
*/

/*static*/ BString
LocaleUtils::TimestampToDateTimeString(uint64 millis)
{
	if (millis == 0)
		return "?";

	UnicodeString pattern("yyyy-MM-dd HH:mm:ss");
		// later use variants of DateFormat::createInstance()
	UErrorCode success = U_ZERO_ERROR;
	SimpleDateFormat sdf(pattern, success);

	if (U_FAILURE(success))
		return "!";

	UnicodeString icuResult;
	sdf.format((UDate) millis, icuResult);
	BString result;
	BStringByteSink converter(&result);
	icuResult.toUTF8(converter);
	return result;
}
