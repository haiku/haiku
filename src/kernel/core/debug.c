/* This file contains the debugger */

/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <debug.h>
#include <arch/int.h>
#include <smp.h>
#include <console.h>
#include <gdb.h>
#include <int.h>
#include <vm.h>

#include <arch/dbg_console.h>
#include <arch/debug.h>
#include <arch/cpu.h>

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
static spinlock dbg_spinlock = 0;
static int debugger_on_cpu = -1;

static struct debugger_command *sCommands;

#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];

#define LINE_BUF_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char line_buf[HISTORY_SIZE][LINE_BUF_SIZE] = { "", };
static char parse_line[LINE_BUF_SIZE] = "";
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
debug_read_line(char *buf, int max_len)
{
	char c;
	int ptr = 0;
	bool done = false;
	int cur_history_spot = cur_line;

	while (!done) {
		c = arch_dbg_con_read();
		switch (c) {
			case '\n':
			case '\r':
				buf[ptr++] = '\0';
				dbg_puts("\n");
				done = true;
				break;
			case 8: // backspace
				if (ptr > 0) {
					dbg_puts("\x1b[1D"); // move to the left one
					dbg_putch(' ');
					dbg_puts("\x1b[1D"); // move to the left one
					ptr--;
				}
				break;
			case 27: // escape sequence
				c = arch_dbg_con_read(); // should be '['
				c = arch_dbg_con_read();
				switch (c) {
					case 67: // right arrow acts like space
						buf[ptr++] = ' ';
						dbg_putch(' ');
						break;
					case 68: // left arrow acts like backspace
						if (ptr > 0) {
							dbg_puts("\x1b[1D"); // move to the left one
							dbg_putch(' ');
							dbg_puts("\x1b[1D"); // move to the left one
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
						if(ptr > 0)
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
				/* HACK ALERT!!!
				 *
				 * If we get a $ at the beginning of the line
				 * we assume we are talking with GDB
				 */
				if (ptr == 0) {
					strcpy(buf, "gdb");
					ptr= 4;
					done= true;
					break;
				} else {
					/* fall thru */
				}
			default:
				buf[ptr++] = c;
				dbg_putch(c);
		}
		if (ptr >= max_len - 2) {
			buf[ptr++] = '\0';
			dbg_puts("\n");
			done = true;
			break;
		}
	}
	return ptr;
}


static int
debug_parse_line(char *buf, char **argv, int *argc, int max_args)
{
	int pos = 0;

	strcpy(parse_line, buf);

	if (!isspace(parse_line[0])) {
		argv[0] = parse_line;
		*argc = 1;
	} else
		*argc = 0;

	while (parse_line[pos] != '\0') {
		if (isspace(parse_line[pos])) {
			parse_line[pos] = '\0';
			// scan all of the whitespace out of this
			while (isspace(parse_line[++pos]))
				;
			if (parse_line[pos] == '\0')
				break;
			argv[*argc] = &parse_line[pos];
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
	int argc;
	struct debugger_command *cmd;
	cpu_status state;

	state = disable_interrupts();

	dprintf("kernel debugger on cpu %d\n", smp_get_current_cpu());
	debugger_on_cpu = smp_get_current_cpu();

	cmd = NULL;

	for (;;) {
		dprintf("kdebug> ");
		debug_read_line(line_buf[cur_line], LINE_BUF_SIZE);
		debug_parse_line(line_buf[cur_line], args, &argc, MAX_ARGS);

		// We support calling last executed command again if
		// B_KDEDUG_CONT was returned last time, so cmd != NULL
		if (argc <= 0 && cmd == NULL)
			continue;

		debugger_on_cpu = smp_get_current_cpu();

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

	restore_interrupts(state);
}


char
dbg_putch(char c)
{
	char ret;
	cpu_status state = disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	if (sSerialDebugEnabled)
		ret = arch_dbg_con_putch(c);
	else
		ret = c;

	release_spinlock(&dbg_spinlock);
	restore_interrupts(state);

	return ret;
}


void
dbg_puts(const char *s)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	if (sSerialDebugEnabled)
		arch_dbg_con_puts(s);

	release_spinlock(&dbg_spinlock);
	restore_interrupts(state);
}


static int
cmd_reboot(int argc, char **argv)
{
	reboot();
	return 0;
		// I'll be really suprised if this line ever run! ;-)
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


status_t
debug_init(kernel_args *args)
{
	return arch_dbg_con_init(args);
}


status_t
debug_init_post_vm(kernel_args *args)
{
	add_debugger_command("help", &cmd_help, "List all debugger commands");
	add_debugger_command("reboot", &cmd_reboot, "Reboot the system");
	add_debugger_command("gdb", &cmd_gdb, "Connect to remote gdb");
	add_debugger_command("continue", &cmd_continue, "Leave kernel debugger");
	add_debugger_command("exit", &cmd_continue, NULL);
	add_debugger_command("es", &cmd_continue, NULL);

	return arch_dbg_init(args);
}

// ToDo: this one is probably not needed
#if 0
bool
dbg_get_serial_debug()
{
	return sSerialDebugEnabled;
}
#endif

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
	acquire_spinlock(&dbg_spinlock);

	cmd->next = sCommands;
	sCommands = cmd;

	release_spinlock(&dbg_spinlock);
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
	acquire_spinlock(&dbg_spinlock);

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

	release_spinlock(&dbg_spinlock);
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
panic(const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	char temp[128];
	cpu_status state;
	
	// XXX by setting kernel_startup = true, we disable
	// XXX the interrupt check in semaphore code etc.
	// XXX should be renamed?
	kernel_startup = true;

	set_dprintf_enabled(true);

	state = disable_interrupts();

	va_start(args, fmt);
	ret = vsprintf(temp, fmt, args);
	va_end(args);

	dprintf("PANIC%d: %s", smp_get_current_cpu(), temp);

	if (debugger_on_cpu != smp_get_current_cpu()) {
		// halt all of the other cpus

		// XXX need to flush current smp mailbox to make sure this goes
		// through. Otherwise it'll hang
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	kernel_debugger(NULL);

	restore_interrupts(state);
}


void
kernel_debugger(const char * message)
{
	arch_dbg_save_registers(&(dbg_register_file[smp_get_current_cpu()][0]));

	if (message) {
		dprintf(message);
		dprintf("\n");
	};

	dprintf("Welcome to Kernel Debugging Land...\n");
	kernel_debugger_loop();
}


bool
set_dprintf_enabled(bool newState)
{
	bool oldState = sSerialDebugEnabled;
	sSerialDebugEnabled = newState;

	return oldState;
}


void
dprintf(const char *fmt, ...)
{
	cpu_status state;
	va_list args;

	if (!sSerialDebugEnabled)
		return;

	// ToDo: maybe add a non-interrupt buffer and path that only
	//	needs to acquire a semaphore instead of needing to disable
	//	interrupts?

	state = disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	va_start(args, fmt);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, fmt, args);
	va_end(args);

	arch_dbg_con_puts(sOutputBuffer);

	release_spinlock(&dbg_spinlock);
	restore_interrupts(state);
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
		dbg_puts(string);
		userString += sizeof(string) - 1;
	} while (length >= (ssize_t)sizeof(string));
}
