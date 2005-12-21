/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>
#include <parsedate.h>

#include <stdio.h>

// this file can also be compiled against the R5 parsedate() implementation
#ifndef PARSEDATE_INVALID_DATE
#	define PARSEDATE_INVALID_DATE 4
#	define PARSEDATE_MINUTE_RELATIVE_TIME 8
#endif


char *
parsedate_flags_to_string(time_t result, int flags)
{
	if (result == -1) {
		if (flags & PARSEDATE_INVALID_DATE)
			return "invalid";
		return "unknown";
	}

	if (flags & PARSEDATE_RELATIVE_TIME) {
		if (flags & PARSEDATE_MINUTE_RELATIVE_TIME)
			return "minute relative";

		return "day relative";
	}
	
	return "absolute";
}


int 
main(int argc, char **argv)
{
	const char *dates[] = {
		"last tuesday",
		"today",
		"next tuesday",
		"tuesday",
		"1976-12-15",
		"5.8.1976",
		"last hour",
		"1 hour",
		"now",
		"12/15/1976",
		"Sat, 08/23/2003",
		"Sun, 08/23/2003",
		"",
		"next monday 3:00",
		"thursday 4:42",
		"thursday +4:42",
		"this thursday 4:42",
		"42 minutes",
		"2 weeks",
		"next 5 minutes",
		"last 15 minutes",
		"-15 minutes",
		"3:33pm GMT",
		"Mon, June 10th, 1993 10:00:03 am GMT",
		NULL
	};

#if 0
	time_t now = time(NULL);
	for (int i = 0; i < 500000; i++) {
		int flags = 0;
		parsedate_etc(dates[0], now, &flags);
	}
#else
	// this crashes the R5 version but not ours:
	// parsedate(NULL, -1);

	for (int32 i = 0; dates[i]; i++) {
		int flags = 0;
		time_t result = parsedate_etc(dates[i], -1, &flags);
		printf("\"%s\" = %ld (%s) -> %s", dates[i], result,
			parsedate_flags_to_string(result, flags), result == -1 ? "-\n" : ctime(&result));
	}
#endif
	return 0;
}
