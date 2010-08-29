/*
 * Copyright 2010, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_ZONE_ACCESSOR_H
#define _TIME_ZONE_ACCESSOR_H


#include <TimeZone.h>


struct BTimeZone::Accessor {
								Accessor(const BTimeZone* timeZone = NULL)
									: fTimeZone(timeZone)
								{}

			void				SetTo(const BTimeZone* timeZone)
								{
									fTimeZone = timeZone;
								}

			icu_44::TimeZone*	IcuTimeZone()
								{
									return fTimeZone->fIcuTimeZone;
								}

			const BTimeZone*	fTimeZone;
};


#endif	// _TIME_ZONE_ACCESSOR_H
