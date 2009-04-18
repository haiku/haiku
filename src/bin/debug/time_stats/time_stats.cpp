/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <OS.h>

#include "time_stats.h"


#define SCHEDULING_ANALYSIS_BUFFER_SIZE	10 * 1024 * 1024


extern const char* __progname;

static const char* kUsage =
	"Usage: %s [ <options> ] <command line>\n"
	"Executes the given command line <command line> and print an analysis of\n"
	"the user and kernel times of all threads that ran during that time.\n"
	"\n"
	"Options:\n"
	"  -b <size>    - When doing scheduling analysis: the size of the buffer\n"
	"                 used (in MB)\n"
	"  -h, --help   - Print this usage info.\n"
	"  -o <output>  - Print the results to file <output>.\n"
	"  -s           - Also perform a scheduling analysis over the time the\n"
	"                 child process ran. This requires that scheduler kernel\n"
	"                 tracing had been enabled at compile time.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, __progname);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	const char* outputFile = NULL;
	bool schedulingAnalysis = false;
	size_t bufferSize = SCHEDULING_ANALYSIS_BUFFER_SIZE;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "output", required_argument, 0, 'o' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+b:ho:s", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'b':
				bufferSize = atol(optarg);
				if (bufferSize < 1 || bufferSize > 1024) {
					fprintf(stderr, "Error: Invalid buffer size. Should be "
						"between 1 and 1024 MB\n");
					exit(1);
				}
				bufferSize *= 1024 * 1024;
				break;
			case 'h':
				print_usage_and_exit(false);
				break;
			case 'o':
				outputFile = optarg;
				break;
			case 's':
				schedulingAnalysis = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind >= argc)
		print_usage_and_exit(true);

	// open output file, if specified
	int outFD = -1;
	if (outputFile != NULL) {
		outFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP| S_IROTH | S_IWOTH);
		if (outFD < 0) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n", outputFile,
				strerror(errno));
			exit(1);
		}
	}

	do_timing_analysis(argc - optind, argv + optind, schedulingAnalysis, outFD,
		bufferSize);

	return 0;
}
