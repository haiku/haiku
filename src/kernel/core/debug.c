/* This file contains the debugger */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <debug.h>
#include <arch/int.h>
#include <smp.h>
#include <console.h>
#include <memheap.h>
#include <gdb.h>
#include <Errors.h>
#include <int.h>

#include <arch/dbg_console.h>
#include <arch/debug.h>
#include <arch/cpu.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


int dbg_register_file[2][14]; /* XXXmpetit -- must be made generic */


static bool serial_debug_on = false;
static spinlock_t dbg_spinlock = 0;
static int debugger_on_cpu = -1;

struct debugger_command
{
	struct debugger_command *next;
	void (*func)(int, char **);
	const char *cmd;
	const char *description;
};

static struct debugger_command *commands;

#define LINE_BUF_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char line_buf[HISTORY_SIZE][LINE_BUF_SIZE] = { "", };
static char parse_line[LINE_BUF_SIZE] = "";
static int cur_line = 0;
static char *args[MAX_ARGS] = { NULL, };

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))

static int debug_read_line(char *buf, int max_len)
{
	char c;
	int ptr = 0;
	bool done = false;
	int cur_history_spot = cur_line;

	while(!done) {
		c = arch_dbg_con_read();
		switch(c) {
			case '\n':
			case '\r':
				buf[ptr++] = '\0';
				dbg_puts("\n");
				done = true;
				break;
			case 8: // backspace
				if(ptr > 0) {
					dbg_puts("\x1b[1D"); // move to the left one
					dbg_putch(' ');
					dbg_puts("\x1b[1D"); // move to the left one
					ptr--;
				}
				break;
			case 27: // escape sequence
				c = arch_dbg_con_read(); // should be '['
				c = arch_dbg_con_read();
				switch(c) {
					case 67: // right arrow acts like space
						buf[ptr++] = ' ';
						dbg_putch(' ');
						break;
					case 68: // left arrow acts like backspace
						if(ptr > 0) {
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

						if(c == 65) {
							// up arrow
							history_line = cur_history_spot - 1;
							if(history_line < 0)
								history_line = HISTORY_SIZE - 1;
						} else {
							// down arrow
							if(cur_history_spot != cur_line) {
								history_line = cur_history_spot + 1;
								if(history_line >= HISTORY_SIZE)
									history_line = 0;
							} else {
								break; // nothing to do here
							}
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
				if(ptr == 0) {
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
		if(ptr >= max_len - 2) {
			buf[ptr++] = '\0';
			dbg_puts("\n");
			done = true;
			break;
		}
	}
	return ptr;
}

static int debug_parse_line(char *buf, char **argv, int *argc, int max_args)
{
	int pos = 0;

	strcpy(parse_line, buf);

	if(!isspace(parse_line[0])) {
		argv[0] = parse_line;
		*argc = 1;
	} else {
		*argc = 0;
	}

	while(parse_line[pos] != '\0') {
		if(isspace(parse_line[pos])) {
			parse_line[pos] = '\0';
			// scan all of the whitespace out of this
			while(isspace(parse_line[++pos]))
				;
			if(parse_line[pos] == '\0')
				break;
			argv[*argc] = &parse_line[pos];
			(*argc)++;

			if(*argc >= max_args - 1)
				break;
		}
		pos++;
	}

	return *argc;
}

static void kernel_debugger_loop()
{
	int argc;
	struct debugger_command *cmd;

	int_disable_interrupts();

	dprintf("kernel debugger on cpu %d\n", smp_get_current_cpu());
	debugger_on_cpu = smp_get_current_cpu();

	for(;;) {
		dprintf("> ");
		debug_read_line(line_buf[cur_line], LINE_BUF_SIZE);
		debug_parse_line(line_buf[cur_line], args, &argc, MAX_ARGS);
		if(argc <= 0)
			continue;

		debugger_on_cpu = smp_get_current_cpu();

		cmd = commands;
		while(cmd != NULL) {
			if(strcmp(args[0], cmd->cmd) == 0) {
				cmd->func(argc, args);
			}
			cmd = cmd->next;
		}
		cur_line++;
		if(cur_line >= HISTORY_SIZE)
			cur_line = 0;
	}
}

void kernel_debugger()
{
	dbg_save_registers(&(dbg_register_file[smp_get_current_cpu()][0]));

	kernel_debugger_loop();
}

int panic(const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	char temp[128];
	int state;

	dbg_set_serial_debug(true);

	state = int_disable_interrupts();

	va_start(args, fmt);
	ret = vsprintf(temp, fmt, args);
	va_end(args);

	dprintf("PANIC%d: %s", smp_get_current_cpu(), temp);

	if(debugger_on_cpu != smp_get_current_cpu()) {
		// halt all of the other cpus

		// XXX need to flush current smp mailbox to make sure this goes
		// through. Otherwise it'll hang
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	kernel_debugger();

	int_restore_interrupts(state);
	return ret;
}

int dprintf(const char *fmt, ...)
{
	va_list args;
	char temp[512];
	int ret = 0;

	if(serial_debug_on) {
		va_start(args, fmt);
		ret = vsprintf(temp, fmt, args);
		va_end(args);

		dbg_puts(temp);
	}
	return ret;
}

char dbg_putch(char c)
{
	char ret;
	int flags = int_disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	if(serial_debug_on)
		ret = arch_dbg_con_putch(c);
	else
		ret = c;

	release_spinlock(&dbg_spinlock);
	int_restore_interrupts(flags);

	return ret;
}

void dbg_puts(const char *s)
{
	int flags = int_disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	if(serial_debug_on)
		arch_dbg_con_puts(s);

	release_spinlock(&dbg_spinlock);
	int_restore_interrupts(flags);
}

int dbg_add_command(void (*func)(int, char **), const char *name, const char *desc)
{
	int flags;
	struct debugger_command *cmd;

	cmd = (struct debugger_command *)kmalloc(sizeof(struct debugger_command));
	if(cmd == NULL)
		return ENOMEM;

	cmd->func = func;
	cmd->cmd = name;
	cmd->description = desc;

	flags = int_disable_interrupts();
	acquire_spinlock(&dbg_spinlock);

	cmd->next = commands;
	commands = cmd;

	release_spinlock(&dbg_spinlock);
	int_restore_interrupts(flags);

	return B_NO_ERROR;
}

static void cmd_reboot(int argc, char **argv)
{
	reboot();
}

static void cmd_help(int argc, char **argv)
{
	struct debugger_command *cmd;

	dprintf("debugger commands:\n");
	cmd = commands;
	while(cmd != NULL) {
		dprintf("%-32s\t\t%s\n", cmd->cmd, cmd->description);
		cmd = cmd->next;
	}
}

int dbg_init(kernel_args *ka)
{
	commands = NULL;

	return arch_dbg_con_init(ka);
}

int dbg_init2(kernel_args *ka)
{
	dbg_add_command(&cmd_help, "help", "List all debugger commands");
	dbg_add_command(&cmd_reboot, "reboot", "Reboot");
	dbg_add_command(&cmd_gdb, "gdb", "Connect to remote gdb");

	return B_NO_ERROR;
}

bool dbg_set_serial_debug(bool new_val)
{
	int temp = serial_debug_on;
	serial_debug_on = new_val;
	return temp;
}

bool dbg_get_serial_debug()
{
	return serial_debug_on;
}

