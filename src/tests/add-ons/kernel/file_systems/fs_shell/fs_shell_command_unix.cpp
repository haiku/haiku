/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "fs_shell_command.h"
#include "fs_shell_command_unix.h"

int
send_external_command(const char *command, int *result)
{
	external_command_message message;
	strncpy(message.command, command, sizeof(message.command));

	// create a socket
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return errno;

	// connect to the fs_shell
	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, kFSShellCommandSocketAddress);
	int addrLen = addr.sun_path + strlen(addr.sun_path) + 1 - (char*)&addr;
	if (connect(fd, (sockaddr*)&addr, addrLen) < 0) {
		close(fd);
		return errno;
	}
	
	// send the command message
	int toWrite = sizeof(message);
	const char *messageBuffer = (char*)&message;
	ssize_t bytesWritten;
	do {
		bytesWritten = write(fd, messageBuffer, toWrite);
		if (bytesWritten > 0) {
			messageBuffer += bytesWritten;
			toWrite -= bytesWritten;
		}
	} while (toWrite > 0 && !(bytesWritten < 0 && errno != EINTR));

	// close connection on error
	if (bytesWritten < 0) {
		fprintf(stderr, "Writing to fs_shell failed: %s\n", strerror(errno));
		close(fd);
		return errno;
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
				fprintf(stderr, "Failed to read reply from fs_shell: %s\n",
					strerror(errno));
				close(fd);
				return errno;
			}
		}

		if (bytesRead == 0) {
			fprintf(stderr, "Unexpected end of fs_shell reply. Was still "
				"expecting %d bytes\n", toRead);
			return EPIPE;
		}
		
		replyBuffer += bytesRead;
		toRead -= bytesRead;
	}

	*result = reply.error;
	return 0;
}

