/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! This file contains the debugger and debug output facilities */


#include "blue_screen.h"

#include <algorithm>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <debug.h>
#include <debug_heap.h>
#include <debug_paranoia.h>
#include <driver_settings.h>
#include <frame_buffer_console.h>
#include <int.h>
#include <kernel.h>
#include <ksystem_info.h>
#include <safemode.h>
#include <smp.h>
#include <thread.h>
#include <tracing.h>
#include <vm/vm.h>
#include <vm/VMTranslationMap.h>

#include <arch/debug_console.h>
#include <arch/debug.h>
#include <util/AutoLock.h>
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


#if __GNUC__ == 2
#	define va_copy(to, from)	__va_copy(to, from)
#endif


struct debug_memcpy_parameters {
	void*		to;
	const void*	from;
	size_t		size;
};

struct debug_strlcpy_parameters {
	char*		to;
	const char*	from;
	size_t		size;
	size_t		result;
};


static const char* const kKDLPrompt = "kdebug> ";
static const char* const kKDLMessageCommandSeparator = "@!";
	// separates panic() message from command list to execute

extern "C" int kgets(char* buffer, int length);

void call_modules_hook(bool enter);

static void syslog_write(const char* text, int32 length, bool notify);

static arch_debug_registers sDebugRegisters[B_MAX_CPU_COUNT];

static debug_page_fault_info sPageFaultInfo;

static bool sSerialDebugEnabled = true;
static bool sSyslogOutputEnabled = true;
static bool sBlueScreenEnabled = false;
	// must always be false on startup
static bool sDebugScreenEnabled = false;
static bool sBlueScreenOutput = true;
static bool sEmergencyKeysEnabled = true;
static spinlock sSpinlock = B_SPINLOCK_INITIALIZER;
static int32 sDebuggerOnCPU = -1;

static sem_id sSyslogNotify = -1;
static struct syslog_message* sSyslogMessage;
static struct ring_buffer* sSyslogBuffer;
static size_t sSyslogBufferOffset = 0;
	// (relative) buffer offset of the yet unsent syslog messages
static bool sSyslogDropped = false;
static bool sDebugSyslog = false;
static size_t sSyslogDebuggerOffset = 0;
	// (relative) buffer offset of the kernel debugger messages of the current
	// KDL session

static const char* sCurrentKernelDebuggerMessagePrefix;
static const char* sCurrentKernelDebuggerMessage;
static va_list sCurrentKernelDebuggerMessageArgs;

#define DEFAULT_SYSLOG_BUFFER_SIZE 65536
#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];
static char sInterruptOutputBuffer[OUTPUT_BUFFER_SIZE];
static char sLastOutputBuffer[OUTPUT_BUFFER_SIZE];
static DebugOutputFilter* sDebugOutputFilter = NULL;
DefaultDebugOutputFilter gDefaultDebugOutputFilter;
static mutex sOutputLock = MUTEX_INITIALIZER("debug output");

static void flush_pending_repeats(bool notifySyslog);
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

static Thread* sDebuggedThread;
static vint32 sInDebugger = 0;
static bool sPreviousDprintfState;
static volatile bool sHandOverKDL = false;
static vint32 sHandOverKDLToCPU = -1;
static bool sCPUTrapped[B_MAX_CPU_COUNT];


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
	size_t length = strlen(string);

	if (sSerialDebugEnabled)
		arch_debug_serial_puts(string);
	if (sSyslogOutputEnabled)
		syslog_write(string, length, false);
	if (sBlueScreenEnabled || sDebugScreenEnabled)
		blue_screen_puts(string);

	for (uint32 i = 0; sSerialDebugEnabled && i < kMaxDebuggerModules; i++) {
		if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_puts)
			sDebuggerModules[i]->debugger_puts(string, length);
	}
}


