/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_COMMAND_BEOS_H
#define FS_SHELL_COMMAND_BEOS_H

static const char *kFSShellCommandPort = "fs shell command port";

struct external_command_message {
	port_id		reply_port;
	char		command[20480];
// TODO: Increase command size and transmit only as much as necessary.
};

struct external_command_reply {
	int			error;
};

#endif	// FS_SHELL_COMMAND_BEOS_H
