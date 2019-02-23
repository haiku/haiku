/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_COMMANDS_H
#define _KERNEL_DEBUG_COMMANDS_H


#include <SupportDefs.h>


#define MAX_DEBUGGER_COMMAND_PIPE_LENGTH	8


typedef struct debugger_command {
	struct debugger_command* next;
	int				(*func)(int, char **);
	const char*		name;
	const char*		description;
	const char*		usage;
	uint32			flags;
} debugger_command;

typedef struct debugger_command_pipe_segment {
	int32				index;
	debugger_command*	command;
	int					argc;
	char**				argv;
	int32				invocations;
	uint64				user_data[4];	// can be used by the command
} debugger_command_pipe_segment;

typedef struct debugger_command_pipe {
	int32							segment_count;
	debugger_command_pipe_segment	segments[MAX_DEBUGGER_COMMAND_PIPE_LENGTH];
	bool							broken;
} debugger_command_pipe;

extern bool gInvokeCommandDirectly;

#ifdef __cplusplus
extern "C" {
#endif

debugger_command* next_debugger_command(debugger_command* command,
	const char* prefix, int prefixLen);
debugger_command* find_debugger_command(const char* name, bool partialMatch,
	bool& ambiguous);
bool in_command_invocation(void);

int invoke_debugger_command(struct debugger_command *command, int argc,
	char** argv);
void abort_debugger_command();

int invoke_debugger_command_pipe(debugger_command_pipe* pipe);
debugger_command_pipe* get_current_debugger_command_pipe();
debugger_command_pipe_segment* get_current_debugger_command_pipe_segment();

debugger_command* get_debugger_commands();
void sort_debugger_commands();

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_DEBUG_COMMANDS_H
