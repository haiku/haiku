/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LANGINFO_H_
#define _LANGINFO_H_


#include <locale.h>
#include <nl_types.h>
#include <sys/cdefs.h>


enum {
	CODESET,	/* codeset name */
	D_T_FMT,	/* string for formatting date and time */
	D_FMT,		/* date format string */
	T_FMT,		/* time format string */
	T_FMT_AMPM,	/* a.m. or p.m. time formatting string */
	AM_STR,		/* Ante Meridian affix */
	PM_STR,		/* Post Meridian affix */

	/* week day names */
	DAY_1,
	DAY_2,
	DAY_3,
	DAY_4,
	DAY_5,
	DAY_6,
	DAY_7,

	/* abbreviated week day names */
	ABDAY_1,
	ABDAY_2,
	ABDAY_3,
	ABDAY_4,
	ABDAY_5,
	ABDAY_6,
	ABDAY_7,

	/* month names */
	MON_1,
	MON_2,
	MON_3,
	MON_4,
	MON_5,
	MON_6,
	MON_7,
	MON_8,
	MON_9,
	MON_10,
	MON_11,
	MON_12,

	/* abbreviated month names */
	ABMON_1,
	ABMON_2,
	ABMON_3,
	ABMON_4,
	ABMON_5,
	ABMON_6,
	ABMON_7,
	ABMON_8,
	ABMON_9,
	ABMON_10,
	ABMON_11,
	ABMON_12,

	ERA,			/* era description segments */
	ERA_D_FMT,		/* era date format string */
	ERA_D_T_FMT,	/* era date and time format string */
	ERA_T_FMT,		/* era time format string */
	ALT_DIGITS,		/* alternative symbols for digits */

	RADIXCHAR,		/* radix char */
	THOUSEP,		/* separator for thousands */

	YESEXPR,		/* affirmative response expression */
	NOEXPR,			/* negative response expression */

	CRNCYSTR,		/* currency symbol */

	_NL_LANGINFO_LAST
};

__BEGIN_DECLS

extern char*	nl_langinfo(nl_item item);

__END_DECLS


#endif /* _LANGINFO_H_ */
