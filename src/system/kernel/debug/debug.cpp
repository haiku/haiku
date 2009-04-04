/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*! This file contains the debugger and debug output facilities */

#include "blue_screen.h"

#include <cpu.h>
#include <debug.h>
#include <debug_paranoia.h>
#include <driver_settings.h>
#include <frame_buffer_console.h>
#include <int.h>
#include <kernel.h>
#include <ksystem_info.h>
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

#include "debug_builtin_commands.h"
#include "debug_commands.h"
#include "debug_output_filter.h"
#include "debug_variables.h"


static const char* const kKDLPrompt = "kdebug> ";

extern "C" int kgets(char* buffer, int length);

void call_modules_hook(bool enter);


int dbg_register_file[B_MAX_CPU_COUNT][14];
	/* XXXmpetit -- must be made generic */

static debug_page_fault_info sPageFaultInfo;

static bool sSerialDebugEnabled = true;
static bool sSyslogOutputEnabled = true;
static bool sBlueScreenEnabled = false;
	// must always be false on startup
static bool sDebugScreenEnabled = false;
static bool sBlueScreenOutput = true;
static spinlock sSpinlock = B_SPINLOCK_INITIALIZER;
static int32 sDebuggerOnCPU = -1;

static sem_id sSyslogNotify = -1;
static struct syslog_message* sSyslogMessage;
static struct ring_buffer* sSyslogBuffer;
static bool sSyslogDropped = false;

static const char* sCurrentKernelDebuggerMessage;

#define DEFAULT_SYSLOG_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];
static char sLastOutputBuffer[OUTPUT_BUFFER_SIZE];
static DebugOutputFilter* sDebugOutputFilter = NULL;
DefaultDebugOutputFilter gDefaultDebugOutputFilter;

static void flush_pending_repeats(bool syslogOutput);
static void check_pending_repeats(void* data, int iter);

static int64 sMessageRepeatFirstTime = 0;
static int64 sMessageRepeatLastTime = 0;
static int32 sMessageRepeatCount = 0;

static debugger_module_info* sDebuggerModules[8];
static const uint32 kMaxDebuggerModules = sizeof(sDebuggerModules)
	/ sizeof(sDebuggerModules[0]);

#define LINE_BUFFER_SIZE 1024
#define HISTORY_SIZE 16

static char sLineBuffer[HISTORY_SIZE][LINE_BUFFER_SIZE] = { "", };
static int32 sCurrentLine = 0;

static debugger_demangle_module_info* sDemangleModule;

static struct thread* sDebuggedThread;
static vint32 sInDebugger = 0;
static bool sPreviousDprintfState;
static volatile bool sHandOverKDL = false;
static vint32 sHandOverKDLToCPU = -1;

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))


// #pragma mark - DebugOutputFilter


DebugOutputFilter::DebugOutputFilter()
{
}


DebugOutputFilter::~DebugOutputFilter()
{
}


void
DebugOutputFilter::PrintString(const char* string)
{
}


void
DebugOutputFilter::Print(const char* format, va_list args)
{
}


void
DefaultDebugOutputFilter::PrintString(const char* string)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_puts(string);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_puts(string);

	for (uint32 i = 0; sSerialDebugEnabled && i < kMaxDebuggerModules; i++) {
		if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
			sDebuggerModules[i]->debugger_puts(string, strlen(string));
	}
}


void
DefaultDebugOutputFilter::Print(const char* format, va_list args)
{
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	flush_pending_repeats(sInDebugger == 0);
	PrintString(sOutputBuffer);
}


// #pragma mark -


DebugOutputFilter*
set_debug_output_filter(DebugOutputFilter* filter)
{
	DebugOutputFilter* oldFilter = sDebugOutputFilter;
	sDebugOutputFilter = filter;
	return oldFilter;
}


static void
kputchar(char c)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_putchar(c);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_putchar(c);
	for (uint32 i = 0; sSerialDebugEnabled && i < kMaxDebuggerModules; i++)
		if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
			sDebuggerModules[i]->debugger_puts(&c, sizeof(c));
}


