/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 */


#include <locale.h>
#include <limits.h>


/*
 * the values below initialize the struct to the "C" locale
 * which is the default (and only required) locale
 */
 
struct lconv _Locale = {
	".",		// decimal point
	
	"",			// thousands separator
	"",			// grouping
	"",			// international currency symbol
	"",			// local currency symbol
	"",			// monetary decimal point
	"",			// monetary thousands separator
	"",			// monetary grouping
	"",			// positive sign
	"",			// negative sign
	
	CHAR_MAX,	// int_frac_digits
	CHAR_MAX,	// frac_digits
	CHAR_MAX,	// p_cs_precedes
	CHAR_MAX,	// p_sep_by_space
	CHAR_MAX,	// n_cs_precedes
	CHAR_MAX,	// n_sep_by_space
	CHAR_MAX,	// p_sign_posn
	CHAR_MAX	// n_sign_posn
};



struct lconv*
localeconv(void)
{
	// return pointer to the current locale
	return &_Locale;
}
