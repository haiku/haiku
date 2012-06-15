/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "debug_builtin_commands.h"

#include <ctype.h>
#include <string.h>

#include <debug.h>
#include <kernel.h>

#include "debug_commands.h"
#include "gdb.h"


static int
cmd_reboot(int argc, char **argv)
{
	arch_cpu_shutdown(true);
	return 0;
		// I'll be really suprised if this line ever runs! ;-)
}


static int
cmd_shutdown(int argc, char **argv)
{
	arch_cpu_shutdown(false);
	return 0;
}


static int
cmd_help(int argc, char **argv)
{
	debugger_command *command, *specified = NULL;
	const char *start = NULL;
	int32 startLength = 0;
	bool ambiguous;

	if (argc > 1) {
		specified = find_debugger_command(argv[1], false, ambiguous);
		if (specified == NULL) {
			start = argv[1];
			startLength = strlen(start);
		}
	}

	if (specified != NULL) {
		// only print out the help of the specified command (and all of its aliases)
		kprintf("debugger command for \"%s\" and aliases:\n", specified->name);
	} else if (start != NULL)
		kprintf("debugger commands starting with \"%s\":\n", start);
	else
		kprintf("debugger commands:\n");

	for (command = get_debugger_commands(); command != NULL;
			command = command->next) {
		if (specified && command->func != specified->func)
			continue;
		if (start != NULL && strncmp(start, command->name, startLength))
			continue;

		kprintf(" %-20s\t\t%s\n", command->name, command->description ? command->description : "-");
	}

	return 0;
}


static int
cmd_continue(int argc, char **argv)
{
	return B_KDEBUG_QUIT;
}


static int
cmd_expr(int argc, char **argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 result;
	if (evaluate_debug_expression(argv[1], &result, false)) {
		kprintf("%llu (0x%llx)\n", result, result);
		set_debug_variable("_", result);
	}

	return 0;
}


static int
cmd_error(int argc, char **argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	int32 error = parse_expression(argv[1]);
	kprintf("error 0x%" B_PRIx32 ": %s\n", error, strerror(error));

	return 0;
}


static int
cmd_head(int argc, char** argv)
{
	debugger_command_pipe_segment* segment
		= get_current_debugger_command_pipe_segment();
	if (segment == NULL) {
		kprintf_unfiltered("%s can only be run as part of a pipe!\n", argv[0]);
		return B_KDEBUG_ERROR;
	}

	struct user_data {
		uint64	max_lines;
		uint64	lines;
	};
	user_data* userData = (user_data*)segment->user_data;

	if (segment->invocations == 0) {
		if (argc != 3) {
			print_debugger_command_usage(argv[0]);
			return B_KDEBUG_ERROR;
		}

		if (!evaluate_debug_expression(argv[1], &userData->max_lines, false))
			return B_KDEBUG_ERROR;
		userData->lines = 0;
	}

	if (++userData->lines <= userData->max_lines) {
		kputs(argv[2]);
		kputs("\n");
	}

	return 0;
}


static int
cmd_tail(int argc, char** argv)
{
	debugger_command_pipe_segment* segment
		= get_current_debugger_command_pipe_segment();
	if (segment == NULL) {
		kprintf_unfiltered("%s can only be run as part of a pipe!\n", argv[0]);
		return B_KDEBUG_ERROR;
	}

	struct user_data {
		uint64	max_lines;
		int64	line_count;
		bool	restarted;
	};
	user_data* userData = (user_data*)segment->user_data;

	if (segment->invocations == 0) {
		if (argc > 3) {
			print_debugger_command_usage(argv[0]);
			return B_KDEBUG_ERROR;
		}

		userData->max_lines = 10;
		if (argc > 2 && !evaluate_debug_expression(argv[1],
				&userData->max_lines, false)) {
			return B_KDEBUG_ERROR;
		}

		userData->line_count = 1;
		userData->restarted = false;
	} else if (!userData->restarted) {
		if (argv[argc - 1] == NULL) {
			userData->restarted = true;
			userData->line_count -= userData->max_lines;
			return B_KDEBUG_RESTART_PIPE;
		}

		++userData->line_count;
	} else {
		if (argv[argc - 1] == NULL)
			return 0;

		if (--userData->line_count < 0) {
			kputs(argv[argc - 1]);
			kputs("\n");
		}
	}

	return 0;
}


static int
cmd_grep(int argc, char** argv)
{
	bool caseSensitive = true;
	bool inverseMatch = false;

	int argi = 1;
	for (; argi < argc; argi++) {
		const char* arg = argv[argi];
		if (arg[0] != '-')
			break;

		for (int32 i = 1; arg[i] != '\0'; i++) {
			if (arg[i] == 'i') {
				caseSensitive = false;
			} else if (arg[i] == 'v') {
				inverseMatch = true;
			} else {
				print_debugger_command_usage(argv[0]);
				return B_KDEBUG_ERROR;
			}
		}
	}

	if (argc - argi != 2) {
		print_debugger_command_usage(argv[0]);
		return B_KDEBUG_ERROR;
	}

	const char* pattern = argv[argi++];
	const char* line = argv[argi++];

	bool match;
	if (caseSensitive) {
		match = strstr(line, pattern) != NULL;
	} else {
		match = false;
		int32 lineLen = strlen(line);
		int32 patternLen = strlen(pattern);
		for (int32 i = 0; i <= lineLen - patternLen; i++) {
			// This is rather slow, but should be OK for our purposes.
			if (strncasecmp(line + i, pattern, patternLen) == 0) {
				match = true;
				break;
			}
		}
	}

	if (match != inverseMatch) {
		kputs(line);
		kputs("\n");
	}

	return 0;
}


