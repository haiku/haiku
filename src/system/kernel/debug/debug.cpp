/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*! This file contains the debugger and debug output facilities */

#include "blue_screen.h"
#include "gdb.h"

#include <debug.h>
#include <driver_settings.h>
#include <frame_buffer_console.h>
#include <int.h>
#include <kernel.h>
#include <smp.h>
#include <thread.h>
#include <tracing.h>
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

#include "debug_commands.h"
#include "debug_variables.h"


static const char* const kKDLPrompt = "kdebug> ";

extern "C" int kgets(char *buffer, int length);


int dbg_register_file[B_MAX_CPU_COUNT][14];
	/* XXXmpetit -- must be made generic */

static bool sSerialDebugEnabled = true;
static bool sSyslogOutputEnabled = true;
static bool sBlueScreenEnabled = false;
	// must always be false on startup
static bool sDebugScreenEnabled = false;
static bool sBlueScreenOutput = true;
static spinlock sSpinlock = 0;
static int32 sDebuggerOnCPU = -1;

static sem_id sSyslogNotify = -1;
static struct syslog_message *sSyslogMessage;
static struct ring_buffer *sSyslogBuffer;
static bool sSyslogDropped = false;

static const char* sCurrentKernelDebuggerMessage;

#define SYSLOG_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];
static char sLastOutputBuffer[OUTPUT_BUFFER_SIZE];

static void flush_pending_repeats(void);
static void check_pending_repeats(void *data, int iter);

static int64 sMessageRepeatFirstTime = 0;
static int64 sMessageRepeatLastTime = 0;
static int32 sMessageRepeatCount = 0;

static debugger_module_info *sDebuggerModules[8];
static const uint32 kMaxDebuggerModules = sizeof(sDebuggerModules)
	/ sizeof(sDebuggerModules[0]);

#define LINE_BUFFER_SIZE 1024
#define HISTORY_SIZE 16

static char sLineBuffer[HISTORY_SIZE][LINE_BUFFER_SIZE] = { "", };
static char sParseLine[LINE_BUFFER_SIZE];
static int32 sCurrentLine = 0;

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))


static void
kputchar(char c)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_putchar(c);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_putchar(c);
}


void
kputs(const char *s)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_puts(s);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_puts(s);
}


static void
insert_chars_into_line(char* buffer, int32& position, int32& length,
	const char* chars, int32 charCount)
{
	// move the following chars to make room for the ones to insert
	if (position < length) {
		memmove(buffer + position + charCount, buffer + position,
			length - position);
	}

	// insert chars
	memcpy(buffer + position, chars, charCount);
	int32 oldPosition = position;
	position += charCount;
	length += charCount;

	// print the new chars (and the following ones)
	kprintf("%.*s", (int)(length - oldPosition),
		buffer + oldPosition);

	// reposition cursor, if necessary
	if (position < length)
		kprintf("\x1b[%ldD", length - position);
}


static void
insert_char_into_line(char* buffer, int32& position, int32& length, char c)
{
	insert_chars_into_line(buffer, position, length, &c, 1);
}


static void
remove_char_from_line(char* buffer, int32& position, int32& length)
{
	if (position == length)
		return;

	length--;

	if (position < length) {
		// move the subsequent chars
		memmove(buffer + position, buffer + position + 1, length - position);

		// print the rest of the line again, if necessary
		for (int32 i = position; i < length; i++)
			kputchar(buffer[i]);
	}

	// visually clear the last char
	kputchar(' ');

	// reposition the cursor
	kprintf("\x1b[%ldD", length - position + 1);
}


class LineEditingHelper {
public:
	virtual	~LineEditingHelper() {}

	virtual	void TabCompletion(char* buffer, int32 capacity, int32& position,
		int32& length) = 0;
};


class CommandLineEditingHelper : public LineEditingHelper {
public:
	CommandLineEditingHelper()
	{
	}

	virtual	~CommandLineEditingHelper() {}

