/*
 * Copyright 2002-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Fleet
 *		Vasilis Kaoutsis
 */


#include <Application.h>
#include <AppFileInfo.h>

#include <stdio.h>
#include <string.h>


void
usage()
{
	printf("usage: version [OPTION] FILENAME [FILENAME2, ...]\n"
		"Returns the version of a file.\n\n"
		"        -h, --help              this usage message\n"
		"        -l, --long              print long version information of FILENAME\n"
		"        -n, --numerical         print in numerical mode\n"
		"                                (Major miDdle miNor Variety Internal)\n"
		"        -s, --system            print system version instead of app version\n"
		"        --version               print version information for this command\n");
}


status_t
get_version(const char *filename, version_kind kind, bool longFlag, bool numericalFlag)
{
	BFile file(filename, O_RDONLY);
	if (file.InitCheck() != B_OK) {
		printf("Couldn't get file info!\n");
		return B_FILE_ERROR;
	}

	BAppFileInfo info(&file);
	if (info.InitCheck() != B_OK) {
		printf("Couldn't get file info!\n");
		return B_FILE_ERROR;
	}

	version_info version;
	if (info.GetVersionInfo(&version, kind) != B_OK) {
		printf("Version unknown!\n");
		return B_ERROR;
	}

	if (longFlag) {
		printf("%s\n", version.long_info);
		return B_OK;
	}

	if (numericalFlag) {
		printf("%" B_PRIu32 " ", version.major);
		printf("%" B_PRIu32 " ", version.middle);
		printf("%" B_PRIu32 " ", version.minor);

		switch (version.variety) {
			case B_DEVELOPMENT_VERSION:
				printf("d ");
				break;

			case B_ALPHA_VERSION:
				printf("a ");
				break;

			case B_BETA_VERSION:
				printf("b ");
				break;

			case B_GAMMA_VERSION:
				printf("g ");
				break;

			case B_GOLDEN_MASTER_VERSION:
				printf("gm ");
				break;

			case B_FINAL_VERSION:
				printf("f ");
				break;	
		}

		printf("%" B_PRIu32 "\n", version.internal);
		return B_OK;
	}

	printf("%s\n", version.short_info);
	return B_OK;
}


/*!
	determines whether  \a string1 contains at least one or more of the characters
	of \a string2 but none of which \a string2 doesn't contain. 

	examples:
	true == ("hel" == "help"); true == ("help" == "help"); true == ("h" == "help");
	false == ("held" == "help"); false == ("helped" == "help"); 
*/
bool
str_less_equal(const char *str1, const char *str2)
{
	char *ptr1 = const_cast<char*>(str1);
	char *ptr2 = const_cast<char*>(str2);

	while (*ptr1 != '\0') {	
		if (*ptr1 != *ptr2)
			return false;
		++ptr1;
		++ptr2;
	}
	return true;
}


int
main(int argc, char *argv[])
{
	BApplication app("application/x.vnd.Haiku-version");
	version_kind kind = B_APP_VERSION_KIND;
	bool longFlag = false;
	bool numericalFlag = false;
	int i;

	if (argc < 2) {
		usage();
		return 0;
	}
	
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			char *ptr = argv[i];
			++ptr;

			if (*ptr == '-') {
				bool lessEqual = false;
				++ptr;

				if (*ptr == 'h') {
					lessEqual = str_less_equal(ptr, "help");
					if (lessEqual) {
						usage();
						return 0;
					}
				} else if (*ptr == 'l') {
					lessEqual = str_less_equal(ptr, "long");
					if (lessEqual)
						longFlag = true;
				} else if (*ptr == 'n') {
					lessEqual = str_less_equal(ptr, "numerical");
					if (lessEqual)
						numericalFlag = true;
				} else if (*ptr == 's') {
					lessEqual = str_less_equal(ptr, "system");
					if (lessEqual)
						kind = B_SYSTEM_VERSION_KIND;
				} else if (*ptr == 'v') {
					lessEqual = str_less_equal(ptr, "version");
					if (lessEqual) {
						get_version(argv[0], B_APP_VERSION_KIND, false, false);
						return 0;
					}
				}

				if (!lessEqual)
					printf("%s unrecognized option `%s'\n", argv[0], argv[i]);
			} else {
				while (*ptr != '\0') {
					if (*ptr == 'h') {
						usage();
						return 0;
					} else if (*ptr == 's')
						kind = B_SYSTEM_VERSION_KIND;
					else if (*ptr == 'l')
						longFlag = true;
					else if (*ptr == 'n')
						numericalFlag = true;
					else {
						printf("%s: invalid option -- %c\n", argv[0], *ptr);
						return 1;
					}

					if (argc < 3) {
						printf("%s: missing FILENAME(S) \n", argv[0]);
						return 1;
					}

					++ptr;
				}
			}
		}
	}

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] != '-'
			&& get_version(argv[i], kind, longFlag, numericalFlag) != B_OK)
			return 1;
	}

	return 0;
}
