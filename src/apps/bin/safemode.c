/*
 * (c) 2004, Jérôme DUVAL for Haiku
 * released under the MIT licence.
 *
 * safemode
 */


#include <syscalls.h>

#include <stdio.h>
#include <strings.h>


int
main(int argc, char **argv)
{
	char buffer[B_FILE_NAME_LENGTH];
	size_t size = sizeof(buffer);

	status_t status = _kern_get_safemode_option("safemode", buffer, &size);
	if (status == B_OK) {
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

