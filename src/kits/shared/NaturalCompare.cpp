/*
 * Copyright 2009, Dana Burkart
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Rene Gollent (anevilyak@gmail.com)
 * Distributed under the terms of the MIT License.
 */


#include <NaturalCompare.h>

#include <ctype.h>
#include <string.h>
#include <strings.h>

#include <Collator.h>
#include <Locale.h>


namespace BPrivate {


// #pragma mark - Natural sorting


//! Compares two strings naturally, as opposed to lexicographically
int
NaturalCompare(const char* stringA, const char* stringB)
{
	static BCollator* collator = NULL;

	if (collator == NULL)
	{
		collator = new BCollator();
		BLocale::Default()->GetCollator(collator);
		collator->SetStrength(B_COLLATE_SECONDARY);
		collator->SetNumericSorting(true);
	}

	return collator->Compare(stringA, stringB);
}


} // namespace BPrivate
