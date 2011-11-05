/*
 * Copyright 2010-2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_ZONE_PRIVATE_H
#define _TIME_ZONE_PRIVATE_H


#include <TimeZone.h>


class BTimeZone::Private {
public:
	Private(const BTimeZone* timeZone = NULL)
		:
		fTimeZone(timeZone)
	{
	}

	void
	SetTo(const BTimeZone* timeZone)
	{
		fTimeZone = timeZone;
	}

	icu::TimeZone*
	ICUTimeZone()
	{
		return fTimeZone->fICUTimeZone;
	}

private:
	const BTimeZone* fTimeZone;
};


#endif	// _TIME_ZONE_PRIVATE_H
