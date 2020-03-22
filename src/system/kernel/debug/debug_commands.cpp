/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "debug_commands.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <KernelExport.h>

#include <debug.h>
#include <debug_heap.h>
#include <lock.h>
#include <thread.h>
#include <util/AutoLock.h>

#include "debug_output_filter.h"
#include "debug_variables.h"


#define INVOKE_COMMAND_FAULT	1
#define INVOKE_COMMAND_ERROR	2


struct invoke_command_parameters {
	debugger_command*	command;
	int					argc;
	char**				argv;
	int					result;
};


static const int32 kMaxInvokeCommandDepth = 5;
static const int32 kOutputBufferSize = 1024;


bool gInvokeCommandDirectly = false;

static spinlock sSpinlock = B_SPINLOCK_INITIALIZER;

static struct debugger_command *sCommands;

static jmp_buf sInvokeCommandEnv[kMaxInvokeCommandDepth];
static int32 sInvokeCommandLevel = 0;
static bool sInCommand = false;
static char sOutputBuffers[MAX_DEBUGGER_COMMAND_PIPE_LENGTH][kOutputBufferSize];
static debugger_command_pipe* sCurrentPipe;
static int32 sCurrentPipeSegment;


static int invoke_pipe_segment(debugger_command_pipe* pipe, int32 index,
	char* argument);


class PipeDebugOutputFilter : public DebugOutputFilter {
public:
	PipeDebugOutputFilter()
	{
	}

	PipeDebugOutputFilter(debugger_command_pipe* pipe, int32 segment,
			char* buffer, size_t bufferSize)
		:
		fPipe(pipe),
		fSegment(segment),
		fBuffer(buffer),
		fBufferCapacity(bufferSize - 1),
		fBufferSize(0)
	{
	}

	virtual void PrintString(const char* string)
	{
		if (fPipe->broken)
			return;

		size_t size = strlen(string);
		while (const char* newLine = strchr(string, '\n')) {
			size_t length = newLine - string;
			_Append(string, length);

			// invoke command
			fBuffer[fBufferSize] = '\0';
			invoke_pipe_segment(fPipe, fSegment + 1, fBuffer);

			fBufferSize = 0;

			string = newLine + 1;
			size -= length + 1;
		}

		_Append(string, size);

		if (fBufferSize == fBufferCapacity) {
			// buffer is full, but contains no newline -- execute anyway
			invoke_pipe_segment(fPipe, fSegment + 1, fBuffer);
			fBufferSize = 0;
		}
	}

	virtual void Print(const char* format, va_list args)
	{
		if (fPipe->broken)
			return;

		// print directly into the buffer
		if (fBufferSize < fBufferCapacity) {
			size_t totalBytes = vsnprintf(fBuffer + fBufferSize,
				fBufferCapacity - fBufferSize, format, args);
			fBufferSize += std::min(totalBytes,
				fBufferCapacity - fBufferSize - 1);
		}

		// execute every complete line
		fBuffer[fBufferSize] = '\0';
		char* line = fBuffer;

		while (char* newLine = strchr(line, '\n')) {
			// invoke command
			*newLine = '\0';
			invoke_pipe_segment(fPipe, fSegment + 1, line);

			line = newLine + 1;
		}

		size_t left = fBuffer + fBufferSize - line;

		if (left == fBufferCapacity) {
			// buffer is full, but contains no newline -- execute anyway
			invoke_pipe_segment(fPipe, fSegment + 1, fBuffer);
			left = 0;
		}

		if (left > 0)
			memmove(fBuffer, line, left);

		fBufferSize = left;
	}

private:
	void _Append(const char* string, size_t length)
	{
		size_t toAppend = min_c(length, fBufferCapacity - fBufferSize);
		memcpy(fBuffer + fBufferSize, string, toAppend);
		fBufferSize += length;
	}

private:
	debugger_command_pipe*	fPipe;
	int32					fSegment;
	char*					fBuffer;
	size_t					fBufferCapacity;
	size_t					fBufferSize;
};


static PipeDebugOutputFilter sPipeOutputFilters[
	MAX_DEBUGGER_COMMAND_PIPE_LENGTH - 1];


static void
invoke_command_trampoline(void* _parameters)
{
	invoke_command_parameters* parameters
		= (invoke_command_parameters*)_parameters;
	parameters->result = parameters->command->func(parameters->argc,
		parameters->argv);
}


