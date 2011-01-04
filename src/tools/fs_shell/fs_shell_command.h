/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FS_SHELL_COMMAND_H
#define _FSSH_FS_SHELL_COMMAND_H


extern bool gUsesFifos;


bool	send_external_command(const char* command, int* result);


#endif	// _FSSH_FS_SHELL_COMMAND_H
