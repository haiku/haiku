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


/*
** Thanks go to:
** Eli Dayan, Jeremy Friesner, Jonathan Thompson, 
** Matthijs Hollemans, Jack Burton, and more.
*/


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <String.h>


static	int		char_to_int(char in);
static	uint64	to_uint64(char *input);
static	int		process_potential_number(char *fromString);
static	int		factor_number(uint64 bigNumber, uint64 factorBy);
static	int		suggest_help(void);
static	int		warn_invalid(char *fromString);
static	void	usage(void);
static	void	version(void);

#define MAX_INT_CALCULATED "18446744073709551615"	// a fairly big number, 2^64 minus 1
#define MAX_TOKEN_LENGTH 20							// Hopefully we can change this upper limit in the future. ;)

char* progName;
bool suggestHelp = false;


static int
char_to_int(char in)
{
	if (in >= '0' && in <= '9')
		return in - '0';
	return(0);
}


static uint64
to_uint64(char *input)
{
	uint64 output = 0;
	while (*input && isdigit (*input)) {
		output = output * 10 + char_to_int (*input++);
	}
	return output;
}


static int
read_line(char *buf, int len)
{
	int i = 0;
	char ch;

	while (true) {
		ch = getchar();
		switch (ch) {

			// backspace, erase the last character written
			case '\b':
				if (i > 0) {
					printf("\b");
					--i;
				}
			break;
	
			// enter key pressed
			case '\n':
				buf[i] = 0;
				return(i);
	
			default:
				buf[i] = ch;
				i++;
				break;
		}
	}
	return(0);
}


static int
process_potential_number(char *fromString)
{
	char *testForChar;
	bool foundChar = false;
	uint64 numberToFactor;
	int baseFactor = 2;
	char str[1024];
	char *token;
	const char *token_set = " \t";
	bool factorNumber = true;
	int i;

	char maxint[] = MAX_INT_CALCULATED;
	BString chrptr; // predefined character pointer
	BString usrptr; // user submitted pointer

	strcpy(str,fromString);
	token = strtok(str, token_set);

	while (1) {
		testForChar = token;
		foundChar = false;

		while (*testForChar) {
			if (! isdigit (*testForChar) ) {
				foundChar = true;
				break;
			}
			*testForChar++;
		}

		if (foundChar) {
			warn_invalid(token);
		} else {
			if ( strlen(token) >= MAX_TOKEN_LENGTH ) {

				for(i=0; i < MAX_TOKEN_LENGTH; i++) {
					chrptr << maxint[i];
					usrptr << token[i];
					
					if ( usrptr > chrptr ) {
						warn_invalid(token);
						suggestHelp = true;
						factorNumber = false;
						break;
						
					}
				}
			}
			
			if ( factorNumber ) {
				numberToFactor = to_uint64(token);
				fprintf(stdout, "%Lu:", numberToFactor);
				factor_number(numberToFactor, baseFactor);
				fprintf(stdout, "\n");
			}
		}

		token = strtok(NULL, token_set);

		if (token == NULL)
			break;
	}

	return(0);
} 


static int
factor_number(uint64 bigNumber, uint64 factorBy)
{
	if ( ( bigNumber == 1 ) || ( bigNumber == 0 ) ) {
		if ( bigNumber == 1 )
			fprintf(stdout, " %d", 1);
		return(0);
	}
		
	uint64 rootNumber = (uint64)sqrt(bigNumber) + 1 ;

	while ( factorBy <= rootNumber ) {
		if ( (bigNumber % factorBy) == 0 ) {
			fprintf(stdout, " %Lu", factorBy);
			break;
		}
		factorBy++;
	}

	if ( factorBy > rootNumber ) {
		fprintf(stdout, " %Lu", bigNumber);
		return(0);
	}
	factor_number(bigNumber/factorBy, factorBy);
	return(0);
}


static int
suggest_help()
{
	fprintf(stderr, "Try `%s --help' for more information.\n", progName);
	return(0);
}


static int
warn_invalid(char *fromString)
{
	fprintf(stderr, "%s: `%s' is not a valid positive integer\n", progName, fromString);
	return(0);
}


static void
usage(void)
{
	(void)fprintf(stdout, "%s OBOS (http://www.openbeos.org/)

Usage: %s [NUMBER]...
   or: %s OPTION

     --help      display this help and exit
     --version   output version information and exit

  Print the prime factors of all specified positive integer NUMBERs.
  If no NUMBERS are specified on the command line, they are read from standard input.
  
  You may use a delimiter characters such as space and tab to separate NUMBERS.

", progName, progName, progName );
}


static void
version(void)
{
	fprintf(stdout, "%s OBOS (http://www.openbeos.org/)\n", progName);
}


int
main(int argc, char *argv[])
{
	int argOption;
	int indexptr = 0;
	char * const * optargv = argv;
	bool processed = false;
	int i;
	char buf[1024];
	
	progName = argv[0];

	struct option helpOption = { "help", no_argument, 0, 1 } ;
	struct option versionOption = { "version", no_argument, 0, 2 } ;

	struct option options[] = {
	helpOption, versionOption, {0}
	};

	for ( i=1; i<argc; i++ ) {
		if ((argv[i][0] >= '0') && (argv[i][0] <= '9')) {
			processed = true;
			process_potential_number(argv[i]);
		} else {
			if ( ( argv[i][0] == '-' ) && (argv[i][1] >= '0') && (argv[i][1] <= '9') ) {
				processed = true;
				warn_invalid(argv[i]);
				suggestHelp = true;
			}
		}
	}
	
	if ( suggestHelp == true ) {
		suggest_help();
		exit(0);
	}

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
						break;
				}
				break;
		}
	}

	argc -= optind;

	if ( (argc == 0) || (processed == false) ) {

		// read stdin
		for (;;) {
			int chars_read;
			chars_read = read_line(buf, sizeof(buf));
			if (chars_read > 0)
				process_potential_number(buf);
		}
	}
	return B_NO_ERROR;
}
