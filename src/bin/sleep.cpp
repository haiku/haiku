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

const bigtime_t microsPerSecond=1000000;

static	bigtime_t	process_arg(char *fromString);
static	void	warn_invalid(char *invalidArg);
static	void	usage(void);
static	void	version(void);

char *progName;

static bigtime_t
process_arg(char *fromString)
{
	bigtime_t microSeconds=0;
	bool endOfString=false;
	int i=0;
	while (i<20 && !endOfString) // Any more than 5 digits is ridiculous, but 20 is possible...
		switch (fromString[i++]) {
			case '0': microSeconds*=10;break;
			case '1': microSeconds=10*microSeconds+1;break;
			case '2': microSeconds=10*microSeconds+2;break;
			case '3': microSeconds=10*microSeconds+3;break;
			case '4': microSeconds=10*microSeconds+4;break;
			case '5': microSeconds=10*microSeconds+5;break;
			case '6': microSeconds=10*microSeconds+6;break;
			case '7': microSeconds=10*microSeconds+7;break;
			case '8': microSeconds=10*microSeconds+8;break;
			case '9': microSeconds=10*microSeconds+9;break;
			case 'u': case 'U': endOfString=true; break;
			case 'i': case 'I': microSeconds*=1000; endOfString=true;break;
			case 's': case 'S': case '\0': microSeconds*=microsPerSecond; endOfString=true; break; 
			case 'm': case 'M': microSeconds*= (60*microsPerSecond);endOfString=true; break; 
			case 'h': case 'H': microSeconds*= (3600*microsPerSecond);endOfString=true; break; 
			case 'd': case 'D': microSeconds*= (86400*microsPerSecond);endOfString=true; break; 
		}
	if (fromString[i-1]!='\0' && fromString[i]!='\0') 
		warn_invalid(fromString);
	return (microSeconds);
}
	
static void
warn_invalid(char *invalidArg)
{
	fprintf(stderr, "%s: invalid time interval `%s'\n", progName, invalidArg);
	fprintf(stdout, "Try `%s --help' for more information.\n", progName);
	exit(1);
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
	bigtime_t sleepTotal = 0; // in microseconds actually. 

	struct option options[] = {
	{ "help", no_argument, NULL, 1 },
	{ "version", no_argument, NULL, 2 },
	{ NULL,0,NULL,0}
	};

	progName = argv[0]; 

	while ((argOption = getopt_long(argc, optargv, "", options, &indexptr)) != -1) {
		switch (argOption) { 
			case 1: // help
				usage();
				break;
			case 2: // version
				version();
				break;
			default: // Should not be possible to get here!
				fprintf (stderr,"HUGE problems with getopt_long!\n");
				exit(1);
				break;
			}
	exit(0);
	}

	if (argc - optind > 0)  {
		for ( int i=optind; i<argc; i++ ) 
			if ((argv[i][0] >= '0') && (argv[i][0] <= '9')) 
				sleepTotal += process_arg(argv[i]);
			else 
				warn_invalid(argv[optind]);
		snooze(sleepTotal);
		}
	else 
		usage();
	
	return B_NO_ERROR;
}
