/*
 * Copyright (c) 1991, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <ktypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *getcwd_logical(void);
int usage(void);
int version(void);

int
main(int argc, char *argv[]) 
{ 
	int physical = false; 
	int argOption;
	char *pcTemp; 
	int indexptr = 0;
	char * const * optargv = argv;

	struct option help_opt = { "help", no_argument, 0, 1 } ;
	struct option version_opt = { "version", no_argument, 0, 2 } ;

	struct option options[] = {
	help_opt, version_opt, 0
	};

	while ((argOption = getopt_long(argc, optargv, "LP", options, &indexptr)) != -1) {
	
		switch (argOption) { 
			case 'L': // Logical
				physical = false;
				break; 
			case 'P': // Physical
				physical = true;
				break; 
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

	if (argc != 0) {
		usage();
	}
	else {
		/*
		 * If the logical option was specified, and we can not find the logical currrent directory 
		 * then behave as if -P (physical) was specified.
		 */
        if ((!physical && (pcTemp = getcwd_logical()) != NULL) || (pcTemp = getcwd(NULL, 0)) != NULL)
			printf("%s\n", pcTemp);
        else
			exit(0);
	}
	return B_NO_ERROR;
}


int
usage(void)
{
	(void)fprintf(stderr, 
"pwd OBOS (C)2003 MIT License. (http://www.openbeos.net/)

Usage: pwd [OPTION]...
Print the current working directory path

	--help		display this help and exit
	--version	display version information and exit
	-L		treat path as a Logical device
	-P		treat path as a Physical device (default)\n"	);
	return;
}

int
version(void)
{
	(void)fprintf(stderr, "pwd OBOS (C)2003 MIT License. (http://www.openbeos.net/)\n");
	return;
}

static char *
getcwd_logical(void)
{
	struct stat logical, physical;
	char *pwd;

	/*
	* Check that $PWD is an absolute logical pathname referring to the current working directory.
	*/
	if ((pwd = getenv("PWD")) != NULL && *pwd == '/') {
		if (stat(pwd, &logical) == -1 || stat(".", &physical) == -1)
			return (NULL);
		if (logical.st_dev == physical.st_dev && logical.st_ino == physical.st_ino)
		return (pwd);
	}

	errno = ENOENT;
	return (NULL);
}