static int
invoke_pipe_segment(debugger_command_pipe* pipe, int32 index, char* argument)
{
	// set debug output
	DebugOutputFilter* oldFilter = set_debug_output_filter(
		index == pipe->segment_count - 1
			? &gDefaultDebugOutputFilter
			: (DebugOutputFilter*)&sPipeOutputFilters[index]);

	// set last command argument
	debugger_command_pipe_segment& segment = pipe->segments[index];
	if (index > 0)
		segment.argv[segment.argc - 1] = argument;

	// invoke command
	int32 oldIndex = sCurrentPipeSegment;
	sCurrentPipeSegment = index;

	int result = invoke_debugger_command(segment.command, segment.argc,
		segment.argv);
	segment.invocations++;

	sCurrentPipeSegment = oldIndex;

	// reset debug output
	set_debug_output_filter(oldFilter);

	if (result == B_KDEBUG_ERROR) {
		pipe->broken = true;

		// Abort the previous pipe segment execution. The complete pipe is
		// aborted iteratively this way.
		if (index > 0)
			abort_debugger_command();
	}

	return result;
}


debugger_command*
next_debugger_command(debugger_command* command, const char* prefix,
	int prefixLen)
{
	if (command == NULL)
		command = sCommands;
	else
		command = command->next;

	while (command != NULL && strncmp(prefix, command->name, prefixLen) != 0)
		command = command->next;

	return command;
}


debugger_command *
find_debugger_command(const char *name, bool partialMatch, bool& ambiguous)
{
	debugger_command *command;

	ambiguous = false;

	// search command by full name

	for (command = sCommands; command != NULL; command = command->next) {
		if (strcmp(name, command->name) == 0)
			return command;
	}

	// if it couldn't be found, search for a partial match

	if (partialMatch) {
		int length = strlen(name);
		command = next_debugger_command(NULL, name, length);
		if (command != NULL) {
			if (next_debugger_command(command, name, length) == NULL)
				return command;

			ambiguous = true;
		}
	}

	return NULL;
}


/*!	Returns whether or not a debugger command is currently being invoked.
*/
bool
in_command_invocation(void)
{
	return sInCommand;
}


/*!	This function is a safe gate through which debugger commands are invoked.
	It sets a fault handler before invoking the command, so that an invalid
	memory access will not result in another KDL session on top of this one
	(and "cont" not to work anymore). We use setjmp() + longjmp() to "unwind"
	the stack after catching a fault.
 */
int
invoke_debugger_command(struct debugger_command *command, int argc, char** argv)
{
	// intercept invocations with "--help" and directly print the usage text
	// If we know the command's usage text, intercept "--help" invocations
	// and print it directly.
	if (argc == 2 && argv[1] != NULL && strcmp(argv[1], "--help") == 0
			&& command->usage != NULL) {
		kprintf_unfiltered("usage: %s ", command->name);
		kputs_unfiltered(command->usage);
		return 0;
	}

	// replace argv[0] with the actual command name
	argv[0] = (char *)command->name;

	DebugAllocPoolScope allocPoolScope;
		// Will automatically clean up all allocations the command leaves over.

	// Invoking the command directly might be useful when debugging debugger
	// commands.
	if (gInvokeCommandDirectly)
		return command->func(argc, argv);

	sInCommand = true;

	invoke_command_parameters parameters;
	parameters.command = command;
	parameters.argc = argc;
	parameters.argv = argv;

	switch (debug_call_with_fault_handler(
			sInvokeCommandEnv[sInvokeCommandLevel++],
			&invoke_command_trampoline, &parameters)) {
		case 0:
			sInvokeCommandLevel--;
			sInCommand = false;
			return parameters.result;

		case INVOKE_COMMAND_FAULT:
		{
			debug_page_fault_info* info = debug_get_page_fault_info();
			if ((info->flags & DEBUG_PAGE_FAULT_NO_INFO) == 0) {
				kprintf_unfiltered("\n[*** %s FAULT at %#lx, pc: %#lx ***]\n",
					(info->flags & DEBUG_PAGE_FAULT_NO_INFO) != 0
						? "WRITE" : "READ",
					info->fault_address, info->pc);
			} else {
				kprintf_unfiltered("\n[*** READ/WRITE FAULT (?), "
					"pc: %#lx ***]\n", info->pc);
			}
			break;
		}
		case INVOKE_COMMAND_ERROR:
			// command aborted (no page fault)
			break;
	}

	sInCommand = false;
	return B_KDEBUG_ERROR;
}


/*!	Aborts the currently executed debugger command (in fact the complete pipe),
	unless direct command invocation has been set. If successful, the function
	won't return.
*/
void
abort_debugger_command()
{
	if (!gInvokeCommandDirectly && sInvokeCommandLevel > 0) {
		longjmp(sInvokeCommandEnv[--sInvokeCommandLevel],
			INVOKE_COMMAND_ERROR);
	}
}


