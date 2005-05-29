/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This file contains the debugger */

#include "blue_screen.h"

#include <debug.h>
#include <frame_buffer_console.h>
#include <gdb.h>

#include <smp.h>
#include <int.h>
#include <vm.h>
#include <driver_settings.h>
#include <arch/debug_console.h>
#include <arch/debug.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct debugger_command {
	struct debugger_command *next;
	int (*func)(int, char **);
	const char *name;
	const char *description;
} debugger_command;

int dbg_register_file[B_MAX_CPU_COUNT][14];
	/* XXXmpetit -- must be made generic */

static bool sSerialDebugEnabled = false;
static bool sSyslogOutputEnabled = false;
static bool sBlueScreenEnabled = false;
static bool sBlueScreenOutput = false;
static spinlock sSpinlock = 0;
static int sDebuggerOnCPU = -1;

static struct debugger_command *sCommands;

#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];

#define LINE_BUF_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char line_buf[HISTORY_SIZE][LINE_BUF_SIZE] = { "", };
static char sParseLine[LINE_BUF_SIZE] = "";
static int cur_line = 0;
static char *args[MAX_ARGS] = { NULL, };

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))


static debugger_command *
find_command(char *name)
{
	debugger_command *command;
	int length;

	// search command by full name

	for (command = sCommands; command != NULL; command = command->next) {
		if (strcmp(name, command->name) == 0)
			return command;
	}

	// if it couldn't be found, search for a partial match

	length = strlen(name);

	for (command = sCommands; command != NULL; command = command->next) {
		if (strncmp(name, command->name, length) == 0)
			return command;
	}

	return NULL;
}


static int
read_line(char *buf, int max_len)
{
	char c;
	int ptr = 0;
	bool done = false;
	int cur_history_spot = cur_line;

	char (*readChar)(void);
	if (sBlueScreenEnabled)
		readChar = blue_screen_getchar;
	else
		readChar = arch_debug_serial_getchar;

	while (!done) {
		c = readChar();

		switch (c) {
			case '\n':
			case '\r':
				buf[ptr++] = '\0';
				debug_putchar('\n');
				done = true;
				break;
			case 8: // backspace
				if (ptr > 0) {
					debug_puts("\x1b[1D"); // move to the left one
					debug_putchar(' ');
					debug_puts("\x1b[1D"); // move to the left one
					ptr--;
				}
				break;
			case 27: // escape sequence
				c = readChar(); // should be '['
				c = readChar();
				switch (c) {
					case 67: // right arrow acts like space
						buf[ptr++] = ' ';
						debug_putchar(' ');
						break;
					case 68: // left arrow acts like backspace
						if (ptr > 0) {
							debug_puts("\x1b[1D"); // move to the left one
							debug_putchar(' ');
							debug_puts("\x1b[1D"); // move to the left one
							ptr--;
						}
						break;
					case 65: // up arrow
					case 66: // down arrow
					{
						int history_line = 0;

//						dprintf("1c %d h %d ch %d\n", cur_line, history_line, cur_history_spot);

						if (c == 65) {
							// up arrow
							history_line = cur_history_spot - 1;
							if (history_line < 0)
								history_line = HISTORY_SIZE - 1;
						} else {
							// down arrow
							if (cur_history_spot != cur_line) {
								history_line = cur_history_spot + 1;
								if(history_line >= HISTORY_SIZE)
									history_line = 0;
							} else
								break; // nothing to do here
						}

//						dprintf("2c %d h %d ch %d\n", cur_line, history_line, cur_history_spot);

						// swap the current line with something from the history
						if (ptr > 0)
							dprintf("\x1b[%dD", ptr); // move to beginning of line

						strcpy(buf, line_buf[history_line]);
						ptr = strlen(buf);
						dprintf("%s\x1b[K", buf); // print the line and clear the rest
						cur_history_spot = history_line;
						break;
					}
					default:
						break;
				}
				break;
			case '$':
			case '+':
				if (!sBlueScreenEnabled) {
					/* HACK ALERT!!!
					 *
					 * If we get a $ at the beginning of the line
					 * we assume we are talking with GDB
					 */
					if (ptr == 0) {
						strcpy(buf, "gdb");
						ptr = 4;
						done = true;
						break;
					} 
				}
				/* supposed to fall through */
			default:
				buf[ptr++] = c;
				debug_putchar(c);
		}
		if (ptr >= max_len - 2) {
			buf[ptr++] = '\0';
			debug_puts("\n");
			done = true;
			break;
		}
	}
	return ptr;
}


