/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "LocaleUtils.h"

#include <stdlib.h>

#include <Collator.h>
#include <Locale.h>
#include <LocaleRoster.h>


BCollator* LocaleUtils::sSharedCollator = NULL;


/*static*/ BCollator*
LocaleUtils::GetSharedCollator()
{
	if (sSharedCollator == NULL) {
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
