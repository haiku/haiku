/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "fs_shell_command.h"
#include "fs_shell_command_beos.h"

int
send_external_command(const char *command, int *result)
{
	external_command_message message;
	strncpy(message.command, command, sizeof(message.command));

	// find the command port
	port_id commandPort = find_port(kFSShellCommandPort);
	if (commandPort < 0) {
		fprintf(stderr, "Couldn't find fs_shell command port.\n");
		return commandPort;
	}

	// create a reply port
	port_id replyPort = create_port(1, "fs shell reply port");
	if (replyPort < 0) {
		fprintf(stderr, "Failed to create a reply port: %s\n",
			strerror(replyPort));
		return replyPort;
	}
	message.reply_port = replyPort;

	// send the command message
	status_t error;
	do {
		error = write_port(commandPort, 0, &message, sizeof(message));
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "Failed to send command: %s\n", strerror(error));
		return error;
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
		return bytesRead;
	}

	*result = reply.error;
	return 0;
}

