/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This file contains the debugger */

#include "blue_screen.h"
#include "gdb.h"

#include <debug.h>
#include <driver_settings.h>
#include <frame_buffer_console.h>
#include <int.h>
#include <smp.h>
#include <vm.h>

#include <arch/debug_console.h>
#include <arch/debug.h>
#include <util/ring_buffer.h>

#include <syslog_daemon.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>


int kgets(char *buffer, int length);

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
static bool sDebugScreenEnabled = false;
static bool sBlueScreenOutput = true;
static spinlock sSpinlock = 0;
static int32 sDebuggerOnCPU = -1;

static sem_id sSyslogNotify = -1;
static struct syslog_message *sSyslogMessage;
static struct ring_buffer *sSyslogBuffer;
static bool sSyslogDropped = false;

static struct debugger_command *sCommands;

#define SYSLOG_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];
static char sLastOutputBuffer[OUTPUT_BUFFER_SIZE];

static void flush_pending_repeats(void);
static void check_pending_repeats(void *data, int iter);

static int64 sMessageRepeatTime = 0;
static int32 sMessageRepeatCount = 0;

#define LINE_BUFFER_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char sLineBuffer[HISTORY_SIZE][LINE_BUFFER_SIZE] = { "", };
static char sParseLine[LINE_BUFFER_SIZE] = "";
static int32 sCurrentLine = 0;
static char *args[MAX_ARGS] = { NULL, };

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))


static debugger_command *
find_command(char *name, bool partialMatch)
{
	debugger_command *command;
	int length;

	// search command by full name

	for (command = sCommands; command != NULL; command = command->next) {
		if (strcmp(name, command->name) == 0)
			return command;
	}

	// if it couldn't be found, search for a partial match

	if (partialMatch) {
		length = strlen(name);
		if (length == 0)
			return NULL;

		for (command = sCommands; command != NULL; command = command->next) {
			if (strncmp(name, command->name, length) == 0)
				return command;
		}
	}

	return NULL;
}


static void
kputchar(char c)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_putchar(c);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_putchar(c);
}


static void
kputs(const char *s)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_puts(s);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_puts(s);
}


static int
read_line(char *buffer, int32 maxLength)
{
	int32 currentHistoryLine = sCurrentLine;
	int32 position = 0;
	bool done = false;
	char c;

	char (*readChar)(void);
	if (sBlueScreenOutput)
		readChar = blue_screen_getchar;
	else
		readChar = arch_debug_serial_getchar;

	while (!done) {
		c = readChar();

		switch (c) {
			case '\n':
			case '\r':
				buffer[position++] = '\0';
				kputchar('\n');
				done = true;
				break;
			case 8: // backspace
				if (position > 0) {
					kputs("\x1b[1D"); // move to the left one
					kputchar(' ');
					kputs("\x1b[1D"); // move to the left one
					position--;
				}
				break;
			case 27: // escape sequence
				c = readChar();
				if (c != '[') {
					// ignore broken escape sequence
					break;
				}
				c = readChar();
				switch (c) {
					case 'C': // right arrow acts like space
						buffer[position++] = ' ';
						kputchar(' ');
						break;
					case 'D': // left arrow acts like backspace
						if (position > 0) {
							kputs("\x1b[1D"); // move to the left one
							kputchar(' ');
							kputs("\x1b[1D"); // move to the left one
							position--;
						}
						break;
					case 'A': // up arrow
					case 'B': // down arrow
					{
						int32 historyLine = 0;

						if (c == 65) {
							// up arrow
							historyLine = currentHistoryLine - 1;
							if (historyLine < 0)
								historyLine = HISTORY_SIZE - 1;
						} else {
							// down arrow
							if (currentHistoryLine == sCurrentLine)
								break;

							historyLine = currentHistoryLine + 1;
							if (historyLine >= HISTORY_SIZE)
								historyLine = 0;
						}

						// clear the history again if we're in the current line again
						// (the buffer we get just is the current line buffer)
						if (historyLine == sCurrentLine)
							sLineBuffer[historyLine][0] = '\0';

						// swap the current line with something from the history
						if (position > 0)
							kprintf("\x1b[%ldD", position); // move to beginning of line

						strcpy(buffer, sLineBuffer[historyLine]);
						position = strlen(buffer);
						kprintf("%s\x1b[K", buffer); // print the line and clear the rest
						currentHistoryLine = historyLine;
						break;
					}
					default:
						break;
				}
				break;
			case '$':
			case '+':
				if (!sBlueScreenOutput) {
					/* HACK ALERT!!!
					 *
					 * If we get a $ at the beginning of the line
					 * we assume we are talking with GDB
					 */
					if (position == 0) {
						strcpy(buffer, "gdb");
						position = 4;
						done = true;
						break;
					} 
				}
				/* supposed to fall through */
			default:
				if (isprint(c)) {
					buffer[position++] = c;
					kputchar(c);
				}
				break;
		}

		if (position >= maxLength - 2) {
			buffer[position++] = '\0';
			kputchar('\n');
			done = true;
			break;
		}
	}

	return position;
}


