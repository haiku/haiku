/*
 * Copyright 2017-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


#define MILLIS_IN_DAY 86400000L

#define HDLOGLEVELCHAR(L) ( \
	L == LOG_LEVEL_INFO ? 'I' \
	: L == LOG_LEVEL_DEBUG ? 'D' \
	: L == LOG_LEVEL_TRACE ? 'T' \
	: L == LOG_LEVEL_ERROR ? 'E' \
	: '?')

// These macros allow for standardized logging to be output.
// The use of macros in this way means that the use of the log is concise where
// it is used and also because the macro unwraps to a block contained with a
// condition statement, if the log level is not sufficient to trigger the log
// line then there is no computational cost to running over the log space.  This
// is because the arguments will not be evaluated.  Avoiding all of the
// conditional clauses in the code to prevent this otherwise would be
// cumbersome.

#define HDLOGPREFIX(L) printf("@%08" B_PRId64 " {%c} <t:%" B_PRId32 "> ", \
	((system_time() / 1000L) % MILLIS_IN_DAY), \
	HDLOGLEVELCHAR(L), \
	abs(find_thread(NULL) % 1000) \
);

#define HDLOG(L, M...) do { if (Logger::IsLevelEnabled(L)) { \
	HDLOGPREFIX(L) \
	printf(M); \
	putchar('\n'); \
} } while (0)

#define HDINFO(M...) HDLOG(LOG_LEVEL_INFO, M)
#define HDDEBUG(M...) HDLOG(LOG_LEVEL_DEBUG, M)
#define HDTRACE(M...) HDLOG(LOG_LEVEL_TRACE, M)
#define HDERROR(M...) HDLOG(LOG_LEVEL_ERROR, M)

#define HDFATAL(M...) do { \
	printf("{!} (failed @ %s:%d) ", __FILE__, __LINE__); \
	printf(M); \
	putchar('\n'); \
	exit(EXIT_FAILURE); \
} while (0)

typedef enum log_level {
	LOG_LEVEL_OFF		= 1,
	LOG_LEVEL_ERROR		= 2,
	LOG_LEVEL_INFO		= 3,
	LOG_LEVEL_DEBUG		= 4,
	LOG_LEVEL_TRACE		= 5
} log_level;


class Logger {
public:
	static	log_level			Level();
	static	void				SetLevel(log_level value);
	static	bool				SetLevelByName(const char *name);

	static	const char*			NameForLevel(log_level value);

	static	bool				IsLevelEnabled(log_level value);
	static	bool				IsInfoEnabled();
	static	bool				IsDebugEnabled();
	static	bool				IsTraceEnabled();

private:
	static	log_level			fLevel;
};


#endif // LOGGER_H
