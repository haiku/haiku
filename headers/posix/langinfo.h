#ifndef _LANGINFO_H_
#define	_LANGINFO_H_

#include <LocaleBuild.h>

#include <LocaleStrings.h>
#include <nl_types.h>

#define	CODESET		B_CODESET			/* codeset name */
#define	D_T_FMT		B_DATE_TIME_FORMAT	/* string for formatting date and time */
#define	D_FMT		B_DATE_FORMAT		/* date format string */
#define	T_FMT		B_TIME_FORMAT		/* time format string */
#define	T_FMT_AMPM	B_AM_PM_TIME_FORMAT	/* a.m. or p.m. time formatting string */
#define	AM_STR		B_AM_STRING			/* Ante Meridian affix */
#define	PM_STR		B_PM_STRING			/* Post Meridian affix */

/* week day names */
#define	DAY_1		B_DAY_1
#define	DAY_2		B_DAY_2
#define	DAY_3		B_DAY_3
#define	DAY_4		B_DAY_4
#define	DAY_5		B_DAY_5
#define	DAY_6		B_DAY_6
#define	DAY_7		B_DAY_7

/* abbreviated week day names */
#define	ABDAY_1		B_AB_DAY_1
#define	ABDAY_2		B_AB_DAY_2
#define	ABDAY_3		B_AB_DAY_3
#define	ABDAY_4		B_AB_DAY_4
#define	ABDAY_5		B_AB_DAY_5
#define	ABDAY_6		B_AB_DAY_6
#define	ABDAY_7		B_AB_DAY_7

/* month names */
#define	MON_1		B_MON_1
#define	MON_2		B_MON_2
#define	MON_3		B_MON_3
#define	MON_4		B_MON_4
#define	MON_5		B_MON_5
#define	MON_6		B_MON_6
#define	MON_7		B_MON_7
#define	MON_8		B_MON_8
#define	MON_9		B_MON_9
#define	MON_10		B_MON_10
#define	MON_11		B_MON_11
#define	MON_12		B_MON_12

/* abbreviated month names */
#define	ABMON_1		B_AB_MON_1
#define	ABMON_2		B_AB_MON_2
#define	ABMON_3		B_AB_MON_3
#define	ABMON_4		B_AB_MON_4
#define	ABMON_5		B_AB_MON_5
#define	ABMON_6		B_AB_MON_6
#define	ABMON_7		B_AB_MON_7
#define	ABMON_8		B_AB_MON_8
#define	ABMON_9		B_AB_MON_9
#define	ABMON_10	B_AB_MON_10
#define	ABMON_11	B_AB_MON_11
#define	ABMON_12	B_AB_MON_12

#define	ERA			B_ERA					/* era description segments */
#define	ERA_D_FMT	B_ERA_DATE_FORMAT		/* era date format string */
#define	ERA_D_T_FMT	B_ERA_DATE_TIME_FORMAT	/* era date and time format string */
#define	ERA_T_FMT	B_TIME_FORMAT			/* era time format string */
#define	ALT_DIGITS	B_ALT_DIGITS			/* alternative symbols for digits */

#define	RADIXCHAR	B_DECIMAL_POINT			/* radix char */
#define	THOUSEP		B_THOUSANDS_SEPARATOR	/* separator for thousands */

#define	YESEXPR		B_YES_EXPRESSION		/* affirmative response expression */
#define	NOEXPR		B_NO_EXPRESSION			/* negative response expression */
#define	YESSTR		B_YES_STRING			/* affirmative response for yes/no queries */
#define	NOSTR		B_NO_STRING				/* negative response for yes/no queries */

#define	CRNCYSTR	B_CURRENCY_SYMBOL		/* currency symbol */

//#define	D_MD_ORDER	57	/* month/day order (local extension) */

#ifdef __cplusplus
extern "C"
#endif
_IMPEXP_LOCALE char	*nl_langinfo(nl_item);

#endif	/* _LANGINFO_H_ */
