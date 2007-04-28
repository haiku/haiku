/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef _FSSH_FS_SHELL_COMMAND_UNIX_H
#define _FSSH_FS_SHELL_COMMAND_UNIX_H


static const char *kFSShellCommandSocketAddress = "/tmp/fs_shell_commands";

struct external_command_message {
	unsigned	command_length;
};

struct external_command_reply {
	int			error;
};


#endif	// _FSSH_FS_SHELL_COMMAND_UNIX_H
