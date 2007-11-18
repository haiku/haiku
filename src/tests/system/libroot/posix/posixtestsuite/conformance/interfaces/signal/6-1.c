/*
 * Copyright (c) 2003, Intel Corporation. All rights reserved.
 * Created by:  salwan.searty REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 This program tests the assertion that signal() shall return SIG_ERR
 and set errno to a positive value if an invalid signal number was
 passed to it.
     
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "posixtest.h"

void myhandler(int signo)
{
	printf("signal_6-1: inside handler\n");
}

int main()
{
	errno = 0;

	if (signal(-1, myhandler) != SIG_ERR) {
                printf("Test FAILED: signal() didn't return SIG_ERR even though invalid signal number was passed to it\n");
               	return PTS_FAIL;
        }

	if (errno != EINVAL) {
		printf("Test FAILED: errno wasn't set to EINVAL even though invalid signal number was passed to the signal() function\n");
               	return PTS_FAIL;
        }
    printf("signal_6-1: Test PASSED\n");
	return PTS_PASS;
} 