void
DefaultDebugOutputFilter::Print(const char* format, va_list args)
{
	vsnprintf(sInterruptOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	flush_pending_repeats(false);
	PrintString(sInterruptOutputBuffer);
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
		kprintf("\x1b[%" B_PRId32 "D", length - position);
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
	kprintf("\x1b[%" B_PRId32 "D", length - position + 1);
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
				kprintf("\x1b[%" B_PRId32 "D", length - position);
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
			case 8:		// backspace (CTRL-H)
			case 0x7f:	// backspace (xterm)
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
					kprintf("\x1b[%" B_PRId32 "D", length - position);

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
							kprintf("\x1b[%" B_PRId32 "D", position); // move to beginning of line

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
						kprintf("\x1b[%" B_PRId32 "D", length - position);
							// reposition cursor
						currentHistoryLine = historyLine;

						break;
					}
					case 'H': // home
					{
						if (position > 0) {
							kprintf("\x1b[%" B_PRId32 "D", position);
							position = 0;
						}
						break;
					}
					case 'F': // end
					{
						if (position < length) {
							kprintf("\x1b[%" B_PRId32 "C", length - position);
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
	while (true) {
		// check serial input
		int c = arch_debug_serial_try_getchar();
		if (c >= 0)
			return (char)c;

		// check blue screen input
		if (sBlueScreenOutput) {
			c = blue_screen_try_getchar();
			if (c >= 0)
				return (char)c;
		}

		// give the kernel debugger modules a chance
		for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
			if (sDebuggerModules[i] && sDebuggerModules[i]->debugger_getchar) {
				int getChar = sDebuggerModules[i]->debugger_getchar();
				if (getChar >= 0)
					return (char)getChar;
			}
		}

		PAUSE();
	}
}


int
kgets(char* buffer, int length)
{
	return read_line(buffer, length);
}


static void
print_kernel_debugger_message()
{
	if (sCurrentKernelDebuggerMessagePrefix != NULL
			|| sCurrentKernelDebuggerMessage != NULL) {
		if (sCurrentKernelDebuggerMessagePrefix != NULL)
			kprintf("%s", sCurrentKernelDebuggerMessagePrefix);
		if (sCurrentKernelDebuggerMessage != NULL
				&& sDebugOutputFilter != NULL) {
			va_list args;
			va_copy(args, sCurrentKernelDebuggerMessageArgs);

			if (const char* commandDelimiter = strstr(
					sCurrentKernelDebuggerMessage,
					kKDLMessageCommandSeparator)) {
				// The message string contains a list of commands to be
				// executed when entering the kernel debugger. We don't
				// want to print those, so we copy the interesting part of
				// the format string.
				if (commandDelimiter != sCurrentKernelDebuggerMessage) {
					size_t length = commandDelimiter
						- sCurrentKernelDebuggerMessage;
					if (char* format = (char*)debug_malloc(length + 1)) {
						memcpy(format, sCurrentKernelDebuggerMessage, length);
						format[length] = '\0';
						sDebugOutputFilter->Print(format, args);
						debug_free(format);
					} else {
						// allocation failed -- just print everything
						sDebugOutputFilter->Print(sCurrentKernelDebuggerMessage,
							args);
					}
				}
			} else
				sDebugOutputFilter->Print(sCurrentKernelDebuggerMessage, args);

			va_end(args);
		}

		kprintf("\n");
	}
}


static void
execute_panic_commands()
{
	if (sCurrentKernelDebuggerMessage == NULL
		|| strstr(sCurrentKernelDebuggerMessage,
				kKDLMessageCommandSeparator) == NULL) {
		return;
	}

	// Indeed there are commands to execute.
	const size_t kCommandBufferSize = 512;
	char* commandBuffer = (char*)debug_malloc(kCommandBufferSize);
	if (commandBuffer != NULL) {
		va_list tempArgs;
		va_copy(tempArgs, sCurrentKernelDebuggerMessageArgs);

		if (vsnprintf(commandBuffer, kCommandBufferSize,
				sCurrentKernelDebuggerMessage, tempArgs)
					< (int)kCommandBufferSize) {
			const char* commands = strstr(commandBuffer,
				kKDLMessageCommandSeparator);
			if (commands != NULL) {
				commands += strlen(kKDLMessageCommandSeparator);
				kprintf("initial commands: %s\n", commands);
				evaluate_debug_command(commands);
			}
		}

		va_end(tempArgs);

		debug_free(commandBuffer);
	}
}


static void
stack_trace_trampoline(void*)
{
	arch_debug_stack_trace();
}


static void
kernel_debugger_loop(const char* messagePrefix, const char* message,
	va_list args, int32 cpu)
{
	DebugAllocPool* allocPool = create_debug_alloc_pool();

	sCurrentKernelDebuggerMessagePrefix = messagePrefix;
	sCurrentKernelDebuggerMessage = message;
	if (sCurrentKernelDebuggerMessage != NULL)
		va_copy(sCurrentKernelDebuggerMessageArgs, args);

	sSyslogDebuggerOffset = sSyslogBuffer != NULL
		? ring_buffer_readable(sSyslogBuffer) : 0;

	print_kernel_debugger_message();

	kprintf("Welcome to Kernel Debugging Land...\n");

	// Set a few temporary debug variables and print on which CPU and in which
	// thread we are running.
	set_debug_variable("_cpu", sDebuggerOnCPU);

	Thread* thread = thread_get_current_thread();
	if (thread == NULL) {
		kprintf("Running on CPU %" B_PRId32 "\n", sDebuggerOnCPU);
	} else if (!debug_is_kernel_memory_accessible((addr_t)thread,
			sizeof(Thread), B_KERNEL_READ_AREA)) {
		kprintf("Running on CPU %" B_PRId32 "\n", sDebuggerOnCPU);
		kprintf("Current thread pointer is %p, which is an address we "
			"can't read from.\n", thread);
		arch_debug_unset_current_thread();
	} else {
		set_debug_variable("_thread", (uint64)(addr_t)thread);
		set_debug_variable("_threadID", thread->id);

		kprintf("Thread %" B_PRId32 " \"%.64s\" running on CPU %" B_PRId32 "\n",
			thread->id, thread->name, sDebuggerOnCPU);

		if (thread->cpu != gCPU + cpu) {
			kprintf("The thread's CPU pointer is %p, but should be %p.\n",
				thread->cpu, gCPU + cpu);
			arch_debug_unset_current_thread();
		} else if (thread->team != NULL) {
			if (debug_is_kernel_memory_accessible((addr_t)thread->team,
					sizeof(Team), B_KERNEL_READ_AREA)) {
				set_debug_variable("_team", (uint64)(addr_t)thread->team);
				set_debug_variable("_teamID", thread->team->id);
			} else {
				kprintf("The thread's team pointer is %p, which is an "
					"address we can't read from.\n", thread->team);
				arch_debug_unset_current_thread();
			}
		}
	}

	if (!has_debugger_command("help") || message != NULL) {
		// No commands yet or we came here via a panic(). Always print a stack
		// trace in these cases.
		jmp_buf* jumpBuffer = (jmp_buf*)debug_malloc(sizeof(jmp_buf));
		if (jumpBuffer != NULL) {
			debug_call_with_fault_handler(*jumpBuffer, &stack_trace_trampoline,
				NULL);
			debug_free(jumpBuffer);
		} else
			arch_debug_stack_trace();
	}

	if (has_debugger_command("help")) {
		// Commands are registered already -- execute panic() commands. Do that
		// with paging disabled, so everything is printed, even if the user
		// can't use the keyboard.
		bool pagingEnabled = blue_screen_paging_enabled();
		blue_screen_set_paging(false);

		execute_panic_commands();

		blue_screen_set_paging(pagingEnabled);
	}

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

		if (rc == B_KDEBUG_QUIT) {
			// okay, exit now.
			break;
		}

		// If the command is continuable, remember the current line index.
		continuableLine = (rc == B_KDEBUG_CONT ? sCurrentLine : -1);

		int previousLine = sCurrentLine - 1;
		if (previousLine < 0)
			previousLine = HISTORY_SIZE - 1;

		// Only use the next slot in the history, if the entries differ
		if (strcmp(sLineBuffer[sCurrentLine], sLineBuffer[previousLine])) {
			if (++sCurrentLine >= HISTORY_SIZE)
				sCurrentLine = 0;
		}
	}

	if (sCurrentKernelDebuggerMessage != NULL)
		va_end(sCurrentKernelDebuggerMessageArgs);

	delete_debug_alloc_pool(allocPool);
}


