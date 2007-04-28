/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "external_commands.h"
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
	
		initialized = true;
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


static bool
read_data(int fd, void* _buffer, size_t toRead)
{
	char* buffer = (char*)_buffer;

	ssize_t bytesRead = 0;
	while (toRead > 0 && !(bytesRead < 0 && errno != EINTR)) {
		bytesRead = read(fd, buffer, toRead);
		if (bytesRead == 0)
			break;
		if (bytesRead > 0) {
			buffer += bytesRead;
			toRead -= bytesRead;
		}
	}

	return (toRead == 0);
}


bool
FSShell::get_external_command(char *input, int len)
{
	do {
		// get a connection
		int connection = get_client_connection();
		if (connection < 0)
			return false;

		// read command message
		external_command_message message;
		if (!read_data(connection, &message, sizeof(message))) {
			// that usually means the connection was closed
			close_client_connection();
			continue;
		}

		// check command length
		if (message.command_length >= (unsigned)len) {
			fprintf(stderr, "Error: Command too long!\n");
			close_client_connection();
			continue;
		}

		// read the command
		if (!read_data(connection, input, message.command_length)) {
			fprintf(stderr, "Error: Reading from connection failed: "
				"%s\n", strerror(errno));
			close_client_connection();
			continue;
		}

		// null-terminate
		input[message.command_length] = '\0';

		return true;
	
	} while (true);
}


void
FSShell::reply_to_external_command(int result)
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


void
FSShell::external_command_cleanup()
{
	unlink(kFSShellCommandSocketAddress);
}
