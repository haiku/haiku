/*
 * Copyright 2010, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _COUNTRY_PRIVATE_H
#define _COUNTRY_PRIVATE_H


#include <Country.h>


class BCountry::Private {
public:
	Private(const BCountry* country = NULL)
		:
		fCountry(country)
	{
	}

	void
	SetTo(const BCountry* country)
	{
		fCountry = country;
	}

	icu_44::Locale*
	ICULocale()
	{
		return fCountry->fICULocale;
	}

private:
	const BCountry* fCountry;
};


#endif	// _COUNTRY_PRIVATE_H