	virtual	void TabCompletion(char* buffer, int32 capacity, int32& position,
		int32& length)
	{
		// find the first space
		char tmpChar = buffer[position];
		buffer[position] = '\0';
		char* firstSpace = strchr(buffer, ' ');
		buffer[position] = tmpChar;

		bool reprintLine = false;

		if (firstSpace != NULL) {
			// a complete command -- print its help

			// get the command
			tmpChar = *firstSpace;
			*firstSpace = '\0';
			bool ambiguous;
			debugger_command* command = find_debugger_command(buffer, true, ambiguous);
			*firstSpace = tmpChar;

			if (command != NULL) {
				kputchar('\n');
				print_debugger_command_usage(command->name);
			} else {
				if (ambiguous)
					kprintf("\nambiguous command\n");
				else
					kprintf("\nno such command\n");
			}

			reprintLine = true;
		} else {
			// a partial command -- look for completions

			// check for possible completions
			int32 count = 0;
			int32 longestName = 0;
			debugger_command* command = NULL;
			int32 longestCommonPrefix = 0;
			const char* previousCommandName = NULL;
			while ((command = next_debugger_command(command, buffer, position))
					!= NULL) {
				count++;
				int32 nameLength = strlen(command->name);
				longestName = max_c(longestName, nameLength);

				// updated the length of the longest common prefix of the
				// commands
				if (count == 1) {
					longestCommonPrefix = longestName;
				} else {
					longestCommonPrefix = min_c(longestCommonPrefix,
						nameLength);

					for (int32 i = position; i < longestCommonPrefix; i++) {
						if (previousCommandName[i] != command->name[i]) {
							longestCommonPrefix = i;
							break;
						}
					}
				}

				previousCommandName = command->name;
			}

			if (count == 0) {
				// no possible completions
				kprintf("\nno completions\n");
				reprintLine = true;
			} else if (count == 1) {
				// exactly one completion
				command = next_debugger_command(NULL, buffer, position);

				// check for sufficient space in the buffer
				int32 neededSpace = longestName - position + 1;
					// remainder of the name plus one space
				// also consider the terminating null char
				if (length + neededSpace + 1 >= capacity)
					return;

				insert_chars_into_line(buffer, position, length,
					command->name + position, longestName - position);
				insert_char_into_line(buffer, position, length, ' ');
			} else if (longestCommonPrefix > position) {
				// multiple possible completions with longer common prefix
				// -- insert the remainder of the common prefix
				
				// check for sufficient space in the buffer
				int32 neededSpace = longestCommonPrefix - position;
				// also consider the terminating null char
				if (length + neededSpace + 1 >= capacity)
					return;

				insert_chars_into_line(buffer, position, length,
					previousCommandName + position, neededSpace);
			} else {
				// multiple possible completions without longer common prefix
				// -- print them all
				kprintf("\n");
				reprintLine = true;

				int columns = 80 / (longestName + 2);
				debugger_command* command = NULL;
				int column = 0;
				while ((command = next_debugger_command(command, buffer, position))
						!= NULL) {
					// spacing
					if (column > 0 && column % columns == 0)
						kputchar('\n');
					column++;

					kprintf("  %-*s", (int)longestName, command->name);
				}
				kputchar('\n');
			}
		}

		// reprint the editing line, if necessary
		if (reprintLine) {
			kprintf("%s%.*s", kKDLPrompt, (int)length, buffer);
			if (position < length)
				kprintf("\x1b[%ldD", length - position);
		}
	}
};


