/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fs_shell_command_unix.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "fs_shell_command.h"


static bool
write_data(int fd, const void* _buffer, size_t toWrite)
{
	const char* buffer = (const char*)_buffer;

	ssize_t bytesWritten;
	do {
		bytesWritten = write(fd, buffer, toWrite);
		if (bytesWritten > 0) {
			buffer += bytesWritten;
			toWrite -= bytesWritten;
		}
	} while (toWrite > 0 && !(bytesWritten < 0 && errno != EINTR));

	return (bytesWritten >= 0);
}


bool
send_external_command(const char *command, int *result)
{
	external_command_message message;
	message.command_length = strlen(command);

	// create a socket
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open unix socket: %s\n",
			strerror(errno));
		return false;
	}

	// connect to the fs_shell
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, kFSShellCommandSocketAddress);
	int addrLen = addr.sun_path + strlen(addr.sun_path) + 1 - (char*)&addr;
	if (connect(fd, (sockaddr*)&addr, addrLen) < 0) {
		fprintf(stderr, "Error: Failed to open connection to FS shell: %s\n",
			strerror(errno));
		close(fd);
		return false;
	}
	
	// send the command message and the command
	if (!write_data(fd, &message, sizeof(message))
		|| !write_data(fd, command, message.command_length)) {
		fprintf(stderr, "Error: Writing to fs_shell failed: %s\n",
			strerror(errno));
		close(fd);
		return false;
	}

	// read the reply	
	external_command_reply reply;
	int toRead = sizeof(reply);
	char *replyBuffer = (char*)&reply;
	while (toRead > 0) {
		int bytesRead = read(fd, replyBuffer, toRead);
		if (bytesRead < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "Error: Failed to read reply from fs_shell: "
					"%s\n", strerror(errno));
				close(fd);
				return false;
			}
		}

		if (bytesRead == 0) {
			fprintf(stderr, "Error: Unexpected end of fs_shell reply. Was "
				"still expecting %d bytes\n", toRead);
			return false;
		}
		
		replyBuffer += bytesRead;
		toRead -= bytesRead;
	}

	*result = reply.error;
	return true;
}

