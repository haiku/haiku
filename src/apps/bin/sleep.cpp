/*
** Copyright (c) 2004 OBOS
** Permission is hereby granted, free of charge, to any person obtaining a copy 
** of this software and associated documentation files (the "Software"), to deal 
** in the Software without restriction, including without limitation the rights 
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
** copies of the Software, and to permit persons to whom the Software is 
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in all 
** copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
** SOFTWARE.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <OS.h>

#define UINT64_MAX (18446744073709551615LL)

static	uint64	process_arg(char *fromString);
static	void	warn_invalid(char *invalidArg);
static	void	suggest_help(void);
static	void	go_sleep(uint64 sleepTotal);
static	void	usage(void);
static	void	version(void);

char *progName;


void
go_sleep(uint64 sleepTotal)
{
	snooze(sleepTotal);
}


static uint64
process_arg(char *fromString)
{
	long double processed;
	uint64 total;
	char lastChar;
	bool invalidChars = false;
	bool microseconds = false;
	bool milliseconds = false;
	int i, length;

	length = strlen(fromString);
	lastChar = fromString[length - 1];

	processed = atol(fromString);

	if ( processed > INT_MAX )
		processed = INT_MAX;
	
	if ( processed == 0 ) {
		warn_invalid(fromString);
		suggest_help();
		exit(1);
	}
	
	switch(lastChar) {
		case 'u':
		case 'U':
			microseconds = true;
			break;
		case 'i':
		case 'I':
			milliseconds = true;
			break;
		case 's':
		case 'S':
			break;
		case 'm':
		case 'M':
			processed *= 60;
			break;
		case 'h':
		case 'H':
			processed *= 3600;
			break;
		case 'd':
		case 'D':
			processed *= 86400;
			break;
		default: {
			for (i=0; i < length; i++) {
				if (! (fromString[i] >= '0' && fromString[i] <= '9'))
					invalidChars = true;
			}
			if (invalidChars) {
				warn_invalid(fromString);
				suggest_help();
				exit(1);
			}
		}
	}
	total = (uint64) processed;
	if (microseconds)
		total *= 1;
	else if (milliseconds)
		total *= 1000;
	else
		total *= 1000000;

	if (total > UINT64_MAX ) {
		total = UINT64_MAX;
	}
	return(total);
}


static void
warn_invalid(char *invalidArg)
{
	fprintf(stderr, "%s: invalid time interval `%s'\n", progName, invalidArg);
}


static void
suggest_help(void)
{
	fprintf(stdout, "Try `%s --help' for more information.\n", progName);
}


void
usage(void)
{
	fprintf(stderr, 
"%s OBOS (http://www.openbeos.org/)

Usage: %s [OPTION]... NUMBER[SUFFIX]

SUFFIX may be:

  u  microseconds
  i  milliseconds
  s  seconds (default if no SUFFIX given)
  m  minutes
  h  hours
  d  days

     --help      display this help and exit
     --version   output version information and exit

Pause for NUMBER seconds.
	
", progName, progName	);
}


void
version(void)
{
	fprintf(stderr, "%s OBOS (http://www.openbeos.org/)\n", progName);
}


int
main(int argc, char *argv[]) 
{ 
	int argOption;
	int indexptr = 0;
	char * const * optargv = argv;
	int i;
	uint64 sleepTime = 0;
	uint64 sleepTotal = 0; // in microseconds actually. 

	struct option helpOption = { "help", no_argument, 0, 1 } ;
	struct option versionOption = { "version", no_argument, 0, 2 } ;

	struct option options[] = {
	helpOption, versionOption, {0}
	};

	progName = argv[0]; // don't put this before or between structs! werrry bad things happen. ;)

	while ((argOption = getopt_long(argc, optargv, "", options, &indexptr)) != -1) {
	
		switch (argOption) { 
			default:
				switch (options[indexptr].val) {
					case 1: // help
						usage();
						exit(0);
					case 2: // version
						version();
						exit(0);
					default:
						suggest_help();
						exit(1);
				}
				break;
		}
	}

	if (argc - optind > 0) {
		for ( i=1; i<argc; i++ ) {
			if ((argv[i][0] >= '0') && (argv[i][0] <= '9')) {
				sleepTime = process_arg(argv[i]);
				sleepTotal += sleepTime;
			} else {
				warn_invalid(argv[optind]);
				suggest_help();
				exit(1);
			}
		}
		go_sleep(sleepTotal);
	} else {
		usage();
	}
	
	return B_NO_ERROR;
}