void
kputs(const char* s)
{
	if (sDebugOutputFilter != NULL)
		sDebugOutputFilter->PrintString(s);
}


void
kputs_unfiltered(const char* s)
{
	gDefaultDebugOutputFilter.PrintString(s);
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
read_line(char* buffer, int32 maxLength,
	LineEditingHelper* editingHelper = NULL)
{
	int32 currentHistoryLine = sCurrentLine;
	int32 position = 0;
	int32 length = 0;
	bool done = false;
	char c = 0;

	while (!done) {
		c = kgetc();

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
			case 0x1f & 'L':	// CTRL-L -- clear screen
				if (sBlueScreenOutput) {
					// All the following needs to be transparent for the
					// serial debug output. I.e. after clearing the screen
					// we have to get the on-screen line into the visual state
					// it should have.

					// clear screen
					blue_screen_clear_screen();

					// reprint line
					buffer[length] = '\0';
					blue_screen_puts(kKDLPrompt);
					blue_screen_puts(buffer);

					// reposition cursor
					if (position < length) {
						for (int i = length; i > position; i--)
							blue_screen_puts("\x1b[1D");
					}
				}
				break;
			case 27: // escape sequence
				c = kgetc();
				if (c != '[') {
					// ignore broken escape sequence
					break;
				}
				c = kgetc();
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
						if (kgetc() != '~')
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
						if (kgetc() != '~')
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


char
kgetc(void)
{
	// give the kernel debugger modules a chance first
	for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
		if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_getchar) {
			int getChar = sDebuggerModules[i]->debugger_getchar();
			if (getChar >= 0)
				return (char)getChar;
		}
	}

	if (sBlueScreenOutput)
		return blue_screen_getchar();

	return arch_debug_serial_getchar();
}


int
kgets(char* buffer, int length)
{
	return read_line(buffer, length);
}


static void
kernel_debugger_loop(void)
{
	int32 previousCPU = sDebuggerOnCPU;
	sDebuggerOnCPU = smp_get_current_cpu();

	kprintf("Welcome to Kernel Debugging Land...\n");

	if (struct thread* thread = thread_get_current_thread()) {
		// set a few temporary debug variables
		set_debug_variable("_thread", (uint64)(addr_t)thread);
		set_debug_variable("_threadID", thread->id);
		set_debug_variable("_team", (uint64)(addr_t)thread->team);
		if (thread->team != NULL)
			set_debug_variable("_teamID", thread->team->id);
		set_debug_variable("_cpu", sDebuggerOnCPU);

		kprintf("Thread %ld \"%s\" running on CPU %ld\n", thread->id,
			thread->name, sDebuggerOnCPU);
	} else
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


static void
enter_kernel_debugger(const char* message)
{
	while (atomic_add(&sInDebugger, 1) > 0) {
		// The debugger is already running, find out where...
		if (sDebuggerOnCPU == smp_get_current_cpu()) {
			// We are re-entering the debugger on the same CPU.
			break;
		}

		// Some other CPU must have entered the debugger and tried to halt
		// us. Process ICIs to ensure we get the halt request. Then we are
		// blocking there until everyone leaves the debugger and we can
		// try to enter it again.
		atomic_add(&sInDebugger, -1);
		smp_intercpu_int_handler();
	}

	arch_debug_save_registers(&dbg_register_file[smp_get_current_cpu()][0]);
	sPreviousDprintfState = set_dprintf_enabled(true);

	if (!gKernelStartup && sDebuggerOnCPU != smp_get_current_cpu()
		&& smp_get_num_cpus() > 1) {
		// First entry on a MP system, send a halt request to all of the other
		// CPUs. Should they try to enter the debugger they will be cought in
		// the loop above.
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL,
			SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenOutput) {
		if (blue_screen_enter(false) == B_OK)
			sBlueScreenEnabled = true;
	}

	sDebugOutputFilter = &gDefaultDebugOutputFilter;

	sDebuggedThread = NULL;

	if (message)
		kprintf("PANIC: %s\n", message);

	sCurrentKernelDebuggerMessage = message;

	// sort the commands
	sort_debugger_commands();

	call_modules_hook(true);
}


static void
exit_kernel_debugger()
{
	call_modules_hook(false);
	set_dprintf_enabled(sPreviousDprintfState);

	sDebugOutputFilter = NULL;

	sBlueScreenEnabled = false;
	if (sDebugScreenEnabled)
		blue_screen_enter(true);

	atomic_add(&sInDebugger, -1);
}


static void
hand_over_kernel_debugger()
{
	// Wait until the hand-over is complete.
	// The other CPU gets our sInDebugger reference and will release it when
	// done. Note, that there's a small race condition: the other CPU could
	// hand over to another CPU without us noticing. Since this is only
	// initiated by the user, it is harmless, though.
	sHandOverKDL = true;
	while (sHandOverKDLToCPU >= 0)
		PAUSE();
}


static int
cmd_dump_kdl_message(int argc, char** argv)
{
	if (sCurrentKernelDebuggerMessage) {
		kputs(sCurrentKernelDebuggerMessage);
		kputs("\n");
	}

	return 0;
}


static int
cmd_dump_syslog(int argc, char** argv)
{
	if (!sSyslogOutputEnabled) {
		kprintf("Syslog is not enabled.\n");
		return 0;
	}

	bool currentOnly = false;
	if (argc > 1) {
		if (!strcmp(argv[1], "-n"))
			currentOnly = true;
		else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	uint32 start = sSyslogBuffer->first;
	size_t end = start + sSyslogBuffer->in;
	if (!currentOnly) {
		// Start the buffer after the current end (we don't really know if
		// this part has been written to already).
		start = (start + sSyslogBuffer->in) % sSyslogBuffer->size;
		end = start + sSyslogBuffer->size;
	} else if (!ring_buffer_readable(sSyslogBuffer)) {
		kprintf("Syslog is empty.\n");
		return 0;
	}

	// break it down to lines to make it grep'able

	bool newLine = false;
	char line[256];
	size_t linePos = 0;
	for (uint32 i = start; i < end; i++) {
		char c = sSyslogBuffer->buffer[i % sSyslogBuffer->size];
		if (c == '\0' || (uint8)c == 0xcc)
			continue;

		line[linePos++] = c;
		newLine = false;

		if (c == '\n' || linePos == sizeof(line) - 1) {
			newLine = c == '\n';
			line[linePos] = '\0';
			linePos = 0;
			kprintf(line);
		}
	}
	if (!newLine)
		kprintf("\n");

	return 0;
}


static int
cmd_switch_cpu(int argc, char** argv)
{
	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc == 1) {
		kprintf("running on CPU %ld\n", smp_get_current_cpu());
		return 0;
	}

	int32 newCPU = parse_expression(argv[1]);
	if (newCPU < 0 || newCPU >= smp_get_num_cpus()) {
		kprintf("invalid CPU index\n");
		return 0;
	}

	if (newCPU == smp_get_current_cpu()) {
		kprintf("already running on CPU %ld\n", newCPU);
		return 0;
	}

	sHandOverKDLToCPU = newCPU;

	return B_KDEBUG_QUIT;
}


static status_t
syslog_sender(void* data)
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
					(uint8*)sSyslogMessage->message, length);
				if (sSyslogDropped) {
					// Add drop marker - since parts had to be dropped, it's
					// guaranteed that we have enough space in the buffer now.
					ring_buffer_write(sSyslogBuffer, (uint8*)"<DROP>", 6);
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
syslog_write(const char* text, int32 length)
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

	ring_buffer_write(sSyslogBuffer, (uint8*)text, length);
	if (trunc)
		ring_buffer_write(sSyslogBuffer, (uint8*)"<TRUNC>", 7);

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
syslog_init(struct kernel_args* args)
{
	status_t status;
	int32 length = 0;

	if (!sSyslogOutputEnabled)
		return B_OK;

	sSyslogMessage = (syslog_message*)malloc(SYSLOG_MESSAGE_BUFFER_SIZE);
	if (sSyslogMessage == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	{
		size_t bufferSize = DEFAULT_SYSLOG_BUFFER_SIZE;
		void* handle = load_driver_settings("kernel");
		if (handle != NULL) {
			const char* sizeString = get_driver_parameter(handle,
				"syslog_buffer_size", NULL, NULL);
			if (sizeString != NULL) {
				bufferSize = strtoul(sizeString, NULL, 0);
				if (bufferSize > 262144)
					bufferSize = 262144;
				else if (bufferSize < SYSLOG_MESSAGE_BUFFER_SIZE)
					bufferSize = SYSLOG_MESSAGE_BUFFER_SIZE;
			}

			unload_driver_settings(handle);
		}

		sSyslogBuffer = create_ring_buffer(bufferSize);
	}
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

	char revisionBuffer[64];
	length = snprintf(revisionBuffer, sizeof(revisionBuffer),
		"Welcome to syslog debug output!\nHaiku revision: %lu\n",
		get_haiku_revision());
	syslog_write(revisionBuffer, length);

	add_debugger_command_etc("syslog", &cmd_dump_syslog,
		"Dumps the syslog buffer.\n",
		"[-n]\nDumps the whole syslog buffer, or, if -n is specified, only "
		"the part that hasn't been sent yet.\n", 0);

	return B_OK;

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
		debugger_module_info* module = sDebuggerModules[index];

		if (enter && module->enter_debugger != NULL)
			module->enter_debugger();
		else if (!enter && module->exit_debugger != NULL)
			module->exit_debugger();

		index++;
	}
}


//	#pragma mark - private kernel API


bool
debug_screen_output_enabled(void)
{
	return sDebugScreenEnabled;
}


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
debug_puts(const char* string, int32 length)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	if (length >= OUTPUT_BUFFER_SIZE)
		length = OUTPUT_BUFFER_SIZE - 1;

	// TODO: Code duplication! Cf. dprintf_args()!
	if (length > 1 && string[length - 1] == '\n'
		&& strncmp(string, sLastOutputBuffer, length) == 0) {
		sMessageRepeatCount++;
		sMessageRepeatLastTime = system_time();
		if (sMessageRepeatFirstTime == 0)
			sMessageRepeatFirstTime = sMessageRepeatLastTime;
	} else {
		flush_pending_repeats(true);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(string);
		if (sSyslogOutputEnabled)
			syslog_write(string, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(string);
		if (sSerialDebugEnabled) {
			for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
				if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
					sDebuggerModules[i]->debugger_puts(string, length);
			}
		}

		memcpy(sLastOutputBuffer, string, length);
		sLastOutputBuffer[length] = 0;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
debug_early_boot_message(const char* string)
{
	arch_debug_serial_early_boot_message(string);
}


status_t
debug_init(kernel_args* args)
{
	new(&gDefaultDebugOutputFilter) DefaultDebugOutputFilter;

	debug_paranoia_init();
	return arch_debug_console_init(args);
}


status_t
debug_init_post_vm(kernel_args* args)
{
	add_debugger_command_etc("cpu", &cmd_switch_cpu,
		"Switches to another CPU.\n",
		"<cpu>\n"
		"Switches to CPU with the index <cpu>.\n", 0);
	add_debugger_command_etc("message", &cmd_dump_kdl_message,
		"Reprint the message printed when entering KDL",
		"\n"
		"Reprints the message printed when entering KDL.\n", 0);

	debug_builtin_commands_init();

	debug_variables_init();
	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);
	tracing_init();

	// get debug settings
	void* handle = load_driver_settings("kernel");
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
debug_init_post_modules(struct kernel_args* args)
{
	void* cookie;

	// check for dupped lines every 10/10 second
	register_kernel_daemon(check_pending_repeats, NULL, 10);

	syslog_init_post_threads();

	// load kernel debugger addons

	static const char* kDemanglePrefix = "debugger/demangle/";

	cookie = open_module_list("debugger");
	uint32 count = 0;
	while (count < kMaxDebuggerModules) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);

		if (read_next_module_name(cookie, name, &nameLength) != B_OK)
			break;

		// get demangle module, if any
		if (!strncmp(name, kDemanglePrefix, strlen(kDemanglePrefix))) {
			if (sDemangleModule == NULL)
				get_module(name, (module_info**)&sDemangleModule);
			continue;
		}

		if (get_module(name, (module_info**)&sDebuggerModules[count]) == B_OK) {
			dprintf("kernel debugger extension \"%s\": loaded\n", name);
			count++;
		} else
			dprintf("kernel debugger extension \"%s\": failed to load\n", name);
	}
	close_module_list(cookie);

	return frame_buffer_console_init_post_modules(args);
}


void
debug_set_page_fault_info(addr_t faultAddress, addr_t pc, uint32 flags)
{
	sPageFaultInfo.fault_address = faultAddress;
	sPageFaultInfo.pc = pc;
	sPageFaultInfo.flags = flags;
}


debug_page_fault_info*
debug_get_page_fault_info()
{
	return &sPageFaultInfo;
}


void
debug_trap_cpu_in_kdl(bool returnIfHandedOver)
{
	cpu_status state = disable_interrupts();

	while (sInDebugger != 0) {
		if (sHandOverKDL && sHandOverKDLToCPU == smp_get_current_cpu()) {
			if (returnIfHandedOver)
				return;

			kernel_debugger(NULL);
		} else
			PAUSE();
	}

	restore_interrupts(state);
}


//	#pragma mark - public API


uint64
parse_expression(const char* expression)
{
	uint64 result;
	return (evaluate_debug_expression(expression, &result, true) ? result : 0);
}


void
panic(const char* format, ...)
{
	va_list args;
	char temp[128];

	va_start(args, format);
	vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	kernel_debugger(temp);
}


void
kernel_debugger(const char* message)
{
	cpu_status state = disable_interrupts();

	while (true) {
		if (sHandOverKDLToCPU == smp_get_current_cpu()) {
			sHandOverKDLToCPU = -1;
			sHandOverKDL = false;
		} else
			enter_kernel_debugger(message);

		kernel_debugger_loop();

		if (sHandOverKDLToCPU < 0) {
			exit_kernel_debugger();
			break;
		}

		hand_over_kernel_debugger();

		debug_trap_cpu_in_kdl(true);

		if (sHandOverKDLToCPU != smp_get_current_cpu())
			break;
	}

	restore_interrupts(state);
}


bool
set_dprintf_enabled(bool newState)
{
	bool oldState = sSerialDebugEnabled;
	sSerialDebugEnabled = newState;

	return oldState;
}


//!	Must be called with the sSpinlock held.
static void
flush_pending_repeats(bool syslogOutput)
{
	if (sMessageRepeatCount <= 0)
		return;

	if (sMessageRepeatCount > 1) {
		static char temp[40];
		size_t length = snprintf(temp, sizeof(temp),
			"Last message repeated %ld times.\n", sMessageRepeatCount);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(temp);
		if (sSyslogOutputEnabled && syslogOutput)
			syslog_write(temp, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(temp);
		if (sSerialDebugEnabled) {
			for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
				if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
					sDebuggerModules[i]->debugger_puts(temp, length);
			}
		}
	} else {
		// if we only have one repeat just reprint the last buffer
		size_t length = strlen(sLastOutputBuffer);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(sLastOutputBuffer);
		if (sSyslogOutputEnabled && syslogOutput)
			syslog_write(sLastOutputBuffer, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(sLastOutputBuffer);
		if (sSerialDebugEnabled) {
			for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
				if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts) {
					sDebuggerModules[i]->debugger_puts(sLastOutputBuffer,
						length);
				}
			}
		}
	}

	sMessageRepeatFirstTime = 0;
	sMessageRepeatCount = 0;
}


