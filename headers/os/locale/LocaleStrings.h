#ifndef _LOCALE_STRINGS_H_
#define _LOCALE_STRINGS_H_


enum country_strings {
	B_COUNTRY_STRINGS_BASE	= 0,

	B_DATE_TIME_FORMAT		= B_COUNTRY_STRINGS_BASE,
	B_TIME_AM_PM_FORMAT,

	B_SHORT_DATE_TIME_FORMAT,
	B_SHORT_TIME_AM_PM_FORMAT,

	B_AM_STRING,
	B_PM_STRING,

	B_DATE_SEPARATOR,
	B_TIME_SEPARATOR,

	B_NUM_COUNTRY_STRINGS,
};

enum language_strings {
	B_LANGUAGE_STRINGS_BASE	= 100,

	B_YESTERDAY_STRING = B_LANGUAGE_STRINGS_BASE,
	B_TODAY_STRING,
	B_TOMORROW_STRING,
	B_FUTURE_STRING,

	B_DAY_1,		// name of the first day of the week, e.g. Sunday
	B_DAY_2,		// ...
	B_DAY_3,		//
	B_DAY_4,
	B_DAY_5,
	B_DAY_6,
	B_DAY_7,

	B_AB_DAY_1,		// abbreviated weekday name, e.g. Sun
	B_AB_DAY_2,		// ...
	B_AB_DAY_3,
	B_AB_DAY_4,
	B_AB_DAY_5,
	B_AB_DAY_6,
	B_AB_DAY_7,

	B_MON_1,		// name of the first month of the year, e.g. January
	B_MON_2,		// ...
	B_MON_3,
	B_MON_4,
	B_MON_5,
	B_MON_6,
	B_MON_7,
	B_MON_8,
	B_MON_9,
	B_MON_10,
	B_MON_11,
	B_MON_12,

	B_AB_MON_1,		// abbreviated month name, e.g. Jan
	B_AB_MON_2,		// ...
	B_AB_MON_3,
	B_AB_MON_4,
	B_AB_MON_5,
	B_AB_MON_6,
	B_AB_MON_7,
	B_AB_MON_8,
	B_AB_MON_9,
	B_AB_MON_10,
	B_AB_MON_11,
	B_AB_MON_12,

	B_YES_EXPRESSION,
	B_NO_EXPRESSION,
	B_YES_STRING,
	B_NO_STRING,

	B_NUM_LANGUAGE_STRINGS = B_AB_MON_12 - B_LANGUAGE_STRINGS_BASE,
};

// specials for POSIX compatibility
enum other_locale_strings {
	B_OTHER_STRINGS_BASE	= 200,

	B_CODESET	= B_OTHER_STRINGS_BASE,
	B_ERA,
	B_ERA_DATE_FORMAT,
	B_ERA_DATE_TIME_FORMAT,
	B_ERA_TIME_FORMAT,
	B_ALT_DIGITS
};

#endif	/* _LOCALE_STRINGS_H_ */
