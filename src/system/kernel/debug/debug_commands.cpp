/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "debug_commands.h"

#include <setjmp.h>
#include <string.h>

#include <KernelExport.h>

#include <debug.h>
#include <lock.h>
#include <thread.h>
#include <util/AutoLock.h>

#include "debug_variables.h"


static spinlock sSpinlock = 0;

static struct debugger_command *sCommands;

static jmp_buf sInvokeCommandEnv;
static bool sInvokeCommandDirectly = false;
static bool sInCommand = false;


debugger_command*
next_debugger_command(debugger_command* command, const char* prefix, int prefixLen)
{
	if (command == NULL)
		command = sCommands;
	else
		command = command->next;

	while (command != NULL && !strncmp(prefix, command->name, prefixLen) == 0)
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


/*!	Returns wether or not a debugger command is currently being invoked.
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
	if (argc == 2 && strcmp(argv[1], "--help") == 0 && command->usage != NULL) {
		kprintf("usage: %s ", command->name);
		kprintf(command->usage);
		return 0;
	}

	struct thread* thread = thread_get_current_thread();
	addr_t oldFaultHandler = thread->fault_handler;

	// replace argv[0] with the actual command name
	argv[0] = (char *)command->name;

	// Invoking the command directly might be useful when debugging debugger
	// commands.
	if (sInvokeCommandDirectly)
		return command->func(argc, argv);

	sInCommand = true;

	if (setjmp(sInvokeCommandEnv) == 0) {
		int result;
		thread->fault_handler = (addr_t)&&error;
		// Fake goto to trick the compiler not to optimize the code at the label
		// away.
		if (!thread)
			goto error;

		result = command->func(argc, argv);

		thread->fault_handler = oldFaultHandler;
		sInCommand = false;
		return result;

error:
		longjmp(sInvokeCommandEnv, 1);
			// jump into the else branch
	} else {
		kprintf("\n[*** READ/WRITE FAULT ***]\n");
	}

	thread->fault_handler = oldFaultHandler;
	sInCommand = false;
	return 0;
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
				debugger_command* tmpCommand = nextCommand->next;
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
	struct debugger_command *cmd;

	cmd = (struct debugger_command*)malloc(sizeof(struct debugger_command));
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
		kprintf("usage: %s ", command->name);
		kprintf(command->usage);
	} else {
		char* args[3] = { NULL, "--help", NULL };
		invoke_debugger_command(command, 2, args);
	}

	return true;
}


//	#pragma mark - public API


int
add_debugger_command(char *name, int (*func)(int, char **), char *desc)
{
	return add_debugger_command_etc(name, func, desc, NULL, 0);
}


int
remove_debugger_command(char * name, int (*func)(int, char **))
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

