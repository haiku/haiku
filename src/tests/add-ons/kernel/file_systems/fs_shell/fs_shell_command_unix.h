/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_COMMAND_UNIX_H
#define FS_SHELL_COMMAND_UNIX_H

static const char *kFSShellCommandSocketAddress = "/tmp/fs_shell_commands";

struct external_command_message {
	char		command[20480];
};

struct external_command_reply {
	int			error;
};

#endif	// FS_SHELL_COMMAND_UNIX_H
