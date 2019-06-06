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
	system_info info;
	status_t status = get_system_info(&info);
	if (status != B_OK) {
		fprintf(stderr, "%s: cannot get system info: %s\n", kProgramName,
			strerror(status));
		return 1;
	}

	printf("max memory:\t\t%" B_PRIu64 "\n", info.max_pages * B_PAGE_SIZE);
	printf("free memory:\t\t%" B_PRIu64 "\n", info.free_memory);
	printf("needed memory:\t\t%" B_PRIu64 "\n", info.needed_memory);
	printf("block cache memory:\t%" B_PRIu64 "\n",
		info.block_cache_pages * B_PAGE_SIZE);
	printf("max swap space:\t\t%" B_PRIu64 "\n",
		info.max_swap_pages * B_PAGE_SIZE);
	printf("free swap space:\t%" B_PRIu64 "\n",
		info.free_swap_pages * B_PAGE_SIZE);
	printf("page faults:\t\t%" B_PRIu32 "\n", info.page_faults);

	if (periodically) {
		puts("\npage faults  used memory    used swap  block cache");
		system_info lastInfo = info;

		while (true) {
			snooze(rate);

			get_system_info(&info);

			int32 pageFaults = info.page_faults - lastInfo.page_faults;
			int64 usedMemory
				= (info.max_pages * B_PAGE_SIZE - info.free_memory)
					- (lastInfo.max_pages * B_PAGE_SIZE - lastInfo.free_memory);
			int64 usedSwap
				= ((info.max_swap_pages - info.free_swap_pages)
						- (lastInfo.max_swap_pages - lastInfo.free_swap_pages))
					* B_PAGE_SIZE;
			int64 blockCache
				= (info.block_cache_pages - lastInfo.block_cache_pages)
					* B_PAGE_SIZE;
			printf("%11" B_PRId32 "  %11" B_PRId64 "  %11" B_PRId64 "  %11"
				B_PRId64 "\n", pageFaults, usedMemory, usedSwap, blockCache);

			lastInfo = info;
		}
	}

	return 0;
}
