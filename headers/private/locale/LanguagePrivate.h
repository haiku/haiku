/*
 * Copyright 2010-2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _LANGUAGE_PRIVATE_H
#define _LANGUAGE_PRIVATE_H


#include <Language.h>


class BLanguage::Private {
public:
	Private(const BLanguage* language = NULL)
		:
		fLanguage(language)
	{
	}

	void
	SetTo(const BLanguage* language)
	{
		fLanguage = language;
	}

	icu::Locale*
	ICULocale()
	{
		return fLanguage->fICULocale;
	}

private:
	const BLanguage* fLanguage;
};


#endif	// _LANGUAGE_PRIVATE_H