static void
enter_kernel_debugger(int32 cpu)
{
	while (atomic_add(&sInDebugger, 1) > 0) {
		atomic_add(&sInDebugger, -1);

		// The debugger is already running, find out where...
		if (sDebuggerOnCPU == cpu) {
			// We are re-entering the debugger on the same CPU.
			break;
		}

		// Some other CPU must have entered the debugger and tried to halt
		// us. Process ICIs to ensure we get the halt request. Then we are
		// blocking there until everyone leaves the debugger and we can
		// try to enter it again.
		smp_intercpu_int_handler(cpu);
	}

	arch_debug_save_registers(&sDebugRegisters[cpu]);
	sPreviousDprintfState = set_dprintf_enabled(true);

	if (!gKernelStartup && sDebuggerOnCPU != cpu && smp_get_num_cpus() > 1) {
		// First entry on a MP system, send a halt request to all of the other
		// CPUs. Should they try to enter the debugger they will be cought in
		// the loop above.
		smp_send_broadcast_ici_interrupts_disabled(cpu, SMP_MSG_CPU_HALT, 0, 0,
			0, NULL, SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenOutput) {
		if (blue_screen_enter(false) == B_OK)
			sBlueScreenEnabled = true;
	}

	sDebugOutputFilter = &gDefaultDebugOutputFilter;

	sDebuggedThread = NULL;

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


static void
kernel_debugger_internal(const char* messagePrefix, const char* message,
	va_list args, int32 cpu)
{
	while (true) {
		if (sHandOverKDLToCPU == cpu) {
			sHandOverKDLToCPU = -1;
			sHandOverKDL = false;
		} else
			enter_kernel_debugger(cpu);

		// If we're called recursively sDebuggerOnCPU will be != -1.
		int32 previousCPU = sDebuggerOnCPU;
		sDebuggerOnCPU = cpu;

		kernel_debugger_loop(messagePrefix, message, args, cpu);

		if (sHandOverKDLToCPU < 0 && previousCPU == -1) {
			// We're not handing over to a different CPU and we weren't
			// called recursively, so we'll exit the debugger.
			exit_kernel_debugger();
		}

		sDebuggerOnCPU = previousCPU;

		if (sHandOverKDLToCPU < 0)
			break;

		hand_over_kernel_debugger();

		debug_trap_cpu_in_kdl(cpu, true);

		if (sHandOverKDLToCPU != cpu)
			break;
	}
}


static int
cmd_dump_kdl_message(int argc, char** argv)
{
	print_kernel_debugger_message();
	return 0;
}


static int
cmd_execute_panic_commands(int argc, char** argv)
{
	execute_panic_commands();
	return 0;
}


static int
cmd_dump_syslog(int argc, char** argv)
{
	if (!sSyslogOutputEnabled) {
		kprintf("Syslog is not enabled.\n");
		return 0;
	}

	bool unsentOnly = false;
	bool ignoreKDLOutput = true;

	int argi = 1;
	for (; argi < argc; argi++) {
		if (strcmp(argv[argi], "-n") == 0)
			unsentOnly = true;
		else if (strcmp(argv[argi], "-k") == 0)
			ignoreKDLOutput = false;
		else
			break;
	}

	if (argi < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	size_t debuggerOffset = sSyslogDebuggerOffset;
	size_t start = unsentOnly ? sSyslogBufferOffset : 0;
	size_t end = ignoreKDLOutput
		? debuggerOffset : ring_buffer_readable(sSyslogBuffer);

	// allocate a buffer for processing the syslog output
	size_t bufferSize = 1024;
	char* buffer = (char*)debug_malloc(bufferSize);
	char stackBuffer[64];
	if (buffer == NULL) {
		buffer = stackBuffer;
		bufferSize = sizeof(stackBuffer);
	}

	// filter the output
	bool newLine = false;
	while (start < end) {
		size_t bytesRead = ring_buffer_peek(sSyslogBuffer, start, buffer,
			std::min(end - start, bufferSize - 1));
		if (bytesRead == 0)
			break;
		start += bytesRead;

		// remove '\0' and 0xcc
		size_t toPrint = 0;
		for (size_t i = 0; i < bytesRead; i++) {
			if (buffer[i] != '\0' && (uint8)buffer[i] != 0xcc)
				buffer[toPrint++] = buffer[i];
		}

		if (toPrint > 0) {
			newLine = buffer[toPrint - 1] == '\n';
			buffer[toPrint] = '\0';
			kputs(buffer);
		}

		if (debuggerOffset > sSyslogDebuggerOffset) {
			// Our output caused older syslog output to be evicted from the
			// syslog buffer. We need to adjust our offsets accordingly. Note,
			// this can still go wrong, if the buffer was already full and more
			// was written to it than we have processed, but we can't help that.
			size_t diff = debuggerOffset - sSyslogDebuggerOffset;
			start -= std::min(start, diff);
			end -= std::min(end, diff);
			debuggerOffset = sSyslogDebuggerOffset;
		}
	}

	if (!newLine)
		kputs("\n");

	if (buffer != stackBuffer)
		debug_free(buffer);

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
		kprintf("running on CPU %" B_PRId32 "\n", smp_get_current_cpu());
		return 0;
	}

	int32 newCPU = parse_expression(argv[1]);
	if (newCPU < 0 || newCPU >= smp_get_num_cpus()) {
		kprintf("invalid CPU index\n");
		return 0;
	}

	if (newCPU == smp_get_current_cpu()) {
		kprintf("already running on CPU %" B_PRId32 "\n", newCPU);
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
		acquire_sem_etc(sSyslogNotify, 1, B_RELATIVE_TIMEOUT, 5000000);
			// Note: We time out since in some situations output is added to
			// the syslog buffer without being allowed to notify us (e.g. in
			// the kernel debugger).
			// TODO: A semaphore is rather unhandy here. It is released for
			// every single message written to the buffer, but we potentially
			// send a lot more than a single message per iteration. On the other
			// hand, as long as the syslog daemon is not running, we acquire
			// the semaphore anyway. A better solution would be a flag + a
			// condition variable.

		sSyslogMessage->when = real_time_clock();

		if (error == B_BAD_PORT_ID) {
			// last message couldn't be sent, try to locate the syslog_daemon
			port = find_port(SYSLOG_PORT_NAME);
			if (port < 0) {
				// Don't recheck too quickly, since find_port) is rather
				// expensive.
				// TODO: Maybe using the port notification mechanism would be
				// the better option here. Alternatively, and probably even
				// better, the syslog daemon could register itself via a syscall
				// (like the messaging service). We could even wait with
				// starting this thread before that happened (end exit as soon
				// as the port is gone).
				snooze(1000000);
				continue;
			}
		}

		if (port >= B_OK) {
			if (!bufferPending) {
				// we need to have exclusive access to our syslog buffer
				cpu_status state = disable_interrupts();
				acquire_spinlock(&sSpinlock);

				length = ring_buffer_readable(sSyslogBuffer)
					- sSyslogBufferOffset;
				if (length > (int32)SYSLOG_MAX_MESSAGE_LENGTH)
					length = SYSLOG_MAX_MESSAGE_LENGTH;

				length = ring_buffer_peek(sSyslogBuffer, sSyslogBufferOffset,
					(uint8*)sSyslogMessage->message, length);
				sSyslogBufferOffset += length;
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
syslog_write(const char* text, int32 length, bool notify)
{
	if (sSyslogBuffer == NULL)
		return;

	if (length > sSyslogBuffer->size) {
		text = "<DROP>";
		length = 6;
	}

	int32 writable = ring_buffer_writable(sSyslogBuffer);
	if (writable < length) {
		// drop old data
		size_t toDrop = length - writable;
		ring_buffer_flush(sSyslogBuffer, toDrop);

		if (toDrop > sSyslogBufferOffset) {
			sSyslogBufferOffset = 0;
			sSyslogDropped = true;
		} else
			sSyslogBufferOffset -= toDrop;

		sSyslogDebuggerOffset -= std::min(toDrop, sSyslogDebuggerOffset);
	}

	ring_buffer_write(sSyslogBuffer, (uint8*)text, length);

	if (notify)
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

	if (sSyslogBuffer != NULL) {
		if (sDebugSyslog)
			delete_area(area_for(sSyslogBuffer));
		else
			delete_ring_buffer(sSyslogBuffer);

		sSyslogBuffer = NULL;
	}

	free(sSyslogMessage);
	delete_sem(sSyslogNotify);

	return B_ERROR;
}


static status_t
syslog_init_post_vm(struct kernel_args* args)
{
	status_t status;
	int32 length = 0;

	if (!sSyslogOutputEnabled) {
		sSyslogBuffer = NULL;
			// Might already have been set in syslog_init(), if the debug syslog
			// was enabled. Just drop it -- we'll never create the area.
		return B_OK;
	}

	sSyslogMessage = (syslog_message*)malloc(SYSLOG_MESSAGE_BUFFER_SIZE);
	if (sSyslogMessage == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	if (sSyslogBuffer == NULL) {
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

		if (sSyslogBuffer == NULL) {
			status = B_NO_MEMORY;
			goto err2;
		}
	} else {
		// create an area for the debug syslog buffer
		void* base = (void*)ROUNDDOWN((addr_t)args->debug_output, B_PAGE_SIZE);
		size_t size = ROUNDUP(args->debug_size, B_PAGE_SIZE);
		area_id area = create_area("syslog debug", &base, B_EXACT_ADDRESS, size,
				B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < 0) {
			status = B_NO_MEMORY;
			goto err2;
		}
	}

	// initialize syslog message
	sSyslogMessage->from = 0;
	sSyslogMessage->options = LOG_KERN;
	sSyslogMessage->priority = LOG_DEBUG;
	sSyslogMessage->ident[0] = '\0';
	//strcpy(sSyslogMessage->ident, "KERNEL");

	if (args->debug_output != NULL)
		syslog_write((const char*)args->debug_output, args->debug_size, false);

	char revisionBuffer[64];
	length = snprintf(revisionBuffer, sizeof(revisionBuffer),
		"Welcome to syslog debug output!\nHaiku revision: %s\n",
		get_haiku_revision());
	syslog_write(revisionBuffer,
		std::min(length, (int32)sizeof(revisionBuffer) - 1), false);

	add_debugger_command_etc("syslog", &cmd_dump_syslog,
		"Dumps the syslog buffer.",
		"[ \"-n\" ] [ \"-k\" ]\n"
		"Dumps the whole syslog buffer, or, if -k is specified, only "
		"the part that hasn't been sent yet.\n", 0);

	return B_OK;

err2:
	free(sSyslogMessage);
err1:
	sSyslogOutputEnabled = false;
	sSyslogBuffer = NULL;
	return status;
}


static status_t
syslog_init(struct kernel_args* args)
{
	if (!args->keep_debug_output_buffer || args->debug_output == NULL)
		return B_OK;

	sSyslogBuffer = create_ring_buffer_etc(args->debug_output, args->debug_size,
		RING_BUFFER_INIT_FROM_BUFFER);
	sDebugSyslog = true;

	return B_OK;
}


static void
debug_memcpy_trampoline(void* _parameters)
{
	debug_memcpy_parameters* parameters = (debug_memcpy_parameters*)_parameters;
	memcpy(parameters->to, parameters->from, parameters->size);
}


static void
debug_strlcpy_trampoline(void* _parameters)
{
	debug_strlcpy_parameters* parameters
		= (debug_strlcpy_parameters*)_parameters;
	parameters->result = strlcpy(parameters->to, parameters->from,
		parameters->size);
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


//!	Must be called with the sSpinlock held.
static void
debug_output(const char* string, int32 length, bool notifySyslog)
{
	if (length >= OUTPUT_BUFFER_SIZE)
		length = OUTPUT_BUFFER_SIZE - 1;

	if (length > 1 && string[length - 1] == '\n'
		&& strncmp(string, sLastOutputBuffer, length) == 0) {
		sMessageRepeatCount++;
		sMessageRepeatLastTime = system_time();
		if (sMessageRepeatFirstTime == 0)
			sMessageRepeatFirstTime = sMessageRepeatLastTime;
	} else {
		flush_pending_repeats(notifySyslog);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(string);
		if (sSyslogOutputEnabled)
			syslog_write(string, length, notifySyslog);
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
}


//!	Must be called with the sSpinlock held.
static void
flush_pending_repeats(bool notifySyslog)
{
	if (sMessageRepeatCount <= 0)
		return;

	if (sMessageRepeatCount > 1) {
		static char temp[40];
		size_t length = snprintf(temp, sizeof(temp),
			"Last message repeated %" B_PRId32 " times.\n", sMessageRepeatCount);
		length = std::min(length, sizeof(temp) - 1);

		if (sSerialDebugEnabled)
			arch_debug_serial_puts(temp);
		if (sSyslogOutputEnabled)
			syslog_write(temp, length, notifySyslog);
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
		if (sSyslogOutputEnabled)
			syslog_write(sLastOutputBuffer, length, notifySyslog);
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
dprintf_args(const char* format, va_list args, bool notifySyslog)
{
	if (are_interrupts_enabled()) {
		MutexLocker locker(sOutputLock);

		int32 length = vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format,
			args);
		length = std::min(length, (int32)OUTPUT_BUFFER_SIZE - 1);

		InterruptsSpinLocker _(sSpinlock);
		debug_output(sOutputBuffer, length, notifySyslog);
	} else {
		InterruptsSpinLocker _(sSpinlock);

		int32 length = vsnprintf(sInterruptOutputBuffer, OUTPUT_BUFFER_SIZE,
			format, args);
		length = std::min(length, (int32)OUTPUT_BUFFER_SIZE - 1);

		debug_output(sInterruptOutputBuffer, length, notifySyslog);
	}
}


// #pragma mark - private kernel API


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
	InterruptsSpinLocker _(sSpinlock);
	debug_output(string, length, true);
}


void
debug_early_boot_message(const char* string)
{
	arch_debug_serial_early_boot_message(string);
}


void
debug_init(kernel_args* args)
{
	new(&gDefaultDebugOutputFilter) DefaultDebugOutputFilter;

	syslog_init(args);

	debug_paranoia_init();
	arch_debug_console_init(args);
}


void
debug_init_post_vm(kernel_args* args)
{
	add_debugger_command_etc("cpu", &cmd_switch_cpu,
		"Switches to another CPU.",
		"<cpu>\n"
		"Switches to CPU with the index <cpu>.\n", 0);
	add_debugger_command_etc("message", &cmd_dump_kdl_message,
		"Reprint the message printed when entering KDL",
		"\n"
		"Reprints the message printed when entering KDL.\n", 0);
	add_debugger_command_etc("panic_commands", &cmd_execute_panic_commands,
		"Execute commands associated with the panic() that caused "
			"entering KDL",
		"\n"
		"Executes the commands associated with the panic() that caused "
			"entering KDL.\n", 0);

	debug_builtin_commands_init();
	arch_debug_init(args);

	debug_heap_init();
	debug_variables_init();
	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);
	tracing_init();
}


void
debug_init_post_settings(struct kernel_args* args)
{
	// get debug settings

	sSerialDebugEnabled = get_safemode_boolean("serial_debug_output",
		sSerialDebugEnabled);
	sSyslogOutputEnabled = get_safemode_boolean("syslog_debug_output",
		sSyslogOutputEnabled);
	sBlueScreenOutput = get_safemode_boolean("bluescreen", true);
	sEmergencyKeysEnabled = get_safemode_boolean("emergency_keys",
		sEmergencyKeysEnabled);
	sDebugScreenEnabled = get_safemode_boolean("debug_screen", false);

	if ((sBlueScreenOutput || sDebugScreenEnabled)
		&& blue_screen_init() != B_OK)
		sBlueScreenOutput = sDebugScreenEnabled = false;

	if (sDebugScreenEnabled)
		blue_screen_enter(true);

	syslog_init_post_vm(args);
}


void
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

	frame_buffer_console_init_post_modules(args);
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
debug_trap_cpu_in_kdl(int32 cpu, bool returnIfHandedOver)
{
	InterruptsLocker locker;

	// return, if we've been called recursively (we call
	// smp_intercpu_int_handler() below)
	if (sCPUTrapped[cpu])
		return;

	arch_debug_save_registers(&sDebugRegisters[cpu]);

	sCPUTrapped[cpu] = true;

	while (sInDebugger != 0) {
		if (sHandOverKDL && sHandOverKDLToCPU == cpu) {
			if (returnIfHandedOver)
				break;

			kernel_debugger_internal(NULL, NULL,
				sCurrentKernelDebuggerMessageArgs, cpu);
		} else
			smp_intercpu_int_handler(cpu);
	}

	sCPUTrapped[cpu] = false;
}


void
debug_double_fault(int32 cpu)
{
	kernel_debugger_internal("Double Fault!", NULL,
		sCurrentKernelDebuggerMessageArgs, cpu);
}


bool
debug_emergency_key_pressed(char key)
{
	if (!sEmergencyKeysEnabled)
		return false;

	if (key == 'd') {
		kernel_debugger("Keyboard Requested Halt.");
		return true;
	}

	// Broadcast to the kernel debugger modules

	for (uint32 i = 0; i < kMaxDebuggerModules; i++) {
		if (sDebuggerModules[i] && sDebuggerModules[i]->emergency_key_pressed) {
			if (sDebuggerModules[i]->emergency_key_pressed(key))
				return true;
		}
	}

	return false;
}


/*!	Verifies that the complete given memory range is accessible in the current
	context.

	Invoked in the kernel debugger only.

	\param address The start address of the memory range to be checked.
	\param size The size of the memory range to be checked.
	\param protection The area protection for which to check. Valid is a bitwise
		or of one or more of \c B_KERNEL_READ_AREA or \c B_KERNEL_WRITE_AREA.
	\return \c true, if the complete memory range can be accessed in all ways
		specified by \a protection, \c false otherwise.
*/
bool
debug_is_kernel_memory_accessible(addr_t address, size_t size,
	uint32 protection)
{
	addr_t endAddress = ROUNDUP(address + size, B_PAGE_SIZE);
	address = ROUNDDOWN(address, B_PAGE_SIZE);

	if (!IS_KERNEL_ADDRESS(address) || endAddress < address)
		return false;

	for (; address < endAddress; address += B_PAGE_SIZE) {
		if (!arch_vm_translation_map_is_kernel_page_accessible(address,
				protection)) {
			return false;
		}
	}

	return true;
}


/*!	Calls a function in a setjmp() + fault handler context.
	May only be used in the kernel debugger.

	\param jumpBuffer Buffer to be used for setjmp()/longjmp().
	\param function The function to be called.
	\param parameter The parameter to be passed to the function to be called.
	\return
		- \c 0, when the function executed without causing a page fault or
		  calling longjmp().
		- \c 1, when the function caused a page fault.
		- Any other value the function passes to longjmp().
*/
int
debug_call_with_fault_handler(jmp_buf jumpBuffer, void (*function)(void*),
	void* parameter)
{
	// save current fault handler
	cpu_ent* cpu = gCPU + sDebuggerOnCPU;
	addr_t oldFaultHandler = cpu->fault_handler;
	addr_t oldFaultHandlerStackPointer = cpu->fault_handler_stack_pointer;

	int result = setjmp(jumpBuffer);
	if (result == 0) {
		arch_debug_call_with_fault_handler(cpu, jumpBuffer, function,
			parameter);
	}

	// restore old fault handler
	cpu->fault_handler = oldFaultHandler;
	cpu->fault_handler_stack_pointer = oldFaultHandlerStackPointer;

	return result;
}


/*!	Similar to user_memcpy(), but can only be invoked from within the kernel
	debugger (and must not be used outside).
	The supplied \a teamID specifies the address space in which to interpret
	the addresses. It can be \c B_CURRENT_TEAM for debug_get_debugged_thread(),
	or any valid team ID. If the addresses are both kernel addresses, the
	argument is ignored and the current address space is used.
*/
status_t
debug_memcpy(team_id teamID, void* to, const void* from, size_t size)
{
	// don't allow address overflows
	if ((addr_t)from + size < (addr_t)from || (addr_t)to + size < (addr_t)to)
		return B_BAD_ADDRESS;

	// Try standard memcpy() with fault handler, if the addresses can be
	// interpreted in the current address space.
	if ((IS_KERNEL_ADDRESS(from) && IS_KERNEL_ADDRESS(to))
			|| debug_is_debugged_team(teamID)) {
		debug_memcpy_parameters parameters = {to, from, size};

		if (debug_call_with_fault_handler(gCPU[sDebuggerOnCPU].fault_jump_buffer,
				&debug_memcpy_trampoline, &parameters) == 0) {
			return B_OK;
		}
	}

	// Try harder. The pages of the respective memory could be unmapped but
	// still exist in a cache (the page daemon does that with inactive pages).
	while (size > 0) {
		uint8 buffer[32];
		size_t toCopy = std::min(size, sizeof(buffer));

		// restrict the size so we don't cross page boundaries
		if (((addr_t)from + toCopy) % B_PAGE_SIZE < toCopy)
			toCopy -= ((addr_t)from + toCopy) % B_PAGE_SIZE;
		if (((addr_t)to + toCopy) % B_PAGE_SIZE < toCopy)
			toCopy -= ((addr_t)to + toCopy) % B_PAGE_SIZE;

		if (vm_debug_copy_page_memory(teamID, (void*)from, buffer, toCopy,
				false) != B_OK
			|| vm_debug_copy_page_memory(teamID, to, buffer, toCopy, true)
				!= B_OK) {
			return B_BAD_ADDRESS;
		}

		from = (const uint8*)from + toCopy;
		to = (uint8*)to + toCopy;
		size -= toCopy;
	}

	return B_OK;
}


/*!	Similar to user_strlcpy(), but can only be invoked from within the kernel
	debugger (and must not be used outside).
	The supplied \a teamID specifies the address space in which to interpret
	the addresses. It can be \c B_CURRENT_TEAM for debug_get_debugged_thread(),
	or any valid team ID. If the addresses are both kernel addresses, the
	argument is ignored and the current address space is used.
*/
ssize_t
debug_strlcpy(team_id teamID, char* to, const char* from, size_t size)
{
	if (from == NULL || (to == NULL && size > 0))
		return B_BAD_ADDRESS;

	// limit size to avoid address overflows
	size_t maxSize = std::min(size,
		~(addr_t)0 - std::max((addr_t)from, (addr_t)to) + 1);
		// NOTE: Since strlcpy() determines the length of \a from, the source
		// address might still overflow.

	// Try standard strlcpy() with fault handler, if the addresses can be
	// interpreted in the current address space.
	if ((IS_KERNEL_ADDRESS(from) && IS_KERNEL_ADDRESS(to))
			|| debug_is_debugged_team(teamID)) {
		debug_strlcpy_parameters parameters = {to, from, maxSize};

		if (debug_call_with_fault_handler(
				gCPU[sDebuggerOnCPU].fault_jump_buffer,
				&debug_strlcpy_trampoline, &parameters) == 0) {
			// If we hit the address overflow boundary, fail.
			if (parameters.result >= maxSize && maxSize < size)
				return B_BAD_ADDRESS;

			return parameters.result;
		}
	}

	// Try harder. The pages of the respective memory could be unmapped but
	// still exist in a cache (the page daemon does that with inactive pages).
	size_t totalLength = 0;
	while (maxSize > 0) {
		char buffer[32];
		size_t toCopy = std::min(maxSize, sizeof(buffer));

		// restrict the size so we don't cross page boundaries
		if (((addr_t)from + toCopy) % B_PAGE_SIZE < toCopy)
			toCopy -= ((addr_t)from + toCopy) % B_PAGE_SIZE;
		if (((addr_t)to + toCopy) % B_PAGE_SIZE < toCopy)
			toCopy -= ((addr_t)to + toCopy) % B_PAGE_SIZE;

		// copy the next part of the string from the source
		if (vm_debug_copy_page_memory(teamID, (void*)from, buffer, toCopy,
				false) != B_OK) {
			return B_BAD_ADDRESS;
		}

		// determine the length of the part and whether we've reached the end
		// of the string
		size_t length = strnlen(buffer, toCopy);
		bool endOfString = length < toCopy;

		from = (const char*)from + toCopy;
		totalLength += length;
		maxSize -= length;

		if (endOfString) {
			// only copy the actual string, including the terminating null
			toCopy = length + 1;
		}

		if (size > 0) {
			// We still have space left in the target buffer.
			if (size <= length) {
				// Not enough space for the complete part. Null-terminate it and
				// copy what we can.
				buffer[size - 1] = '\0';
				totalLength += length - size;
				toCopy = size;
			}

			if (vm_debug_copy_page_memory(teamID, to, buffer, toCopy, true)
					!= B_OK) {
				return B_BAD_ADDRESS;
			}

			to = (char*)to + toCopy;
			size -= toCopy;
		}

		if (endOfString)
			return totalLength;
	}

	return totalLength;
}


// #pragma mark - public API


uint64
parse_expression(const char* expression)
{
	uint64 result;
	return evaluate_debug_expression(expression, &result, true) ? result : 0;
}


void
panic(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	cpu_status state = disable_interrupts();

	kernel_debugger_internal("PANIC: ", format, args, smp_get_current_cpu());

	restore_interrupts(state);

	va_end(args);
}


void
kernel_debugger(const char* message)
{
	cpu_status state = disable_interrupts();

	kernel_debugger_internal(message, NULL, sCurrentKernelDebuggerMessageArgs,
		smp_get_current_cpu());

	restore_interrupts(state);
}


bool
set_dprintf_enabled(bool newState)
{
	bool oldState = sSerialDebugEnabled;
	sSerialDebugEnabled = newState;

	return oldState;
}


void
dprintf(const char* format, ...)
{
	va_list args;

	if (!sSerialDebugEnabled && !sSyslogOutputEnabled && !sBlueScreenEnabled)
		return;

	va_start(args, format);
	dprintf_args(format, args, true);
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


struct arch_debug_registers*
debug_get_debug_registers(int32 cpu)
{
	if (cpu < 0 || cpu > smp_get_num_cpus())
		return NULL;

	return &sDebugRegisters[cpu];
}


Thread*
debug_set_debugged_thread(Thread* thread)
{
	Thread* previous = sDebuggedThread;
	sDebuggedThread = thread;
	return previous;
}


Thread*
debug_get_debugged_thread()
{
	return sDebuggedThread != NULL
		? sDebuggedThread : thread_get_current_thread();
}


/*!	Returns whether the supplied team ID refers to the same team the currently
	debugged thread (debug_get_debugged_thread()) belongs to.
	Always returns \c true, if \c B_CURRENT_TEAM is given.
*/
bool
debug_is_debugged_team(team_id teamID)
{
	if (teamID == B_CURRENT_TEAM)
		return true;

	Thread* thread = debug_get_debugged_thread();
	return thread != NULL && thread->team != NULL
		&& thread->team->id == teamID;
}


//	#pragma mark -
//	userland syscalls


status_t
_user_kernel_debugger(const char *userMessage)
{
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	char message[512];
	strcpy(message, "USER: ");
	size_t len = strlen(message);

	if (userMessage == NULL || !IS_USER_ADDRESS(userMessage)
		|| 	user_strlcpy(message + len, userMessage, sizeof(message) - len)
			< 0) {
		return B_BAD_ADDRESS;
	}

	kernel_debugger(message);
	return B_OK;
}


void
_user_debug_output(const char* userString)
{
	if (!sSerialDebugEnabled && !sSyslogOutputEnabled)
		return;

	if (!IS_USER_ADDRESS(userString))
		return;

	char string[512];
	int32 length;
	do {
		length = user_strlcpy(string, userString, sizeof(string));
		if (length <= 0)
			break;
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
