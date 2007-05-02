/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_EXTERNAL_COMMANDS_H
#define _FSSH_EXTERNAL_COMMANDS_H


namespace FSShell {


bool	get_external_command(char* input, int len);
void	reply_to_external_command(int result);
void	external_command_cleanup();


}	// namespace FSShell


#endif	// _FSSH_EXTERNAL_COMMANDS_H
