/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fs_shell_command_beos.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "fs_shell_command.h"


bool gUsesFifos = false;


bool
send_external_command(const char *command, int *result)
{
	int commandLen = strlen(command);
	if (commandLen > kMaxCommandLength) {
		fprintf(stderr, "Error: Command line too long.\n");
		return false;
	}

	char _message[sizeof(external_command_message) + kMaxCommandLength];
	external_command_message* message = (external_command_message*)_message;
	strcpy(message->command, command);

	// find the command port
	port_id commandPort = find_port(kFSShellCommandPort);
	if (commandPort < 0) {
		fprintf(stderr, "Error: Couldn't find fs_shell command port.\n");
		return false;
	}

	// create a reply port
	port_id replyPort = create_port(1, "fs shell reply port");
	if (replyPort < 0) {
		fprintf(stderr, "Error: Failed to create a reply port: %s\n",
			strerror(replyPort));
		return false;
	}
	message->reply_port = replyPort;

	// send the command message
	status_t error;
	do {
		error = write_port(commandPort, 0, message,
			sizeof(external_command_message) + commandLen);
	} while (error == B_INTERRUPTED);

	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to send command: %s\n", strerror(error));
		return false;
	}

	// wait for the reply
	external_command_reply reply;
	ssize_t bytesRead;
	do {
		int32 code;
		bytesRead = read_port(replyPort, &code, &reply, sizeof(reply));
	} while (bytesRead == B_INTERRUPTED);

	if (bytesRead < 0) {
		fprintf(stderr, "Error: Failed to read reply from fs_shell: %s\n",
			strerror(bytesRead));
		return false;
	}

	*result = reply.error;
	return true;
}

