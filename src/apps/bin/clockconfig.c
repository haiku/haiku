/* 
 * Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
 * Distributed under the terms of the MIT License.
 */


#include <FindDirectory.h>
#include <stdio.h>
#include <string.h>
#include <OS.h>


#ifdef COMPILE_FOR_R5
void _kset_tzfilename_(const char *name, size_t length, bool isGMT);
void _kset_tzspecs_(uint32 timezone_offset, bool dst_observed);

#	define _kern_set_tzfilename _kset_tzfilename_
#	define _kern_set_timezone _kset_tzspecs_
#else
#	include <syscalls.h>
#endif


int
main(int argc, char **argv)
{
	char path[B_PATH_NAME_LENGTH];
	char link[B_PATH_NAME_LENGTH];
	FILE *file;
	bool isGMT = false;
	struct tm* tm = NULL;
	time_t t;

	printf("Looking for RTC settings file\n");
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, B_PATH_NAME_LENGTH) != B_OK) {
		fprintf(stderr, "%s: can't find settings directory\n", argv[0]);
		exit(1);
	}

	strcat(path, "/RTC_time_settings");
	file = fopen(path, "r");	
	if (file != NULL) {
		char string[10];
		fscanf(file, "%s", string);
		isGMT = (strncmp(string, "local", 5) != 0);
		fclose(file);
	} else {
		fprintf(stderr, "%s: can't read RTC settings\n", argv[0]);
		printf("%s: No knowledge about contents of RTC\n", argv[0]);

		// we don't care if not RTC settings is found, default is local
	}

	printf("%s: RTC stores %s time.\n", argv[0], isGMT ? "GMT" : "local" );

	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, B_PATH_NAME_LENGTH) != B_OK) {
		fprintf(stderr, "can't find settings directory\n");
		return 1;
	}

	strcat(path, "/timezone");

	printf("Looking for %s file\n", path);
	if (readlink(path, link, B_PATH_NAME_LENGTH) > 0) {
		printf("%s: Setting timezone to '%s'\n", argv[0], link);

		_kern_set_tzfilename(link, strlen(link), isGMT);

		tzset();

		time(&t);
		tm = localtime(&t);
		_kern_set_timezone(tm->tm_gmtoff, tm->tm_isdst);		
	} else {
		fprintf(stderr, "%s: can't read link for timezone\n", argv[0]);
		printf("%s: No timezone setting.\n", argv[0]);
	}

	return 0;	
}
