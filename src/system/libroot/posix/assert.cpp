/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#undef NDEBUG
	// just in case

#include <OS.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char* __progname;


void
__assert_fail(const char* assertion, const char* file, unsigned int line,
	const char* function)
{
	fprintf(stderr, "%s: %s:%d:%s: %s\n", __progname, file, line, function,
		assertion);

	// If there's no handler installed for SIGABRT, call debugger().
	struct sigaction signalAction;
	if (sigaction(SIGABRT, NULL, &signalAction) == 0
			&& signalAction.sa_handler == SIG_DFL) {
		debugger(assertion);
	}

	abort();
}


void
__assert_perror_fail(int error, const char* file, unsigned int line,
	const char* function)
{
	__assert_fail(strerror(error), file, line, function);
}
