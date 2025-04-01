/*
 * Copyright 2025, Jérôme Duval, jerome.duval@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>


int
main(int argc, char** argv)
{
	double loadavg[3];
	int count = getloadavg(loadavg, 3);
	if (count == -1)
		return 1;
	printf("getloadavg() returned %" PRId32 ", values %f %f %f\n", count, loadavg[0], loadavg[1],
		loadavg[2]);

	return 0;
}
