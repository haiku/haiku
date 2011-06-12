/*
 * Copyright (c) 2003, Intel Corporation. All rights reserved.
 * Created by:  salwan.searty REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 This program tests the assertion that if signal had been blocked, then
 sigset shall return SIG_HOLD. Steps:
 1. block SIGCHLD via sigprocmask()
 2. use sigset()

*/

#define _XOPEN_SOURCE 600

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "posixtest.h"

int main()
{
	sigset_t signalMask;
	if (sigemptyset(&signalMask) != 0) {
		printf("sigemtpyset() failed unexpectedly: %s\n", strerror(errno));
		return PTS_UNRESOLVED;
	}

	if (sigaddset(&signalMask, SIGCHLD) != 0) {
		printf("sigaddset() failed unexpectedly: %s\n", strerror(errno));
		return PTS_UNRESOLVED;
	}

	if (sigprocmask(SIG_BLOCK, &signalMask, NULL) != 0) {
		printf("sigprocmask() failed unexpectedly: %s\n", strerror(errno));
		return PTS_UNRESOLVED;
	}

        if (sigset(SIGCHLD,SIG_HOLD) != SIG_HOLD) {
		printf("Test FAILED: sigset() didn't return SIG_HOLD\n");
		return PTS_FAIL;
	}

	return PTS_PASS;
} 