static int
parse_line(char *buf, char **argv, int *argc, int max_args)
{
	int pos = 0;

	strcpy(sParseLine, buf);

	if (!isspace(sParseLine[0])) {
		argv[0] = sParseLine;
		*argc = 1;
	} else
		*argc = 0;

	while (sParseLine[pos] != '\0') {
		if (isspace(sParseLine[pos])) {
			sParseLine[pos] = '\0';
			// scan all of the whitespace out of this
			while (isspace(sParseLine[++pos]))
				;
			if (sParseLine[pos] == '\0')
				break;
			argv[*argc] = &sParseLine[pos];
			(*argc)++;

			if (*argc >= max_args - 1)
				break;
		}
		pos++;
	}

	return *argc;
}


static void
kernel_debugger_loop(void)
{
	dprintf("Running on CPU %d\n", smp_get_current_cpu());

	sDebuggerOnCPU = smp_get_current_cpu();

	for (;;) {
		struct debugger_command *cmd = NULL;
		int argc;

		dprintf("kdebug> ");
		read_line(line_buf[cur_line], LINE_BUF_SIZE);
		parse_line(line_buf[cur_line], args, &argc, MAX_ARGS);

		// We support calling last executed command again if
		// B_KDEDUG_CONT was returned last time, so cmd != NULL
		if (argc <= 0 && cmd == NULL)
			continue;

		sDebuggerOnCPU = smp_get_current_cpu();

		if (argc > 0)
			cmd = find_command(args[0]);

		if (cmd == NULL)
			dprintf("unknown command, enter \"help\" to get a list of all supported commands\n");
		else {
			int rc = cmd->func(argc, args);

			if (rc == B_KDEBUG_QUIT)
				break;	// okay, exit now.

			if (rc != B_KDEBUG_CONT)
				cmd = NULL;		// forget last command executed...
		}

		cur_line++;
		if (cur_line >= HISTORY_SIZE)
			cur_line = 0;
	}
}


static int
cmd_reboot(int argc, char **argv)
{
	arch_cpu_shutdown(true);
	return 0;
		// I'll be really suprised if this line ever runs! ;-)
}


static int
cmd_help(int argc, char **argv)
{
	debugger_command *command, *specified = NULL;

	if (argc > 1)
		specified = find_command(argv[1]);

	if (specified != NULL) {
		// only print out the help of the specified command (and all of its aliases)
		dprintf("debugger command for \"%s\" and aliases:\n", specified->name);
	} else
		dprintf("debugger commands:\n");

	for (command = sCommands; command != NULL; command = command->next) {
		if (specified && command->func != specified->func)
			continue;

		dprintf(" %-20s\t\t%s\n", command->name, command->description ? command->description : "-");
	}

	return 0;
}


static int
cmd_continue(int argc, char **argv)
{
	return B_KDEBUG_QUIT;
}


//	#pragma mark -


void
debug_putchar(char c)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	if (sSerialDebugEnabled)
		arch_debug_serial_putchar(c);
	if (sBlueScreenOutput)
		blue_screen_putchar(c);

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
debug_puts(const char *s)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	if (sSerialDebugEnabled)
		arch_debug_serial_puts(s);
	if (sBlueScreenOutput)
		blue_screen_puts(s);

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
debug_early_boot_message(const char *string)
{
	arch_debug_serial_early_boot_message(string);
}


status_t
debug_init(kernel_args *args)
{
	return arch_debug_console_init(args);
}


