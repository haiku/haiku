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
#include <ktypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <String.h>


static	int		char_to_int(char in);
static	uint64	to_uint64(char *input);
static	int		Process(char *FromString);
static	int		Factor(uint64 BigNumber, uint64 FactorBy);
static	int		SuggestHelpMessage();
static	int		WarnNotValid(char *NotValidString);
static	int		usage(void);
static	int		version(void);

#define MAX_INT_CALCULATED "18446744073709551615"	// a fairly big number, 2^64 minus 1
#define MAX_TOKEN_LENGTH 20							// Hopefully we can change this upper limit in the future. ;)

char* prog_name;
bool SuggestHelp = false;


static int
char_to_int (char in)
{
	if (in >= '0' && in <= '9')
		return in - '0';

	return(0);
}


static uint64
to_uint64 (char *input)
{
	uint64 output = 0;
	while (*input && isdigit (*input)) {
		output = output * 10 + char_to_int (*input++);
	}
	return output;
}


static int
readline(char *buf, int len)
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
Process(char *FromString)
{
	char *TestForChar;
	bool FoundChar = false;
	uint64 NumberToFactor;
	int BaseFactor = 2;
	char str[1024];
	char *Token;
	const char *Tokens = " \t";
	bool factorNumber = true;
	int i;

	char maxint[] = MAX_INT_CALCULATED;
	BString chrptr; // predefined character pointer
	BString usrptr; // user submitted pointer

	strcpy(str,FromString);
	Token = strtok(str, Tokens);

	while (1) {
		TestForChar = Token;
		FoundChar = false;

		while (*TestForChar) {
			if (! isdigit (*TestForChar) ) {
				FoundChar = true;
				break;
			}
			*TestForChar++;
		}

		if (FoundChar) {
			WarnNotValid(Token);
		} else {
			if ( strlen(Token) >= MAX_TOKEN_LENGTH ) {

				for(i=0; i < MAX_TOKEN_LENGTH; i++) {
					chrptr << maxint[i];
					usrptr << Token[i];
					
					if ( usrptr > chrptr ) {
						WarnNotValid(Token);
						SuggestHelp = true;
						factorNumber = false;
						break;
						
					}
				}
			}
			
			if ( factorNumber ) {
				NumberToFactor = to_uint64(Token);
				(void)fprintf(stdout, "%Lu:", NumberToFactor);
				Factor(NumberToFactor, BaseFactor);
				(void)printf("\n");
			}
		}

		Token = strtok(NULL, Tokens);

		if (Token == NULL)
			break;
	}

	return(0);
} 


static int
Factor(uint64 BigNumber, uint64 FactorBy)
{
	if ( ( BigNumber == 1 ) || ( BigNumber == 0 ) ) {
		if ( BigNumber == 1 )
			(void)fprintf(stdout, " %d", 1);
		return(0);
	}
		
	uint64 RootNumber = (uint64)sqrt(BigNumber) + 1 ;

	while ( FactorBy <= RootNumber ) {
		if ( (BigNumber % FactorBy) == 0 ) {
			(void)fprintf(stdout, " %Lu", FactorBy);
			break;
		}
		FactorBy++;
	}

	if ( FactorBy > RootNumber ) {
		(void)fprintf(stdout, " %Lu", BigNumber);
		return(0);
	}
	Factor(BigNumber/FactorBy, FactorBy);
	return(0);
}


static int
SuggestHelpMessage()
{
	(void)fprintf(stdout, "Try `%s --help' for more information.\n", prog_name);
	return(0);
}


static int
WarnNotValid(char *NotValidString)
{
	(void)fprintf(stdout, "%s: `%s' is not a valid positive integer\n", prog_name, NotValidString);
	return(0);
}


static int
usage(void)
{
	(void)fprintf(stdout, "factor OBOS (http://www.openbeos.org/)

Usage: %s [NUMBER]...
   or: %s OPTION

	--help\t\tdisplay this help and exit
	--version\tdisplay version information and exit

  Print the prime factors of all specified positive integer NUMBERs.
  If no NUMBERS are specified on the command line, they are read from standard input.
  
  You may use a delimiter characters such as space and tab to separate NUMBERS.

", prog_name, prog_name );
	return(0);
}


static int
version(void)
{
	(void)fprintf(stdout, "%s OBOS (http://www.openbeos.org/)\n", prog_name);
	return(0);
}


int
main(int argc, char *argv[])
{
	int argOption;
	int indexptr = 0;
	char * const * optargv = argv;
	bool Processed = false;
	int i;
	char buf[1024];
	
	prog_name = argv[0];

	struct option help_opt = { "help", no_argument, 0, 1 } ;
	struct option version_opt = { "version", no_argument, 0, 2 } ;

	struct option options[] = {
	help_opt, version_opt, 0
	};

	for ( i=1; i<argc; i++ ) {
		if ((argv[i][0] >= '0') && (argv[i][0] <= '9')) {
			Processed = true;
			Process(argv[i]);
		} else {
			if ( ( argv[i][0] == '-' ) && (argv[i][1] >= '0') && (argv[i][1] <= '9') ) {
				Processed = true;
				WarnNotValid(argv[i]);
				SuggestHelp = true;
			}
		}
	}
	
	if ( SuggestHelp == true ) {
		SuggestHelpMessage();
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

	if ( (argc == 0) || (Processed == false) ) {

		// read stdin
		for (;;) {
			int chars_read;
			chars_read = readline(buf, sizeof(buf));
			if (chars_read > 0)
				Process(buf);
		}
	}
	return B_NO_ERROR;
}
