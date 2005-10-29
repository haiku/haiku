/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_shell_command.h"

static void
add_char(char *&buffer, int &bufferSize, char c)
{
	if (bufferSize <= 0) {
		fprintf(stderr, "Command line too long\n");
		exit(1);
	}

	*buffer = c;
	buffer++;
	bufferSize--;
}


static void
prepare_command_string(const char *const *argv, int argc, char *buffer,
		int bufferSize)
{
	for (int argi = 0; argi < argc; argi++) {
		const char *arg = argv[argi];

		if (argi > 0)
			add_char(buffer, bufferSize, ' ');

		while (*arg) {
			if (strchr(" ", *arg))
				add_char(buffer, bufferSize, '\\');
			add_char(buffer, bufferSize, *arg);
			arg++;
		}
	}

	add_char(buffer, bufferSize, '\0');
}


int
main(int argc, const char *const *argv)
{
	if (argc < 2) {
		fprintf(stderr, "No command given.\n");
		exit(1);
	}

	// prepare the command string
	char command[20480];
	prepare_command_string(argv + 1, argc - 1, command, sizeof(command));

	// send the command
	int result;
	int error = send_external_command(command, &result);
	if (error != 0)
		exit(1);

	// evaluate result
	if (result != 0) {
		fprintf(stderr, "Command failed: %s\n", strerror(result));
		fprintf(stderr, "Command was:\n  %s\n", command);
		exit(1);
	}

	return 0;
}