int
kgets(char *buffer, int length)
{
	return read_line(buffer, length);
}


static int
parse_line(char *buf, char **argv, int *argc, int max_args)
{
	int pos = 0;

	strcpy(sParseLine, buf);

	if (sParseLine[0] != '\0' && !isspace(sParseLine[0])) {
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
	int32 previousCPU = sDebuggerOnCPU;
	sDebuggerOnCPU = smp_get_current_cpu();

	kprintf("Welcome to Kernel Debugging Land...\n");
	kprintf("Running on CPU %ld\n", sDebuggerOnCPU);

	for (;;) {
		struct debugger_command *cmd = NULL;
		int argc;

		kprintf("kdebug> ");
		read_line(sLineBuffer[sCurrentLine], LINE_BUFFER_SIZE);
		parse_line(sLineBuffer[sCurrentLine], args, &argc, MAX_ARGS);

		// We support calling last executed command again if
		// B_KDEDUG_CONT was returned last time, so cmd != NULL
		if (argc <= 0 && cmd == NULL)
			continue;

		sDebuggerOnCPU = smp_get_current_cpu();

		if (argc > 0)
			cmd = find_command(args[0], true);

		if (cmd == NULL)
			kprintf("unknown command, enter \"help\" to get a list of all supported commands\n");
		else {
			int rc = cmd->func(argc, args);

			if (rc == B_KDEBUG_QUIT)
				break;	// okay, exit now.

			if (rc != B_KDEBUG_CONT)
				cmd = NULL;		// forget last command executed...
		}

		if (++sCurrentLine >= HISTORY_SIZE)
			sCurrentLine = 0;
	}

	sDebuggerOnCPU = previousCPU;
}


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

	if (argc > 1) {
		specified = find_command(argv[1], false);
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

	for (command = sCommands; command != NULL; command = command->next) {
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


static status_t
syslog_sender(void *data)
{
	status_t error = B_BAD_PORT_ID;
	port_id port = -1;
	bool bufferPending = false;
	int32 length = 0;

	while (true) {
		// wait for syslog data to become available
		acquire_sem(sSyslogNotify);

		sSyslogMessage->when = real_time_clock();

		if (error == B_BAD_PORT_ID) {
			// last message couldn't be sent, try to locate the syslog_daemon
			port = find_port(SYSLOG_PORT_NAME);
		}

		if (port >= B_OK) {
			if (!bufferPending) {
				// we need to have exclusive access to our syslog buffer
				cpu_status state = disable_interrupts();
				acquire_spinlock(&sSpinlock);

				length = ring_buffer_readable(sSyslogBuffer);
				if (length > SYSLOG_MAX_MESSAGE_LENGTH)
					length = SYSLOG_MAX_MESSAGE_LENGTH;

				length = ring_buffer_read(sSyslogBuffer,
					(uint8 *)sSyslogMessage->message, length);
				if (sSyslogDropped) {
					// add drop marker
					if (length < SYSLOG_MAX_MESSAGE_LENGTH - 6)
						strlcat(sSyslogMessage->message, "<DROP>", SYSLOG_MAX_MESSAGE_LENGTH);
					else if (length > 7)
						strcpy(sSyslogMessage->message + length - 7, "<DROP>");

					sSyslogDropped = false;
				}

				release_spinlock(&sSpinlock);
				restore_interrupts(state);
			}

			if (length == 0) {
				// the buffer we came here for might have been sent already
				bufferPending = false;
				continue;
			}

			error = write_port_etc(port, SYSLOG_MESSAGE, sSyslogMessage,
				sizeof(struct syslog_message) + length, B_RELATIVE_TIMEOUT, 0);

			if (error < B_OK) {
				// sending has failed - just wait, maybe it'll work later.
				bufferPending = true;
				continue;
			}

			if (bufferPending) {
				// we could write the last pending buffer, try to read more
				// from the syslog ring buffer
				release_sem_etc(sSyslogNotify, 1, B_DO_NOT_RESCHEDULE);
				bufferPending = false;
			}
		}
	}

	return 0;
}


static void
syslog_write(const char *text, int32 length)
{
	bool trunc = false;

	if (sSyslogBuffer == NULL)
		return;

	if (ring_buffer_writable(sSyslogBuffer) < length) {
		// truncate data
		length = ring_buffer_writable(sSyslogBuffer);

		if (length > 8) {
			trunc = true;
			length -= 8;
		} else
			sSyslogDropped = true;
	}

	ring_buffer_write(sSyslogBuffer, (uint8 *)text, length);
	if (trunc)
		ring_buffer_write(sSyslogBuffer, (uint8 *)"<TRUNC>", 7);

	release_sem_etc(sSyslogNotify, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
syslog_init_post_threads(void)
{
	if (!sSyslogOutputEnabled)
		return B_OK;

	sSyslogNotify = create_sem(0, "syslog data");
	if (sSyslogNotify >= B_OK) {
		thread_id thread = spawn_kernel_thread(syslog_sender, "syslog sender",
			B_LOW_PRIORITY, NULL);
		if (thread >= B_OK && resume_thread(thread) == B_OK)
			return B_OK;
	}

	// initializing kernel syslog service failed -- disable it

	sSyslogOutputEnabled = false;
	free(sSyslogMessage);
	free(sSyslogBuffer);
	delete_sem(sSyslogNotify);

	return B_ERROR;
}


static status_t
syslog_init(void)
{
	status_t status;

	if (!sSyslogOutputEnabled)
		return B_OK;

	sSyslogMessage = malloc(SYSLOG_MESSAGE_BUFFER_SIZE);
	if (sSyslogMessage == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	sSyslogBuffer = create_ring_buffer(SYSLOG_BUFFER_SIZE);
	if (sSyslogBuffer == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	// initialize syslog message
	sSyslogMessage->from = 0;
	sSyslogMessage->options = LOG_KERN;
	sSyslogMessage->priority = LOG_DEBUG;
	sSyslogMessage->ident[0] = '\0';
	//strcpy(sSyslogMessage->ident, "KERNEL");

	return B_OK;

err3:
	free(sSyslogBuffer);
err2:
	free(sSyslogMessage);
err1:
	sSyslogOutputEnabled = false;
	return status;
}


//	#pragma mark - private kernel API


void
debug_stop_screen_debug_output(void)
{
	sDebugScreenEnabled = false;
}


bool
debug_debugger_running(void)
{
	return sDebuggerOnCPU != -1;
}


void
debug_puts(const char *string, int32 length)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	if (length >= OUTPUT_BUFFER_SIZE)
		length = OUTPUT_BUFFER_SIZE - 1;

	if (strncmp(string, sLastOutputBuffer, length) == 0
		&& length > 1 && string[length - 1] == '\n') {
		sMessageRepeatCount++;
		sMessageRepeatTime = system_time();
	} else {
		flush_pending_repeats();
		kputs(string);

		// kputs() doesn't output to syslog (as it's only used
		// from the kernel debugger elsewhere)
		if (sSyslogOutputEnabled)
			syslog_write(string, length);

		memcpy(sLastOutputBuffer, string, length);
		sLastOutputBuffer[length] = 0;
	}

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
	add_debugger_command("shutdown", &cmd_shutdown, "Shut down the system");
	add_debugger_command("gdb", &cmd_gdb, "Connect to remote gdb");
	add_debugger_command("exit", &cmd_continue, "Same as \"continue\"");
	add_debugger_command("es", &cmd_continue, "Same as \"continue\"");
	add_debugger_command("continue", &cmd_continue, "Leave kernel debugger");

	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);

	// get debug settings
	handle = load_driver_settings("kernel");
	if (handle != NULL) {
		sSerialDebugEnabled = get_driver_boolean_parameter(handle,
			"serial_debug_output", true, true);
		sSyslogOutputEnabled = get_driver_boolean_parameter(handle,
			"syslog_debug_output", false, false);
		sBlueScreenOutput = get_driver_boolean_parameter(handle,
			"bluescreen", true, true);

		unload_driver_settings(handle);
	}

	handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (handle != NULL) {
		sDebugScreenEnabled = get_driver_boolean_parameter(handle,
			"debug_screen", false, false);

		unload_driver_settings(handle);
	}

	if ((sBlueScreenOutput || sDebugScreenEnabled)
		&& blue_screen_init() != B_OK)
		sBlueScreenOutput = sDebugScreenEnabled = false;

	if (sDebugScreenEnabled)
		blue_screen_enter(true);

	syslog_init();

	return arch_debug_init(args);
}


status_t
debug_init_post_modules(struct kernel_args *args)
{
	void *cookie;

	// check for dupped lines every 10/10 second
	register_kernel_daemon(check_pending_repeats, NULL, 10);

	syslog_init_post_threads();

	// load kernel debugger addons
	cookie = open_module_list("debugger");
	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);
		module_info *module;

		if (read_next_module_name(cookie, name, &nameLength) != B_OK)
			break;
		if (get_module(name, &module) == B_OK)
			dprintf("kernel debugger extention \"%s\": loaded\n", name);
		else
			dprintf("kernel debugger extention \"%s\": failed to load\n", name);
	}
	close_module_list(cookie);

	return frame_buffer_console_init_post_modules(args);
}


//	#pragma mark - public API


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

	kernel_debugger(temp);
}


void
kernel_debugger(const char *message)
{
	cpu_status state;

	arch_debug_save_registers(&dbg_register_file[smp_get_current_cpu()][0]);
	set_dprintf_enabled(true);

	state = disable_interrupts();

	if (sDebuggerOnCPU != smp_get_current_cpu()) {
		// halt all of the other cpus

		// XXX need to flush current smp mailbox to make sure this goes
		// through. Otherwise it'll hang
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenOutput) {
		sBlueScreenEnabled = true;
		blue_screen_enter(false);
	}

	if (message)
		kprintf("PANIC: %s\n", message);

	kernel_debugger_loop();

	sBlueScreenEnabled = false;
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


static void
flush_pending_repeats(void)
{
	if (sMessageRepeatCount > 0) {
		int32 length;

		if (sMessageRepeatCount > 1) {
			static char temp[40];
			length = snprintf(temp, sizeof(temp),
				"Last message repeated %ld times.\n", sMessageRepeatCount);

			if (sSerialDebugEnabled)
				arch_debug_serial_puts(temp);
			if (sSyslogOutputEnabled)
				syslog_write(temp, length);
			if (sBlueScreenEnabled || sDebugScreenEnabled)
				blue_screen_puts(temp);
		} else {
			// if we only have one repeat just reprint the last buffer
			if (sSerialDebugEnabled)
				arch_debug_serial_puts(sLastOutputBuffer);
			if (sSyslogOutputEnabled)
				syslog_write(sLastOutputBuffer, strlen(sLastOutputBuffer));
			if (sBlueScreenEnabled || sDebugScreenEnabled)
				blue_screen_puts(sLastOutputBuffer);
		}

		sMessageRepeatCount = 0;
	}
}


static void
check_pending_repeats(void *data, int iter)
{
	(void)data;
	(void)iter;
	if (sMessageRepeatCount > 0
		&& (system_time() - sMessageRepeatTime) > 1000000) {
		cpu_status state = disable_interrupts();
		acquire_spinlock(&sSpinlock);

		flush_pending_repeats();

		release_spinlock(&sSpinlock);
		restore_interrupts(state);
	}
}


void
dprintf(const char *format, ...)
{
	cpu_status state;
	va_list args;
	int32 length;

	if (!sSerialDebugEnabled && !sSyslogOutputEnabled && !sBlueScreenEnabled)
		return;

	// ToDo: maybe add a non-interrupt buffer and path that only
	//	needs to acquire a semaphore instead of needing to disable
	//	interrupts?

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	va_start(args, format);
	length = vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	if (length >= OUTPUT_BUFFER_SIZE)
		length = OUTPUT_BUFFER_SIZE - 1;

	if (strncmp(sOutputBuffer, sLastOutputBuffer, length) == 0
		&& length > 1 && sOutputBuffer[length - 1] == '\n') {
		sMessageRepeatCount++;
		sMessageRepeatTime = system_time();
	} else {
		flush_pending_repeats();

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(sOutputBuffer);
		if (sSyslogOutputEnabled)
			syslog_write(sOutputBuffer, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(sOutputBuffer);

		memcpy(sLastOutputBuffer, sOutputBuffer, length);
		sLastOutputBuffer[length] = 0;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


/**	Similar to dprintf() but thought to be used in the kernel
 *	debugger only (it doesn't lock).
 */

void
kprintf(const char *format, ...)
{
	va_list args;

	// ToDo: don't print anything if the debugger is not running!

	va_start(args, format);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	flush_pending_repeats();
	kputs(sOutputBuffer);
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
		debug_puts(string, length);
		userString += sizeof(string) - 1;
	} while (length >= (ssize_t)sizeof(string));
}


void
dump_block(const char *buffer, int size, const char *prefix)
{
	const int DUMPED_BLOCK_SIZE = 16;
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		dprintf(prefix);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				dprintf(" ");

			if (i >= size)
				dprintf("  ");
			else
				dprintf("%02x", *(unsigned char *)(buffer + i));
		}
		dprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					dprintf(".");
				else
					dprintf("%c", c);
			} else
				break;
		}
		dprintf("\n");
	}
}
