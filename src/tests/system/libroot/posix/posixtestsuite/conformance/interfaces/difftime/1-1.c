/*   
 * Copyright (c) 2002-3, Intel Corporation. All rights reserved.
 * Created by:  salwan.searty REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this 
 * source tree.

 Test that the difftime function shall return the difference between 
 two calendar times.
 */

#define WAIT_DURATION 1

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "posixtest.h"

int main()
{
	time_t time1, time0;

	double time_diff;
	time_diff = 0;
	time0 = time(NULL);
	sleep(WAIT_DURATION);
	time1 = time(NULL);
	time_diff = difftime(time1, time0);

	if (time_diff != WAIT_DURATION) {
		printf("%sdifftime_1-1: Test FAILED: "
			"difftime did not return the correct value%s\n",
			red, normal);
		return PTS_FAIL;
	}

	printf("%sdifftime_1-1:%s               %sPASSED%s\n", boldOn, boldOff, green, normal);
	return PTS_PASS;
}

