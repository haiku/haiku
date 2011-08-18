/*
 * Copyright 2010-2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _FORMATTING_CONVENTIONS_PRIVATE_H
#define _FORMATTING_CONVENTIONS_PRIVATE_H


#include <FormattingConventions.h>


class BFormattingConventions::Private {
public:
	Private(const BFormattingConventions* conventions = NULL)
		:
		fFormattingConventions(conventions)
	{
	}

	void
	SetTo(const BFormattingConventions* conventions)
	{
		fFormattingConventions = conventions;
	}

	icu::Locale*
	ICULocale()
	{
		return fFormattingConventions->fICULocale;
	}

private:
	const BFormattingConventions* fFormattingConventions;
};


#endif	// _FORMATTING_CONVENTIONS_PRIVATE_H
