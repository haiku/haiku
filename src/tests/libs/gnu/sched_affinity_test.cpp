/*
 * Copyright 2023, Jérôme Duval, jerome.duval@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>


int
main(int argc, char** argv)
{
	if (argc != 2)
		exit(1);
	if (strcmp(argv[1], "off") != 0) {
		printf("with affinity on CPU 1\n");
		pthread_t thread = pthread_self();
		cpuset_t cpuset;
		CPUSET_ZERO(&cpuset);
		CPUSET_SET(1, &cpuset);
		if (pthread_setaffinity_np(thread, sizeof(cpuset_t), &cpuset) != 0) {
			fprintf(stderr, "pthread_setaffinity_np failed\n");
			exit(1);
		}
		if (pthread_getaffinity_np(thread, sizeof(cpuset_t), &cpuset) != 0) {
			fprintf(stderr, "pthread_getaffinity_np failed\n");
			exit(1);
		}
		if (!CPUSET_ISSET(1, &cpuset))
			fprintf(stderr, "affinity not on CPU 1\n");
		if (sched_getcpu() != 1)
			fprintf(stderr, "not running on CPU 1\n");
	} else {
		printf("without affinity\n");
	}
	int* p;
	while (true)
		(*p)++;
	return *p;
}