static void
check_pending_repeats(void* /*data*/, int /*iteration*/)
{
	if (sMessageRepeatCount > 0
		&& (system_time() - sMessageRepeatLastTime > 1000000
			|| system_time() - sMessageRepeatFirstTime > 3000000)) {
		cpu_status state = disable_interrupts();
		acquire_spinlock(&sSpinlock);

		flush_pending_repeats(true);

		release_spinlock(&sSpinlock);
		restore_interrupts(state);
	}
}


static void
dprintf_args(const char* format, va_list args, bool syslogOutput)
{
	cpu_status state;
	int32 length;
	uint32 i;

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
		flush_pending_repeats(syslogOutput);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(sOutputBuffer);
		if (syslogOutput)
			syslog_write(sOutputBuffer, length);
		if (sBlueScreenEnabled || sDebugScreenEnabled)
			blue_screen_puts(sOutputBuffer);
		for (i = 0; sSerialDebugEnabled && i < kMaxDebuggerModules; i++) {
			if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
				sDebuggerModules[i]->debugger_puts(sOutputBuffer, length);
		}

		memcpy(sLastOutputBuffer, sOutputBuffer, length);
		sLastOutputBuffer[length] = 0;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
dprintf(const char* format, ...)
{
	va_list args;

	if (!sSerialDebugEnabled && !sSyslogOutputEnabled && !sBlueScreenEnabled)
		return;

	va_start(args, format);
	dprintf_args(format, args, sSyslogOutputEnabled);
	va_end(args);
}


void
dprintf_no_syslog(const char* format, ...)
{
	va_list args;

	if (!sSerialDebugEnabled && !sBlueScreenEnabled)
		return;

	va_start(args, format);
	dprintf_args(format, args, false);
	va_end(args);
}


/*!	Similar to dprintf() but thought to be used in the kernel
	debugger only (it doesn't lock).
*/
void
kprintf(const char* format, ...)
{
	if (sDebugOutputFilter != NULL) {
		va_list args;
		va_start(args, format);
		sDebugOutputFilter->Print(format, args);
		va_end(args);
	}
}


void
kprintf_unfiltered(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	gDefaultDebugOutputFilter.Print(format, args);
	va_end(args);
}


const char*
debug_demangle_symbol(const char* symbol, char* buffer, size_t bufferSize,
	bool* _isObjectMethod)
{
	if (sDemangleModule != NULL && sDemangleModule->demangle_symbol != NULL) {
		return sDemangleModule->demangle_symbol(symbol, buffer, bufferSize,
			_isObjectMethod);
	}

	if (_isObjectMethod != NULL)
		*_isObjectMethod = false;

	return symbol;
}


status_t
debug_get_next_demangled_argument(uint32* _cookie, const char* symbol,
	char* name, size_t nameSize, int32* _type, size_t* _argumentLength)
{
	if (sDemangleModule != NULL && sDemangleModule->get_next_argument != NULL) {
		return sDemangleModule->get_next_argument(_cookie, symbol, name,
			nameSize, _type, _argumentLength);
	}

	return B_NOT_SUPPORTED;
}


struct thread*
debug_set_debugged_thread(struct thread* thread)
{
	struct thread* previous = sDebuggedThread;
	sDebuggedThread = thread;
	return previous;
}


struct thread*
debug_get_debugged_thread()
{
	return sDebuggedThread != NULL
		? sDebuggedThread : thread_get_current_thread();
}


//	#pragma mark -
//	userland syscalls


void
_user_debug_output(const char* userString)
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
dump_block(const char* buffer, int size, const char* prefix)
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
				dprintf("%02x", *(unsigned char*)(buffer + i));
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