status_t
debug_init_post_vm(kernel_args *args)
{
	void *handle;

	add_debugger_command("help", &cmd_help, "List all debugger commands");
	add_debugger_command("reboot", &cmd_reboot, "Reboot the system");
	add_debugger_command("gdb", &cmd_gdb, "Connect to remote gdb");
	add_debugger_command("continue", &cmd_continue, "Leave kernel debugger");
	add_debugger_command("exit", &cmd_continue, NULL);
	add_debugger_command("es", &cmd_continue, NULL);

	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);

	// get debug settings
	handle = load_driver_settings("kernel");
	if (handle != NULL) {
		if (get_driver_boolean_parameter(handle, "serial_debug_output", true, true))
			sSerialDebugEnabled = true;
		if (get_driver_boolean_parameter(handle, "syslog_debug_output", false, false))
			sSyslogOutputEnabled = true;
		if (get_driver_boolean_parameter(handle, "bluescreen", false, false)) {
			if (blue_screen_init() == B_OK)
				sBlueScreenEnabled = true;
		}

		unload_driver_settings(handle);
	}

	return arch_debug_init(args);
}


//	#pragma mark -
//	public API


int
add_debugger_command(char *name, int (*func)(int, char **), char *desc)
{
	cpu_status state;
	struct debugger_command *cmd;

	cmd = (struct debugger_command *)malloc(sizeof(struct debugger_command));
	if (cmd == NULL)
		return ENOMEM;

	cmd->func = func;
	cmd->name = name;
	cmd->description = desc;

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	cmd->next = sCommands;
	sCommands = cmd;

	release_spinlock(&sSpinlock);
	restore_interrupts(state);

	return B_NO_ERROR;
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


uint32
parse_expression(const char *expression)
{
	return strtoul(expression, NULL, 0);
}


void
panic(const char *format, ...)
{
	va_list args;
	char temp[128];
	
	set_dprintf_enabled(true);

	va_start(args, format);
	vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	dprintf("PANIC: %s", temp);
	kernel_debugger(NULL);
}


void
kernel_debugger(const char *message)
{
	cpu_status state;

	arch_debug_save_registers(&dbg_register_file[smp_get_current_cpu()][0]);
	set_dprintf_enabled(true);

	state = disable_interrupts();

	// XXX by setting kernel_startup = true, we disable
	// XXX the interrupt check in semaphore code etc.
	// XXX should be renamed?
	kernel_startup = true;

	if (sDebuggerOnCPU != smp_get_current_cpu()) {
		// halt all of the other cpus

		// XXX need to flush current smp mailbox to make sure this goes
		// through. Otherwise it'll hang
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenEnabled) {
		sBlueScreenOutput = true;
		blue_screen_enter();
	}

	if (message) {
		kprintf(message);
		kprintf("\n");
	}

	kprintf("Welcome to Kernel Debugging Land...\n");
	kernel_debugger_loop();

	sBlueScreenOutput = false;
	kernel_startup = false;
	restore_interrupts(state);

	// ToDo: in case we change dbg_register_file - don't we want to restore it?
}


bool
set_dprintf_enabled(bool newState)
{
	bool oldState = sSerialDebugEnabled;
	sSerialDebugEnabled = newState;

	return oldState;
}


void
dprintf(const char *format, ...)
{
	cpu_status state;
	va_list args;

	if (!sSerialDebugEnabled)
		return;

	// ToDo: maybe add a non-interrupt buffer and path that only
	//	needs to acquire a semaphore instead of needing to disable
	//	interrupts?

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	va_start(args, format);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	arch_debug_serial_puts(sOutputBuffer);
	if (sBlueScreenOutput)
		blue_screen_puts(sOutputBuffer);

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


/**	Similar to dprintf() but thought to be used in the kernel
 *	debugger only.
 */

void
kprintf(const char *format, ...)
{
	va_list args;

	// ToDo: don't print anything if the debugger is not running!

	va_start(args, format);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	arch_debug_serial_puts(sOutputBuffer);
	if (sBlueScreenOutput)
		blue_screen_puts(sOutputBuffer);
}


//	#pragma mark -
//	userland syscalls


void
_user_debug_output(const char *userString)
{
	char string[512];
	int32 length;

	if (!sSerialDebugEnabled)
		return;

	if (!IS_USER_ADDRESS(userString))
		return;

	do {
		length = user_strlcpy(string, userString, sizeof(string));
		debug_puts(string);
		userString += sizeof(string) - 1;
	} while (length >= (ssize_t)sizeof(string));
}
