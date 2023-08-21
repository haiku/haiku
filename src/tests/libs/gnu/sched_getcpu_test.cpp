/*
 * Copyright 2023, Jérôme Duval, jerome.duval@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sched.h>
#include <stdio.h>


int
main(int argc, char** argv)
{
	int cpu = sched_getcpu();
	printf("cpu: %d\n", cpu);
	return 0;
}

