/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TIME_UTILS_H
#define TIME_UTILS_H


#include <stdio.h>


struct decomposed_bigtime {
	nanotime_t	hours;
	int			minutes;
	int			seconds;
	int			micros;
};


struct decomposed_nanotime {
	nanotime_t	hours;
	int			minutes;
	int			seconds;
	int			nanos;
};


static void
decompose_time(bigtime_t time, decomposed_bigtime& decomposed)
{
	// nanosecs
	decomposed.micros = time % 1000000;
	time /= 1000000;
	// secs
	decomposed.seconds = time % 60;
	time /= 60;
	// mins
	decomposed.minutes = time % 60;
	time /= 60;
	// hours
	decomposed.hours = time;
}


static void
decompose_time(nanotime_t time, decomposed_nanotime& decomposed)
{
	// nanosecs
	decomposed.nanos = time % 1000000000;
	time /= 1000000000;
	// secs
	decomposed.seconds = time % 60;
	time /= 60;
	// mins
	decomposed.minutes = time % 60;
	time /= 60;
	// hours
	decomposed.hours = time;
}


static inline const char*
format_bigtime(bigtime_t time, char* buffer, size_t bufferSize)
{
	decomposed_bigtime decomposed;
	decompose_time(time, decomposed);

	snprintf(buffer, bufferSize, "%02" B_PRId64 ":%02d:%02d:%06d",
		decomposed.hours, decomposed.minutes, decomposed.seconds,
		decomposed.micros);
	return buffer;
}


static inline BString
format_bigtime(bigtime_t time)
{
	char buffer[64];
	format_bigtime(time, buffer, sizeof(buffer));
	return BString(buffer);
}


static inline const char*
format_nanotime(nanotime_t time, char* buffer, size_t bufferSize)
{
	decomposed_nanotime decomposed;
	decompose_time(time, decomposed);

	snprintf(buffer, bufferSize, "%02" B_PRId64 ":%02d:%02d:%09d",
		decomposed.hours, decomposed.minutes, decomposed.seconds,
		decomposed.nanos);
	return buffer;
}


static inline BString
format_nanotime(nanotime_t time)
{
	char buffer[64];
	format_nanotime(time, buffer, sizeof(buffer));
	return BString(buffer);
}


#endif	// TIME_UTILS_H
