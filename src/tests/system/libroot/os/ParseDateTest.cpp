/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>
#include <parsedate.h>

#include <stdio.h>


#define ABSOLUTE		0
#define UNKNOWN			0
#define DAY_RELATIVE	PARSEDATE_RELATIVE_TIME
#define MINUTE_RELATIVE	(PARSEDATE_RELATIVE_TIME \
	| PARSEDATE_MINUTE_RELATIVE_TIME)
#define INVALID			PARSEDATE_INVALID_DATE


char*
parsedate_flags_to_string(time_t result, int flags)
{
	if (result == -1) {
		if ((flags & PARSEDATE_INVALID_DATE) != 0)
			return "invalid";
		return "unknown";
	}

	if ((flags & PARSEDATE_RELATIVE_TIME) != 0) {
		if ((flags & PARSEDATE_MINUTE_RELATIVE_TIME) != 0)
			return "minute relative";

		return "day relative";
	}

	return "absolute";
}


int
main(int argc, char** argv)
{
	time_t now = 1249471430;
		// August 5th, 2009, 11:23:50

	struct test {
		time_t		result;
		const char*	date;
		int			flags;
	};
	static const test kDates[] = {
		{1248739200,	"last tuesday", DAY_RELATIVE},
		{1249430400,	"today", DAY_RELATIVE},
		{1249948800,	"next tuesday", DAY_RELATIVE},
		{1249344000,	"tuesday", DAY_RELATIVE},
		{1249467830,	"last hour", MINUTE_RELATIVE},
		{1249475030,	"1 hour", MINUTE_RELATIVE},
		{now,			"now", MINUTE_RELATIVE},
		{219456000,		"1976-12-15", ABSOLUTE},
		{219456000,		"12/15/1976", ABSOLUTE},
		{219456000,		"1976/12/15", ABSOLUTE},
		{219456000,		"15.12.1976", ABSOLUTE},
		{208051200,		"5.8.1976", ABSOLUTE},
		{1061596800,	"Sat, 08/23/2003", ABSOLUTE},
		{1061596800,	"Sun, 08/23/2003", ABSOLUTE},
		{-1,			"", INVALID},
		{1249873200,	"next monday 3:00", DAY_RELATIVE},
		{1249533720,	"thursday 4:42", DAY_RELATIVE},
		{1249533740,	"thursday +4:42:20", DAY_RELATIVE},
		{1249533720,	"this thursday 4:42", DAY_RELATIVE},
		{1249473950,	"42 minutes", MINUTE_RELATIVE},
		{1250640000,	"2 weeks", DAY_RELATIVE},
		{1249471730,	"next 5 minutes", MINUTE_RELATIVE},
		{1249470530,	"last 15 minutes", MINUTE_RELATIVE},
		{1249470530,	"-15 minutes", MINUTE_RELATIVE},
		{1249486380,	"3:33pm GMT", DAY_RELATIVE},
		{739706403,		"Mon, June 10th, 1993 10:00:03 am GMT", ABSOLUTE},
		{-1,			"Sat, 16 Aug   Ìîñêîâñêîå âðåìÿ (çèìà)", INVALID},
		{739706403,		"Mon, June 10th, 1993 10:00:03 am UTC", ABSOLUTE},
		{739699203,		"Mon, June 10th, 1993 10:00:03 am CEST", ABSOLUTE},
		{739706403,		"Mon, June 10th, 1993 10:00:03 am +0000", ABSOLUTE},
		{-1,			"Mon, June 10th, 1993 10:00:03 am 0000", UNKNOWN},
		{739702803,		"Mon, June 10th, 1993 10:00:03 am +0100", ABSOLUTE},
		{739731603,		"Mon, June 10th, 1993 10:00:03 am -0700", ABSOLUTE},
		{739654203,		"Mon, June 10th, 1993 06:00:03 am ACDT", ABSOLUTE},
		{-1,			NULL, 0}
	};

	bool verbose = argc > 1;

	if (verbose)
		printf("All times relative to: %s (%ld)\n", ctime(&now), now);

	for (int32 i = 0; kDates[i].date; i++) {
		int flags = 0;
		time_t result = parsedate_etc(kDates[i].date, now, &flags);

		bool failure = false;
		if (flags != kDates[i].flags || result != kDates[i].result)
			failure = true;
		if (failure) {
			printf("FAILURE: parsing time at index %ld (should be %ld, %s)\n",
				i, kDates[i].result,
				parsedate_flags_to_string(kDates[i].result, flags));
		}

		if (verbose || failure) {
			printf("\"%s\" = %ld (%s) -> %s", kDates[i].date, result,
				parsedate_flags_to_string(result, flags), result == -1
					? "-\n" : ctime(&result));
		}
	}

	return 0;
}
