/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Akshay Agarwal <agarwal.akshay.akshay8@gmail.com>
 */
#ifndef _B_RELATIVE_DATE_TIME_FORMAT_H_
#define _B_RELATIVE_DATE_TIME_FORMAT_H_


#include <Format.h>
#include <SupportDefs.h>


class BString;

#ifndef U_ICU_NAMESPACE
  #define U_ICU_NAMESPACE icu
#endif
namespace U_ICU_NAMESPACE {
	class GregorianCalendar;
	class RelativeDateTimeFormatter;
}


class BRelativeDateTimeFormat: public BFormat {
	typedef BFormat				Inherited;
public:
								BRelativeDateTimeFormat();
								BRelativeDateTimeFormat(const BLanguage& language,
									const BFormattingConventions& conventions);
								BRelativeDateTimeFormat(const BRelativeDateTimeFormat& other);
			virtual				~BRelativeDateTimeFormat();

			status_t			Format(BString& string, const time_t timeValue) const;

private:
			U_ICU_NAMESPACE::RelativeDateTimeFormatter*	fFormatter;
			U_ICU_NAMESPACE::GregorianCalendar*	fCalendar;
};


#endif	// _B_RELATIVE_DATE_TIME_FORMAT_H_
