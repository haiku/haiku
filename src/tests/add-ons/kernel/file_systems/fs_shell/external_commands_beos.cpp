/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "external_commands.h"
#include "fs_shell_command_beos.h"

static port_id sReplyPort = -1;

static port_id
get_command_port()
{
	static port_id port = -1;
	static bool initialized = false;

	if (!initialized) {
		port = create_port(10, kFSShellCommandPort);
		initialized = true;
	}

	return port;
}


char *
get_external_command(const char *prompt, char *input, int len)
{
	// get/create the port
	port_id port = get_command_port();
	if (port < 0) {
		fprintf(stderr, "Failed to create command port: %s\n", strerror(port));
		return NULL;
	}


	while (true) {
		// read a message
		external_command_message message;
		ssize_t bytesRead;
		do {
			int32 code;
			bytesRead = read_port(port, &code, &message, sizeof(message));
		} while (bytesRead == B_INTERRUPTED);

		if (bytesRead < 0) {
			fprintf(stderr, "Reading from port failed: %s\n",
				strerror(bytesRead));
			return NULL;
		}

		// get the len of the command
		int commandLen = (char*)&message + bytesRead - message.command;
		if (commandLen <= 1) {
			fprintf(stderr, "No command given.\n");
			continue;
		}
		if (commandLen > len) {
			fprintf(stderr, "Command too long. Ignored.\n");
			continue;
		}

		// copy the command
		memcpy(input, message.command, commandLen);
		input[len - 1] = '\0';	// always NULL-terminate
		sReplyPort = message.reply_port;
		return input;
	}
}


void
reply_to_external_command(int result)
{
	if (sReplyPort >= 0) {
		// prepare the message
		external_command_reply reply;
		reply.error = result;

		// send the reply
		status_t error;
		do {
			error = write_port(sReplyPort, 0, &reply, sizeof(reply));
		} while (error == B_INTERRUPTED);
		sReplyPort = -1;

		if (error != B_OK) {
			fprintf(stderr, "Failed to send command result to reply port: %s\n",
				strerror(error));
		}
	}
}


void
external_command_cleanup()
{
	// The port will be deleted automatically when the team exits.
}