static int
cmd_wc(int argc, char** argv)
{
	debugger_command_pipe_segment* segment
		= get_current_debugger_command_pipe_segment();
	if (segment == NULL) {
		kprintf_unfiltered("%s can only be run as part of a pipe!\n", argv[0]);
		return B_KDEBUG_ERROR;
	}

	struct user_data {
		uint64	lines;
		uint64	words;
		uint64	chars;
	};
	user_data* userData = (user_data*)segment->user_data;

	if (segment->invocations == 0) {
		if (argc != 2) {
			print_debugger_command_usage(argv[0]);
			return B_KDEBUG_ERROR;
		}

		userData->lines = 0;
		userData->words = 0;
		userData->chars = 0;
	}

	const char* line = argv[1];
	if (line == NULL) {
		// last run -- print results
		kprintf("%10lld %10lld %10lld\n", userData->lines, userData->words,
			userData->chars);
		return 0;
	}

	userData->lines++;
	userData->chars++;
		// newline

	// count words and chars in this line
	bool inWord = false;
	for (; *line != '\0'; line++) {
		userData->chars++;
		if ((isspace(*line) != 0) == inWord) {
			inWord = !inWord;
			if (inWord)
				userData->words++;
		}
	}

	return 0;
}


static int
cmd_faults(int argc, char** argv)
{
	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return B_KDEBUG_ERROR;
	}

	if (argc == 2)
		gInvokeCommandDirectly = parse_expression(argv[1]) == 0;

	kprintf("Fault handling is %s%s.\n", argc == 2 ? "now " : "",
		gInvokeCommandDirectly ? "off" : "on");
	return 0;
}


// #pragma mark -


void
debug_builtin_commands_init()
{
	add_debugger_command_etc("help", &cmd_help, "List all debugger commands",
		"[name]\n"
		"Lists all debugger commands or those starting with \"name\".\n", 0);
	add_debugger_command_etc("reboot", &cmd_reboot, "Reboot the system",
		"\n"
		"Reboots the system.\n", 0);
	add_debugger_command_etc("shutdown", &cmd_shutdown, "Shut down the system",
		"\n"
		"Shuts down the system.\n", 0);
	add_debugger_command_etc("gdb", &cmd_gdb, "Connect to remote gdb",
		"\n"
		"Connects to a remote gdb connected to the serial port.\n", 0);
	add_debugger_command_etc("continue", &cmd_continue, "Leave kernel debugger",
		"\n"
		"Leaves kernel debugger.\n", 0);
	add_debugger_command_alias("exit", "continue", "Same as \"continue\"");
	add_debugger_command_alias("es", "continue", "Same as \"continue\"");
	add_debugger_command_etc("expr", &cmd_expr,
		"Evaluates the given expression and prints the result",
		"<expression>\n"
		"Evaluates the given expression and prints the result.\n",
		B_KDEBUG_DONT_PARSE_ARGUMENTS);
	add_debugger_command_etc("error", &cmd_error,
		"Prints a human-readable description for an error code",
		"<error>\n"
		"Prints a human-readable description for the given numeric error\n"
		"code.\n"
		"  <error>  - The numeric error code.\n", 0);
	add_debugger_command_etc("faults", &cmd_faults, "Toggles fault handling "
		"for debugger commands",
		"[0|1]\n"
		"Toggles fault handling on (1) or off (0).\n", 0);
	add_debugger_command_etc("head", &cmd_head,
		"Prints only the first lines of output from another command",
		"<maxLines>\n"
		"Should be used in a command pipe. It prints only the first\n"
		"<maxLines> lines of output from the previous command in the pipe and\n"
		"silently discards the rest of the output.\n", 0);
	add_debugger_command_etc("tail", &cmd_tail,
		"Prints only the last lines of output from another command",
		"[ <maxLines> ]\n"
		"Should be used in a command pipe. It prints only the last\n"
		"<maxLines> (default 10) lines of output from the previous command in\n"
		"the pipe and silently discards the rest of the output.\n",
		B_KDEBUG_PIPE_FINAL_RERUN);
	add_debugger_command_etc("grep", &cmd_grep,
		"Filters output from another command",
		"[ -i ] [ -v ] <pattern>\n"
		"Should be used in a command pipe. It filters all output from the\n"
		"previous command in the pipe according to the given pattern.\n"
		"When \"-v\" is specified, only those lines are printed that don't\n"
		"match the given pattern, otherwise only those that do match. When\n"
		"\"-i\" is specified, the pattern is matched case insensitive,\n"
		"otherwise case sensitive.\n", 0);
	add_debugger_command_etc("wc", &cmd_wc,
		"Counts the lines, words, and characters of another command's output",
		"<maxLines>\n"
		"Should be used in a command pipe. It prints how many lines, words,\n"
		"and characters the output of the previous command consists of.\n",
		B_KDEBUG_PIPE_FINAL_RERUN);
}
