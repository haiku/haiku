/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "external_commands.h"

#ifdef __BEOS__

#include <stdio.h>
#include <string.h>

#include <OS.h>

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


#else // ! __BEOS__


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "fs_shell_command_unix.h"


static int sClientConnection = -1;

static int
get_command_socket()
{
	static int fd = -1;
	static bool initialized = false;
	if (!initialized) {
		// get the listener socket
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
			return -1;
	
		// bind it to the port
		sockaddr_un addr;
		unlink(kFSShellCommandSocketAddress);
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, kFSShellCommandSocketAddress);
		int addrLen = addr.sun_path + strlen(addr.sun_path) + 1 - (char*)&addr;
		if (bind(fd, (sockaddr*)&addr, addrLen) < 0) {
			close(fd);
			return -1;
		}
		
		// start listening
		if (listen(fd, 1) < 0) {
			close(fd);
			return -1;
		}
	}

	return fd;
}


static int
get_client_connection()
{
	if (sClientConnection >= 0)
		return sClientConnection;

	// get the listener socket
	int commandFD = get_command_socket();
	if (commandFD < 0)
		return -1;

	// accept a connection
	do {
		sockaddr_un addr;
		socklen_t addrLen = sizeof(addr);
		sClientConnection = accept(commandFD, (sockaddr*)&addr, &addrLen);
	} while (sClientConnection < 0 && errno == EINTR);

	return sClientConnection;
}


static void
close_client_connection()
{
	if (sClientConnection >= 0) {
		close(sClientConnection);
		sClientConnection = -1;
	}
}


char *
get_external_command(const char *prompt, char *input, int len)
{
	do {
		// get a connection
		int connection = get_client_connection();
		if (connection < 0)
			return NULL;

		// read until we have a full command
		external_command_message message;
		int toRead = sizeof(message);
		char *buffer = (char*)&message;
		while (toRead > 0) {
			int bytesRead = read(connection, buffer, toRead);
			if (bytesRead < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					fprintf(stderr, "Reading from connection failed: %s\n", strerror(errno));
					break;
				}
			}

			// connection closed?
			if (bytesRead == 0)
				break;
			
			buffer += bytesRead;
			toRead -= bytesRead;
		}

		// connection may be broken: discard it
		if (toRead > 0) {
			close_client_connection();
			continue;
		}

		// get the len of the command
		int commandLen = strnlen(message.command, sizeof(message.command)) + 1;
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
		return input;
	
	} while (true);
}


void
reply_to_external_command(int result)
{
	if (sClientConnection < 0)
		return;

	// prepare the reply
	external_command_reply reply;
	reply.error = result;

	// send the reply
	int toWrite = sizeof(reply);
	char *replyBuffer = (char*)&reply;
	ssize_t bytesWritten;
	do {
		bytesWritten = write(sClientConnection, replyBuffer, toWrite);
		if (bytesWritten > 0) {
			replyBuffer += bytesWritten;
			toWrite -= bytesWritten;
		}
	} while (toWrite > 0 && !(bytesWritten < 0 && errno != EINTR));

	// connection may be broken: discard it
	if (bytesWritten < 0)
		close_client_connection();
}

#endif // ! __BEOS__