int
invoke_debugger_command_pipe(debugger_command_pipe* pipe)
{
	debugger_command_pipe* oldPipe = sCurrentPipe;
	sCurrentPipe = pipe;

	// prepare outputs
	// TODO: If a pipe is invoked in a pipe, outputs will clash.
	int32 segments = pipe->segment_count;
	for (int32 i = 0; i < segments - 1; i++) {
		new(&sPipeOutputFilters[i]) PipeDebugOutputFilter(pipe, i,
			sOutputBuffers[i], kOutputBufferSize);
	}

	int result;
	while (true) {
		result = invoke_pipe_segment(pipe, 0, NULL);

		// perform final rerun for all commands that want it
		for (int32 i = 1; result != B_KDEBUG_ERROR && i < segments; i++) {
			debugger_command_pipe_segment& segment = pipe->segments[i];
			if ((segment.command->flags & B_KDEBUG_PIPE_FINAL_RERUN) != 0) {
				result = invoke_pipe_segment(pipe, i, NULL);
				if (result == B_KDEBUG_RESTART_PIPE) {
					for (int32 j = 0; j < i; j++)
						pipe->segments[j].invocations = 0;
					break;
				}
			}
		}

		if (result != B_KDEBUG_RESTART_PIPE)
			break;
	}

	sCurrentPipe = oldPipe;

	return result;
}


debugger_command_pipe*
get_current_debugger_command_pipe()
{
	return sCurrentPipe;
}


debugger_command_pipe_segment*
get_current_debugger_command_pipe_segment()
{
	return sCurrentPipe != NULL
		? &sCurrentPipe->segments[sCurrentPipeSegment] : NULL;
}


debugger_command*
get_debugger_commands()
{
	return sCommands;
}


void
sort_debugger_commands()
{
	// bubble sort the commands
	debugger_command* stopCommand = NULL;
	while (stopCommand != sCommands) {
		debugger_command** command = &sCommands;
		while (true) {
			debugger_command* nextCommand = (*command)->next;
			if (nextCommand == stopCommand) {
				stopCommand = *command;
				break;
			}

			if (strcmp((*command)->name, nextCommand->name) > 0) {
				(*command)->next = nextCommand->next;
				nextCommand->next = *command;
				*command = nextCommand;
			}

			command = &(*command)->next;
		}
	}
}


status_t
add_debugger_command_etc(const char* name, debugger_command_hook func,
	const char* description, const char* usage, uint32 flags)
{
	bool ambiguous;
	debugger_command *cmd = find_debugger_command(name, false, ambiguous);
	if (cmd != NULL && ambiguous == false)
		return B_NAME_IN_USE;

	cmd = (debugger_command*)malloc(sizeof(debugger_command));
	if (cmd == NULL)
		return B_NO_MEMORY;

	cmd->func = func;
	cmd->name = name;
	cmd->description = description;
	cmd->usage = usage;
	cmd->flags = flags;

	InterruptsSpinLocker _(sSpinlock);

	cmd->next = sCommands;
	sCommands = cmd;

	return B_OK;
}


status_t
add_debugger_command_alias(const char* newName, const char* oldName,
	const char* description)
{
	// get the old command
	bool ambiguous;
	debugger_command* command = find_debugger_command(oldName, false,
		ambiguous);
	if (command == NULL)
		return B_NAME_NOT_FOUND;

	// register new command
	return add_debugger_command_etc(newName, command->func,
		description != NULL ? description : command->description,
		command->usage, command->flags);
}


bool
print_debugger_command_usage(const char* commandName)
{
	// get the command
	bool ambiguous;
	debugger_command* command = find_debugger_command(commandName, true,
		ambiguous);
	if (command == NULL)
		return false;

	// directly print the usage text, if we know it, otherwise invoke the
	// command with "--help"
	if (command->usage != NULL) {
		kprintf_unfiltered("usage: %s ", command->name);
		kputs_unfiltered(command->usage);
	} else {
		const char* args[3] = { NULL, "--help", NULL };
		invoke_debugger_command(command, 2, (char**)args);
	}

	return true;
}


bool
has_debugger_command(const char* commandName)
{
	bool ambiguous;
	return find_debugger_command(commandName, false, ambiguous) != NULL;
}


//	#pragma mark - public API

int
add_debugger_command(const char *name, int (*func)(int, char **),
	const char *desc)
{
	return add_debugger_command_etc(name, func, desc, NULL, 0);
}


int
remove_debugger_command(const char * name, int (*func)(int, char **))
{
	struct debugger_command *cmd = sCommands;
	struct debugger_command *prev = NULL;
	cpu_status state;

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	while (cmd) {
		if (!strcmp(cmd->name, name) && cmd->func == func)
			break;

		prev = cmd;
		cmd = cmd->next;
	}

	if (cmd) {
		if (cmd == sCommands)
			sCommands = cmd->next;
		else
			prev->next = cmd->next;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);

	if (cmd) {
		free(cmd);
		return B_NO_ERROR;
	}

	return B_NAME_NOT_FOUND;
}

