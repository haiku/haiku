/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "fs_shell_command_beos.h"

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
			if (strchr("", *arg))
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
	external_command_message message;
	prepare_command_string(argv + 1, argc - 1, message.command,
		sizeof(message.command));

	// find the command port
	port_id commandPort = find_port(kFSShellCommandPort);
	if (commandPort < 0) {
		fprintf(stderr, "Couldn't find fs_shell command port.\n");
		exit(1);
	}

	// create a reply port
	port_id replyPort = create_port(1, "fs shell reply port");
	if (replyPort < 0) {
		fprintf(stderr, "Failed to create a reply port: %s\n",
			strerror(replyPort));
		exit(1);
	}
	message.reply_port = replyPort;

	// send the command message
	status_t error;
	do {
		error = write_port(commandPort, 0, &message, sizeof(message));
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "Failed to send command: %s\n", strerror(error));
		exit(1);
	}

	// wait for the reply
	external_command_reply reply;
	ssize_t bytesRead;
	do {
		int32 code;
		bytesRead = read_port(replyPort, &code, &reply, sizeof(reply));
	} while (bytesRead == B_INTERRUPTED);

	if (bytesRead < 0) {
		fprintf(stderr, "Failed to read reply from fs_shell: %s\n",
			strerror(bytesRead));
		exit(1);
	}

	// evaluate result
	if (reply.error != B_OK) {
		fprintf(stderr, "Command failed: %s\n", strerror(reply.error));
		fprintf(stderr, "Command was:\n  %s\n", message.command);
		exit(1);
	}

	return 0;
}