static int
read_line(char *buffer, int32 maxLength,
	LineEditingHelper* editingHelper = NULL)
{
	int32 currentHistoryLine = sCurrentLine;
	int32 position = 0;
	int32 length = 0;
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
				buffer[length++] = '\0';
				kputchar('\n');
				done = true;
				break;
			case '\t':
			{
				if (editingHelper != NULL) {
					editingHelper->TabCompletion(buffer, maxLength,
						position, length);
				}
				break;
			}
			case 8: // backspace
				if (position > 0) {
					kputs("\x1b[1D"); // move to the left one
					position--;
					remove_char_from_line(buffer, position, length);
				}
				break;
			case 0x1f & 'K':	// CTRL-K -- clear line after current position
				if (position < length) {
					// clear chars
					for (int32 i = position; i < length; i++)
						kputchar(' ');

					// reposition cursor
					kprintf("\x1b[%ldD", length - position);

					length = position;
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
					case 'C': // right arrow
						if (position < length) {
							kputs("\x1b[1C"); // move to the right one
							position++;
						}
						break;
					case 'D': // left arrow
						if (position > 0) {
							kputs("\x1b[1D"); // move to the left one
							position--;
						}
						break;
					case 'A': // up arrow
					case 'B': // down arrow
					{
						int32 historyLine = 0;

						if (c == 'A') {
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
						if (historyLine == sCurrentLine) {
							sLineBuffer[historyLine][0] = '\0';
						} else if (sLineBuffer[historyLine][0] == '\0') {
							// empty history lines are unused -- so bail out
							break;
						}

						// swap the current line with something from the history
						if (position > 0)
							kprintf("\x1b[%ldD", position); // move to beginning of line

						strcpy(buffer, sLineBuffer[historyLine]);
						length = position = strlen(buffer);
						kprintf("%s\x1b[K", buffer); // print the line and clear the rest
						currentHistoryLine = historyLine;
						break;
					}
					case '5':	// if "5~", it's PAGE UP
					case '6':	// if "6~", it's PAGE DOWN
					{
						if (readChar() != '~')
							break;

						// PAGE UP: search backward, PAGE DOWN: forward
						int32 searchDirection = (c == '5' ? -1 : 1);

						bool found = false;
						int32 historyLine = currentHistoryLine;
						do {
							historyLine = (historyLine + searchDirection
								+ HISTORY_SIZE) % HISTORY_SIZE;
							if (historyLine == sCurrentLine)
								break;

							if (strncmp(sLineBuffer[historyLine], buffer,
									position) == 0) {
								found = true;
							}
						} while (!found);

						// bail out, if we've found nothing or hit an empty
						// (i.e. unused) history line
						if (!found || strlen(sLineBuffer[historyLine]) == 0)
							break;

						// found a suitable line -- replace the current buffer
						// content with it
						strcpy(buffer, sLineBuffer[historyLine]);
						length = strlen(buffer);
						kprintf("%s\x1b[K", buffer + position);
							// print the line and clear the rest
						kprintf("\x1b[%ldD", length - position);
							// reposition cursor
						currentHistoryLine = historyLine;

						break;
					}
					case 'H': // home
					{
						if (position > 0) {
							kprintf("\x1b[%ldD", position);
							position = 0;
						}
						break;
					}
					case 'F': // end
					{
						if (position < length) {
							kprintf("\x1b[%ldC", length - position);
							position = length;
						}
						break;
					}
					case '3':	// if "3~", it's DEL
					{
						if (readChar() != '~')
							break;

						if (position < length)
							remove_char_from_line(buffer, position, length);

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
				if (isprint(c))
					insert_char_into_line(buffer, position, length, c);
				break;
		}

		if (length >= maxLength - 2) {
			buffer[length++] = '\0';
			kputchar('\n');
			done = true;
			break;
		}
	}

	return length;
}


int
kgets(char *buffer, int length)
{
	return read_line(buffer, length);
}


static void
kernel_debugger_loop(void)
{
	int32 previousCPU = sDebuggerOnCPU;
	sDebuggerOnCPU = smp_get_current_cpu();

	// set a few temporary debug variables
	if (struct thread* thread = thread_get_current_thread()) {
		set_debug_variable("_thread", (uint64)(addr_t)thread);
		set_debug_variable("_threadID", thread->id);
		set_debug_variable("_team", (uint64)(addr_t)thread->team);
		set_debug_variable("_teamID", thread->team->id);
		set_debug_variable("_cpu", sDebuggerOnCPU);
	}

	kprintf("Welcome to Kernel Debugging Land...\n");
	kprintf("Running on CPU %ld\n", sDebuggerOnCPU);

	int32 continuableLine = -1;
		// Index of the previous command line, if the command returned
		// B_KDEBUG_CONT, i.e. asked to be repeatable, -1 otherwise.

	for (;;) {
		CommandLineEditingHelper editingHelper;
		kprintf(kKDLPrompt);
		char* line = sLineBuffer[sCurrentLine];
		read_line(line, LINE_BUFFER_SIZE, &editingHelper);

		// check, if the line is empty or whitespace only
		bool whiteSpaceOnly = true;
		for (int i = 0 ; line[i] != '\0'; i++) {
			if (!isspace(line[i])) {
				whiteSpaceOnly = false;
				break;
			}
		}

		if (whiteSpaceOnly) {
			if (continuableLine < 0)
				continue;

			// the previous command can be repeated
			sCurrentLine = continuableLine;
			line = sLineBuffer[sCurrentLine];
		}

		int rc = evaluate_debug_command(line);

		if (rc == B_KDEBUG_QUIT)
			break;	// okay, exit now.

		// If the command is continuable, remember the current line index.
		continuableLine = (rc == B_KDEBUG_CONT ? sCurrentLine : -1);

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
cmd_dump_kdl_message(int argc, char **argv)
{
	if (sCurrentKernelDebuggerMessage) {
		kputs(sCurrentKernelDebuggerMessage);
		kputchar('\n');
	}

	return 0;
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
	kprintf("error 0x%lx: %s\n", error, strerror(error));

	return 0;
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
				if (length > (int32)SYSLOG_MAX_MESSAGE_LENGTH)
					length = SYSLOG_MAX_MESSAGE_LENGTH;

				length = ring_buffer_read(sSyslogBuffer,
					(uint8 *)sSyslogMessage->message, length);
				if (sSyslogDropped) {
					// add drop marker
					if (length < (int32)SYSLOG_MAX_MESSAGE_LENGTH - 6)
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

	if ((int32)ring_buffer_writable(sSyslogBuffer) < length) {
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
syslog_init(struct kernel_args *args)
{
	status_t status;

	if (!sSyslogOutputEnabled)
		return B_OK;

	sSyslogMessage = (syslog_message*)malloc(SYSLOG_MESSAGE_BUFFER_SIZE);
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

	if (args->debug_output != NULL)
		syslog_write((const char*)args->debug_output, args->debug_size);

	return B_OK;

err3:
	free(sSyslogBuffer);
err2:
	free(sSyslogMessage);
err1:
	sSyslogOutputEnabled = false;
	return status;
}


void
call_modules_hook(bool enter)
{
	uint32 index = 0;
	while (index < kMaxDebuggerModules && sDebuggerModules[index] != NULL) {
		debugger_module_info *module = sDebuggerModules[index];

		if (enter && module->enter_debugger != NULL)
			module->enter_debugger();
		else if (!enter && module->exit_debugger != NULL)
			module->exit_debugger();

		index++;
	}
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

	if (length > 1 && string[length - 1] == '\n'
		&& strncmp(string, sLastOutputBuffer, length) == 0) {
		sMessageRepeatCount++;
		sMessageRepeatLastTime = system_time();
		if (sMessageRepeatFirstTime == 0)
			sMessageRepeatFirstTime = sMessageRepeatLastTime;
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
	add_debugger_command_etc("message", &cmd_dump_kdl_message,
		"Reprint the message printed when entering KDL",
		"\n"
		"Reprints the message printed when entering KDL.\n", 0);
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

	debug_variables_init();
	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);
	tracing_init();

	// get debug settings
	void *handle = load_driver_settings("kernel");
	if (handle != NULL) {
		sSerialDebugEnabled = get_driver_boolean_parameter(handle,
			"serial_debug_output", sSerialDebugEnabled, sSerialDebugEnabled);
		sSyslogOutputEnabled = get_driver_boolean_parameter(handle,
			"syslog_debug_output", sSyslogOutputEnabled, sSyslogOutputEnabled);
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

	syslog_init(args);

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
	uint32 count = 0;
	while (count < kMaxDebuggerModules) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);

		if (read_next_module_name(cookie, name, &nameLength) != B_OK)
			break;

		if (get_module(name, (module_info **)&sDebuggerModules[count]) == B_OK) {
			dprintf("kernel debugger extension \"%s\": loaded\n", name);
			count++;
		} else
			dprintf("kernel debugger extension \"%s\": failed to load\n", name);
	}
	close_module_list(cookie);

	return frame_buffer_console_init_post_modules(args);
}


//	#pragma mark - public API


uint64
parse_expression(const char *expression)
{
	uint64 result;
	return (evaluate_debug_expression(expression, &result, true) ? result : 0);
}


void
panic(const char *format, ...)
{
	va_list args;
	char temp[128];

	va_start(args, format);
	vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	kernel_debugger(temp);
}


void
kernel_debugger(const char *message)
{
	static vint32 inDebugger = 0;
	cpu_status state;
	bool dprintfState;

	state = disable_interrupts();
	while (atomic_add(&inDebugger, 1) > 0) {
		// The debugger is already running, find out where...
		if (sDebuggerOnCPU != smp_get_current_cpu()) {
			// Some other CPU must have entered the debugger and tried to halt
			// us. Process ICIs to ensure we get the halt request. Then we are
			// blocking there until everyone leaves the debugger and we can
			// try to enter it again.
			atomic_add(&inDebugger, -1);
			smp_intercpu_int_handler();
		} else {
			// We are re-entering the debugger on the same CPU.
			break;
		}
	}

	arch_debug_save_registers(&dbg_register_file[smp_get_current_cpu()][0]);
	dprintfState = set_dprintf_enabled(true);

	if (sDebuggerOnCPU != smp_get_current_cpu() && smp_get_num_cpus() > 1) {
		// First entry on a MP system, send a halt request to all of the other
		// CPUs. Should they try to enter the debugger they will be cought in
		// the loop above.
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0,
			(void *)&inDebugger, SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenOutput) {
		if (blue_screen_enter(false) == B_OK)
			sBlueScreenEnabled = true;
	}

	if (message)
		kprintf("PANIC: %s\n", message);

	sCurrentKernelDebuggerMessage = message;

	// sort the commands
	sort_debugger_commands();

	call_modules_hook(true);

	kernel_debugger_loop();

	call_modules_hook(false);
	set_dprintf_enabled(dprintfState);

	sBlueScreenEnabled = false;
	atomic_add(&inDebugger, -1);
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

		sMessageRepeatFirstTime = 0;
		sMessageRepeatCount = 0;
	}
}


static void
check_pending_repeats(void *data, int iter)
{
	(void)data;
	(void)iter;
	if (sMessageRepeatCount > 0
		&& ((system_time() - sMessageRepeatLastTime) > 1000000
		|| (system_time() - sMessageRepeatFirstTime) > 3000000)) {
		cpu_status state = disable_interrupts();
		acquire_spinlock(&sSpinlock);

		flush_pending_repeats();

		release_spinlock(&sSpinlock);
		restore_interrupts(state);
	}
}


static void
dprintf_args(const char *format, va_list args, bool syslogOutput)
{
	cpu_status state;
	int32 length;

	// ToDo: maybe add a non-interrupt buffer and path that only
	//	needs to acquire a semaphore instead of needing to disable
	//	interrupts?

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	length = vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);

	if (length >= OUTPUT_BUFFER_SIZE)
		length = OUTPUT_BUFFER_SIZE - 1;

	if (length > 1 && sOutputBuffer[length - 1] == '\n'
		&& strncmp(sOutputBuffer, sLastOutputBuffer, length) == 0) {
		sMessageRepeatCount++;
		sMessageRepeatLastTime = system_time();
		if (sMessageRepeatFirstTime == 0)
			sMessageRepeatFirstTime = sMessageRepeatLastTime;
	} else {
		flush_pending_repeats();

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(sOutputBuffer);
		if (syslogOutput)
			syslog_write(sOutputBuffer, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(sOutputBuffer);

		memcpy(sLastOutputBuffer, sOutputBuffer, length);
		sLastOutputBuffer[length] = 0;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
dprintf(const char *format, ...)
{
	va_list args;

	if (!sSerialDebugEnabled && !sSyslogOutputEnabled && !sBlueScreenEnabled)
		return;

	va_start(args, format);
	dprintf_args(format, args, sSyslogOutputEnabled);
	va_end(args);
}


void
dprintf_no_syslog(const char *format, ...)
{
	va_list args;

	if (!sSerialDebugEnabled && !sBlueScreenEnabled)
		return;

	va_start(args, format);
	dprintf_args(format, args, false);
	va_end(args);
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

		dprintf("%s%04x ", prefix, i);
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
