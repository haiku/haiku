/*
 * (c) 2004, Jérôme DUVAL for Haiku
 * released under the MIT licence.
 */


#include <drivers/driver_settings.h>
#include <syscalls.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>


int
main(int argc, char **argv)
{
	const char *optionName = B_SAFEMODE_SAFE_MODE;
	bool realString = false;
	char buffer[B_FILE_NAME_LENGTH];
	size_t size = sizeof(buffer);
	status_t status;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			printf("Usage: %s [-s] [optionName]\n"
				"Prints the value of a kernel safemode option, "
				"(default is safemode boot itself).\n\n"
				"  -s\t\tReturn the real string result, not a boolean.\n",
				argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-s"))
			realString = true;
		else
			optionName = argv[i];
	}

	status = _kern_get_safemode_option(optionName, buffer, &size);
	if (status == B_OK) {
		if (realString) {
			puts(buffer);
			return 0;
		}
		if (!strncasecmp(buffer, "true", size)
			|| !strncasecmp(buffer, "yes", size)
			|| !strncasecmp(buffer, "on", size)
			|| !strncasecmp(buffer, "enabled", size)) {
			puts("yes");
			return 1;
		}
	}

	puts("no");
	return 0;
}

