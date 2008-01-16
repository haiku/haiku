/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_COMMANDS_H
#define _KERNEL_DEBUG_COMMANDS_H


#include <SupportDefs.h>


struct debugger_command {
	struct debugger_command *next;
	int (*func)(int, char **);
	const char *name;
	const char *description;
};

#ifdef __cplusplus
extern "C" {
#endif

debugger_command* next_debugger_command(debugger_command* command,
	const char* prefix, int prefixLen);
debugger_command* find_debugger_command(const char* name, bool partialMatch,
	bool& ambiguous);
int invoke_debugger_command(struct debugger_command *command, int argc,
	char** argv);

debugger_command* get_debugger_commands();
void sort_debugger_commands();

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_DEBUG_COMMANDS_H
