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

int main(int argc, char *argv[]) 
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

	while ((argOption = getopt_long(argc, optargv, "LP", options, &indexptr)) != -1)
		switch (argOption) { 
			case '?':
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
	argc -= optind;

	if (argc != 0)
	{
		usage();
	}
	else
	{
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


int usage(void)
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

int version(void)
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

