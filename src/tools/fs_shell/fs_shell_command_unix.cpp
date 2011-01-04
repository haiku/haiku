/*
 * Copyright 2005-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_shell_command.h"


bool gUsesFifos = true;


bool
send_external_command(const char* command, int* result)
{
	// open the pipe to the FS shell
	FILE* out = fdopen(4, "w");
	if (out == NULL) {
		fprintf(stderr, "Error: Failed to open command output: %s\n",
			strerror(errno));
		return false;
	}

	// open the pipe from the FS shell
	FILE* in = fdopen(3, "r");
	if (in == NULL) {
		fprintf(stderr, "Error: Failed to open command reply input: %s\n",
			strerror(errno));
		return false;
	}

	// write the command
	if (fputs(command, out) == EOF || fputc('\n', out) == EOF
		|| fflush(out) == EOF) {
		fprintf(stderr, "Error: Failed to write command to FS shell: %s\n",
			strerror(errno));
		return false;
	}

	// read the reply
	char buffer[16];
	if (fgets(buffer, sizeof(buffer), in) == NULL) {
		fprintf(stderr, "Error: Failed to get command reply: %s\n",
			strerror(errno));
		return false;
	}

	// parse the number
	char* end;
	*result = strtol(buffer, &end, 10);
	if (end == buffer) {
		fprintf(stderr, "Error: Read non-number command reply from FS shell: "
			"\"%s\"\n", buffer);
		return false;
	}

	return true;
}

