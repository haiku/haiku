/*
 * Copyright 2025, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALEINFO_H
#define _LOCALEINFO_H


#include <stdlib.h>
#include <locale.h>
#include <wchar.h>


enum {
	LC_CTYPE__NL_CTYPE_OUTDIGITS_MB_LEN = 0,
	LC_CTYPE__NL_CTYPE_OUTDIGIT0_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT1_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT2_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT3_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT4_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT5_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT6_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT7_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT8_MB,
	LC_CTYPE__NL_CTYPE_OUTDIGIT9_MB,
	LC_CTYPE__NL_CTYPE_MB_CUR_MAX,

#define LC_CTYPE__NL_CTYPE_INDIGITS_MB_LEN LC_CTYPE__NL_CTYPE_OUTDIGITS_MB_LEN
#define LC_CTYPE__NL_CTYPE_INDIGITS0_MB LC_CTYPE__NL_CTYPE_OUTDIGIT0_MB

#define LC_CTYPE__NL_CTYPE_OUTDIGITS_WC_LEN LC_CTYPE__NL_CTYPE_OUTDIGITS_MB_LEN
#define LC_CTYPE__NL_CTYPE_OUTDIGIT0_WC LC_CTYPE__NL_CTYPE_OUTDIGIT0_MB
#define LC_CTYPE__NL_CTYPE_INDIGITS_WC_LEN LC_CTYPE__NL_CTYPE_INDIGITS_MB_LEN
#define LC_CTYPE__NL_CTYPE_INDIGITS0_WC LC_CTYPE__NL_CTYPE_INDIGITS0_MB

	LC_MONETARY_MON_DECIMAL_POINT,
	LC_MONETARY_MON_THOUSANDS_SEP,
	LC_MONETARY_MON_GROUPING,

#define LC_MONETARY__NL_MONETARY_DECIMAL_POINT_WC LC_MONETARY_MON_DECIMAL_POINT
#define LC_MONETARY__NL_MONETARY_THOUSANDS_SEP_WC LC_MONETARY_MON_THOUSANDS_SEP

	LC_NUMERIC_DECIMAL_POINT,
	LC_NUMERIC_THOUSANDS_SEP,
	LC_NUMERIC_GROUPING,

#define LC_NUMERIC__NL_NUMERIC_DECIMAL_POINT_WC LC_NUMERIC_DECIMAL_POINT
#define LC_NUMERIC__NL_NUMERIC_THOUSANDS_SEP_WC LC_NUMERIC_THOUSANDS_SEP
};

static inline const char*
_nl_current(int value)
{
	struct lconv* lconv = localeconv();
	switch (value) {
		// TODO: Not correct for non-ASCII/UTF-8 multibyte locales!
		// (perhaps via alloca+wcrtomb? or do we need new localeinfo?)
#define DIGIT(D) case LC_CTYPE__NL_CTYPE_OUTDIGIT##D##_MB: return #D
		DIGIT(0);
		DIGIT(1);
		DIGIT(2);
		DIGIT(3);
		DIGIT(4);
		DIGIT(5);
		DIGIT(6);
		DIGIT(7);
		DIGIT(8);
		DIGIT(9);
#undef DIGIT

		case LC_MONETARY_MON_DECIMAL_POINT:
			return lconv->mon_decimal_point;
		case LC_MONETARY_MON_THOUSANDS_SEP:
			return lconv->mon_thousands_sep;
		case LC_MONETARY_MON_GROUPING:
			return lconv->mon_grouping;

		case LC_NUMERIC_DECIMAL_POINT:
			return lconv->decimal_point;
		case LC_NUMERIC_THOUSANDS_SEP:
			return lconv->thousands_sep;
		case LC_NUMERIC_GROUPING:
			return lconv->grouping;
	}
	return NULL;
}

static inline const wchar_t
_nl_current_word(int value)
{
	struct lconv* lconv = NULL;
	mbstate_t temp;
	const char* str = NULL;
	wchar_t out = 0;

	switch (value) {
		case LC_CTYPE__NL_CTYPE_OUTDIGITS_WC_LEN:
			return 1;

		/* We always use UTF-32 in wchar_t. */
#define DIGIT(D) case LC_CTYPE__NL_CTYPE_OUTDIGIT##D##_MB: return 0x0030 + D
		DIGIT(0);
		DIGIT(1);
		DIGIT(2);
		DIGIT(3);
		DIGIT(4);
		DIGIT(5);
		DIGIT(6);
		DIGIT(7);
		DIGIT(8);
		DIGIT(9);
#undef DIGIT

		case LC_CTYPE__NL_CTYPE_MB_CUR_MAX:
			return __ctype_get_mb_cur_max();

		case LC_MONETARY_MON_DECIMAL_POINT:
			lconv = localeconv();
			str = lconv->mon_decimal_point;
			break;
		case LC_MONETARY_MON_THOUSANDS_SEP:
			lconv = localeconv();
			str = lconv->mon_thousands_sep;
			break;

		case LC_NUMERIC_DECIMAL_POINT:
			lconv = localeconv();
			str = lconv->decimal_point;
			break;
		case LC_NUMERIC_THOUSANDS_SEP:
			lconv = localeconv();
			str = lconv->thousands_sep;
			break;
	}
	if (str == NULL)
		return out;

	mbrtowc(&out, str, 1, &temp);
	return out;
}


#define _NL_CURRENT(category, item) \
	_nl_current(category##_##item)
#define _NL_CURRENT_WORD(category, item) \
	_nl_current_word(category##_##item)


#endif	/* _LOCALEINFO_H */
