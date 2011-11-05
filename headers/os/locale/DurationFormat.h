/*
 * Copyright 2010-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_DURATION_FORMAT_H_
#define _B_DURATION_FORMAT_H_


#include <Format.h>
#include <String.h>
#include <TimeUnitFormat.h>


class BTimeZone;

namespace icu {
	class GregorianCalendar;
}


class BDurationFormat : public BFormat {
	typedef	BFormat				Inherited;

public:
								BDurationFormat(
									const BString& separator = ", ");
								BDurationFormat(const BDurationFormat& other);
	virtual						~BDurationFormat();

			BDurationFormat&	operator=(const BDurationFormat& other);

			void				SetSeparator(const BString& separator);

	virtual	status_t			SetLocale(const BLocale* locale);
			status_t			SetTimeZone(const BTimeZone* timeZone);

			status_t			Format(bigtime_t startValue,
									bigtime_t stopValue, BString* buffer,
									time_unit_style style = B_TIME_UNIT_FULL
									) const;

private:
			BString				fSeparator;
			BTimeUnitFormat		fTimeUnitFormat;
			icu::GregorianCalendar*	fCalendar;
};


#endif
