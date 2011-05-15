/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <system_info.h>


static struct option const kLongOptions[] = {
	{"periodic", no_argument, 0, 'p'},
	{"rate", required_argument, 0, 'r'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *kProgramName = __progname;


void
usage(int status)
{
	fprintf(stderr, "usage: %s [-p] [-r <time>]\n"
		" -p,--periodic\tDumps changes periodically every second.\n"
		" -r,--rate\tDumps changes periodically every <time> milli seconds.\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char** argv)
{
	bool periodically = false;
	bigtime_t rate = 1000000LL;

	int c;
	while ((c = getopt_long(argc, argv, "pr:h", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'p':
				periodically = true;
				break;
			case 'r':
				rate = atoi(optarg) * 1000LL;
				if (rate <= 0) {
					fprintf(stderr, "%s: Invalid rate: %s\n",
						kProgramName, optarg);
					return 1;
				}
				periodically = true;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}
	system_memory_info info;
	status_t status = __get_system_info_etc(B_MEMORY_INFO, &info,
		sizeof(system_memory_info));
	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get system info: %s\n", kProgramName,
			strerror(status));
		return 1;
	}

	printf("max memory:\t\t%Lu\n", info.max_memory);
	printf("free memory:\t\t%Lu\n", info.free_memory);
	printf("needed memory:\t\t%Lu\n", info.needed_memory);
	printf("block cache memory:\t%Lu\n", info.block_cache_memory);
	printf("max swap space:\t\t%Lu\n", info.max_swap_space);
	printf("free swap space:\t%Lu\n", info.free_swap_space);
	printf("page faults:\t\t%lu\n", info.page_faults);

	if (periodically) {
		puts("\npage faults  used memory    used swap  block cache");
		system_memory_info lastInfo = info;

		while (true) {
			snooze(rate);

			__get_system_info_etc(B_MEMORY_INFO, &info,
				sizeof(system_memory_info));

			printf("%11ld  %11Ld  %11Ld  %11Ld\n",
				(int32)info.page_faults - lastInfo.page_faults,
				(info.max_memory - info.free_memory)
					- (lastInfo.max_memory - lastInfo.free_memory),
				(info.max_swap_space - info.free_swap_space)
					- (lastInfo.max_swap_space - lastInfo.free_swap_space),
				info.block_cache_memory - lastInfo.block_cache_memory);

			lastInfo = info;
		}
	}

	return 0;
}
