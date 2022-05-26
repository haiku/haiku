/*
** Copyright (c) 2004 Haiku
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


#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static	void	print_user_info(int userID, char *suffix);
static	void	print_group_info(int groupID, char *suffix);
static	void	print_group_list(int listID);
static	void	print_combined_info(void);
static	void	suggest_help(void);
static	void	usage(void);
static	void	version(void);

char *progName;
static int gFlag, glFlag, nFlag, rFlag, uFlag;
struct passwd *euidName;
struct passwd *ruidName;
struct group *egidName;
struct group *rgidName;
uid_t eUID;
uid_t rUID;
gid_t eGID;
gid_t rGID;
gid_t groupList;
char *suffix;


static void
print_user_info(int userID, char *suffix) {
	struct stat statBuffer;
	struct passwd *userIDName;
	if ((userIDName = getpwuid(userID)) != NULL) {
		if (nFlag)
   			fprintf(stdout, "%s%s", userIDName->pw_name, suffix);
		else 
    		fprintf(stdout, "%u%s", eUID, suffix); 
	} else
   		fprintf(stdout, "%-8d%s", statBuffer.st_uid, suffix);
}


static void
print_group_info(int groupID, char *suffix) {
	struct stat statBuffer;
	struct group *groupIDName;
	if ((groupIDName = getgrgid(groupID)) != NULL) {
    	if (nFlag)
    		fprintf(stdout, "%s%s", groupIDName->gr_name, suffix);
    	else
    		fprintf(stdout, "%u%s", groupID, suffix);
	} else
    	fprintf(stdout, " %-8d%s", statBuffer.st_gid, suffix);
}

static void
print_group_list(int groupID) {
	int cnt, id, lastID, nGroups;
	gid_t *groups; 
	long ngroups_max; 

	ngroups_max = sysconf(NGROUPS_MAX) + 1; 
	groups = (gid_t *)malloc(ngroups_max *sizeof(gid_t)); 

	nGroups = getgroups(ngroups_max, groups);

	suffix = "";
	print_group_info(groupID, suffix);
	for (lastID = -1, cnt = 0; cnt < ngroups_max; ++cnt) {
		if (lastID == (id = groups[cnt]))
			continue;
		suffix = " ";
		print_group_info(id, suffix);
		lastID = id;
	}
	fprintf(stdout, "\n");
}


static void
print_combined_info() {
	if ( eUID != rUID ) {
		suffix = "";
		rFlag = 1;
		fprintf(stdout, "uid=");
		print_user_info(rUID, suffix);
		fprintf(stdout, "(");
		nFlag = 1;
		print_user_info(rUID, suffix);
		fprintf(stdout, ") gid=");
		rFlag = 1;	nFlag = 0;
		print_group_info(rGID, suffix);
		fprintf(stdout, "(");
		rFlag = 0; nFlag = 1;
		print_group_info(eGID, suffix);
		fprintf(stdout, ") euid=");
		rFlag = 0;	nFlag = 0;
		print_user_info(eUID, suffix);
		fprintf(stdout, "(");
		rFlag = 0; nFlag = 1;
		print_user_info(eUID, suffix);
		fprintf(stdout, ")\n");
	} else {
		suffix = "";
		rFlag = 1;
		fprintf(stdout, "uid=");
		print_user_info(rUID, suffix);
		fprintf(stdout, "(");
		nFlag = 1;
		print_user_info(rUID, suffix);
		fprintf(stdout, ") gid=");
		rFlag = 1;	nFlag = 0;
		print_group_info(rGID, suffix);
		fprintf(stdout, "(");
		rFlag = 1;	nFlag = 1;
		print_group_info(rGID, suffix);
		fprintf(stdout, ")\n");
	}
}


static void
suggest_help(void)
{
	(void)fprintf(stdout, "Try `%s --help' for more information.\n", progName);
}


static void
usage(void)
{
	fprintf(stdout, 
"%s Haiku (https://www.haiku-os.org/)

Usage: %s [OPTION]... [USERNAME]

  -g, --group     print only the group ID
  -G, --groups    print only the supplementary groups
  -n, --name      print a name instead of a number, for -ugG
  -r, --real      print the real ID instead of effective ID, for -ugG
  -u, --user      print only the user ID
      --help      display this help and exit
      --version   output version information and exit

Print information for USERNAME, or the current user.
	
", progName, progName	);
}


static void
version(void)
{
	fprintf(stdout, "%s Haiku (https://www.haiku-os.org/)\n", progName);
}


int
main(int argc, char *argv[]) 
{ 
	int argOption;
	int indexptr = 0;
	char * const * optargv = argv;

	struct option groupOption = { "group", no_argument, 0, 1 } ;
	struct option groupsOption = { "groups", no_argument, 0, 2 } ;
	struct option nameOption = { "name", no_argument, 0, 3 } ;
	struct option realOption = { "real", no_argument, 0, 4 } ;
	struct option userOption = { "user", no_argument, 0, 5 } ;
	struct option helpOption = { "help", no_argument, 0, 6 } ;
	struct option versionOption = { "version", no_argument, 0, 7 } ;

	struct option options[] = {
	groupOption, groupsOption, nameOption, realOption, userOption, helpOption, versionOption, {0}
	};

	struct passwd *suppliedName;

	gFlag = glFlag = nFlag = rFlag = uFlag = 0;
	progName = argv[0]; // don't put this before or between structs! werrry bad things happen. ;)

	while ((argOption = getopt_long(argc, optargv, "gGnru", options, &indexptr)) != -1) {
	
		switch (argOption) { 
			case 'g':
				gFlag = 1;
				break; 
			case 'G':
				glFlag = 1;
				break; 
			case 'n':
				nFlag = 1;
				break; 
			case 'r':
				rFlag = 1;
				break; 
			case 'u':
				uFlag = 1;
				break; 
			default:
				switch (options[indexptr].val) {
					case 6: // help
						usage();
						exit(0);
					case 7: // version
						version();
						exit(0);
					default:
						suggest_help();
						exit(0);
				}
				break;
		}
	}

	if (argc - optind > 1)
		usage();
	
	if (argc - optind == 1) {
		suppliedName = getpwnam(argv[optind]);
	
		if (suppliedName == NULL) {
			fprintf(stderr, "%s: %s: No such user\n", progName, argv[optind]);
			suggest_help();
			exit(1);
		}
		rUID = eUID = suppliedName->pw_uid;
		rGID = eGID = suppliedName->pw_gid;
    } else {
		eUID = geteuid ();
		rUID = getuid ();
		eGID = getegid ();
		rGID = getgid ();
		
		euidName = getpwuid(eUID);
		ruidName = getpwuid(rUID);
		egidName = getgrgid(eGID);
		rgidName = getgrgid(rGID);
    }

	if ( gFlag + glFlag + uFlag > 1 ) {
		fprintf(stderr, "%s: cannot print only user and only group\n", progName);
		suggest_help();
		exit(1);
	}

	if ( gFlag + glFlag + uFlag == 0 && (rFlag || nFlag)) {
		fprintf(stderr, "%s: cannot print only names or real IDs in default format\n", progName); 
		suggest_help();
		exit(1);
	}

	if (gFlag) {
	// group information
		suffix = "\n";
		if (rFlag)
			print_group_info(rUID, suffix);
		else
			print_group_info(eUID, suffix);
		exit(0);
	}

	if (glFlag) {
	// group list
		if (rFlag)
			print_group_list(rUID);
		else
			print_group_list(eUID);
		exit(0);
	}
	
	if (uFlag) {
	// user information
		suffix = "\n";
		if (rFlag)
			print_user_info(rUID, suffix);
		else
			print_user_info(eUID, suffix);
		exit(0);
	}

	// no arguments? print combined info.
	print_combined_info();

	return B_NO_ERROR;
}
