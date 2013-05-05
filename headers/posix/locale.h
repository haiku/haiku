/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_H_
#define _LOCALE_H_


#include <null.h>

struct lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char int_p_cs_precedes;
	char int_p_sep_by_space;
	char int_n_cs_precedes;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

#define	LC_ALL		0
#define	LC_COLLATE	1
#define	LC_CTYPE	2
#define	LC_MONETARY	3
#define	LC_NUMERIC	4
#define	LC_TIME		5
#define	LC_MESSAGES	6
/*
 * the values above must be kept in loopable order (i.e. strictly increasing
 * with no holes) and in sync with the value below
 */
#define	LC_LAST		LC_MESSAGES

#ifdef __cplusplus
extern "C" {
#endif

extern struct lconv *localeconv(void);
extern char *setlocale(int category, const char *locale);

#ifdef __cplusplus
}
#endif

#endif /* _LOCALE_H_ */
