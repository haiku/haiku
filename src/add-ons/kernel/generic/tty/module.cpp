/*
 * Copyright 2010, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h>

#include <tty/tty_module.h>

#include "tty_private.h"

struct mutex gGlobalTTYLock;
struct mutex gTTYCookieLock;
struct recursive_lock gTTYRequestLock;


static void
dump_tty_settings(struct tty_settings& settings)
{
	kprintf("  pgrp_id:      %" B_PRId32 "\n", settings.pgrp_id);
	kprintf("  session_id:   %" B_PRId32 "\n", settings.session_id);

	kprintf("  termios:\n");
	kprintf("    c_iflag:    0x%08" B_PRIx32 "\n", settings.termios.c_iflag);
	kprintf("    c_oflag:    0x%08" B_PRIx32 "\n", settings.termios.c_oflag);
	kprintf("    c_cflag:    0x%08" B_PRIx32 "\n", settings.termios.c_cflag);
	kprintf("    c_lflag:    0x%08" B_PRIx32 "\n", settings.termios.c_lflag);
	kprintf("    c_line:     %d\n", settings.termios.c_line);
	kprintf("    c_ispeed:   %u\n", settings.termios.c_ispeed);
	kprintf("    c_ospeed:   %u\n", settings.termios.c_ospeed);
	for (int i = 0; i < NCCS; i++)
		kprintf("    c_cc[%02d]:   %d\n", i, settings.termios.c_cc[i]);

	kprintf("  wsize:        %u x %u c, %u x %u pxl\n",
		settings.window_size.ws_row, settings.window_size.ws_col,
		settings.window_size.ws_xpixel, settings.window_size.ws_ypixel);
}


static void
dump_tty_struct(struct tty& tty)
{
	kprintf("  tty @:        %p\n", &tty);
	kprintf("  is_master:    %s\n", tty.is_master ? "true" : "false");
	kprintf("  open_count:   %" B_PRId32 "\n", tty.open_count);
	kprintf("  select_pool:  %p\n", tty.select_pool);
	kprintf("  pending_eof:  %" B_PRIu32 "\n", tty.pending_eof);

	kprintf("  input_buffer:\n");
	kprintf("    first:      %" B_PRId32 "\n", tty.input_buffer.first);
	kprintf("    in:         %lu\n", tty.input_buffer.in);
	kprintf("    size:       %lu\n", tty.input_buffer.size);
	kprintf("    buffer:     %p\n", tty.input_buffer.buffer);

	kprintf("  reader queue:\n");
	tty.reader_queue.Dump("    ");
	kprintf("  writer queue:\n");
	tty.writer_queue.Dump("    ");

	dump_tty_settings(*tty.settings);

	kprintf("  cookies:     ");
	TTYCookieList::Iterator it = tty.cookies.GetIterator();
	while (tty_cookie* cookie = it.Next())
		kprintf(" %p", cookie);
	kprintf("\n");
}


static int
dump_tty(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("Usage: %s <tty address>\n", argv[0]);
		return 0;
	}

	char* endpointer;
	uintptr_t index = strtoul(argv[1], &endpointer, 0);
	if (*endpointer != '\0') {
		kprintf("Invalid tty index.\n");
		return 0;
	}

	struct tty* tty = (struct tty*)index;
	dump_tty_struct(*tty);

	return 0;
}


void
tty_add_debugger_commands()
{
	add_debugger_command("tty", &dump_tty, "Dump info on a tty");
}


void
tty_remove_debugger_commands()
{
	remove_debugger_command("tty", &dump_tty);
}


static status_t
init_tty_module()
{
	// create the request mutex
	recursive_lock_init(&gTTYRequestLock, "tty requests");

	// create the global mutex
	mutex_init(&gGlobalTTYLock, "tty global");

	// create the cookie mutex
	mutex_init(&gTTYCookieLock, "tty cookies");

	tty_add_debugger_commands();

	return B_OK;
}


static void
uninit_tty_module()
{
	tty_remove_debugger_commands();

	recursive_lock_destroy(&gTTYRequestLock);
	mutex_destroy(&gTTYCookieLock);
	mutex_destroy(&gGlobalTTYLock);
}


static int32
tty_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_tty_module();

		case B_MODULE_UNINIT:
			uninit_tty_module();
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct tty_module_info sTTYModule = {
	{
		B_TTY_MODULE_NAME,
		0, //B_KEEP_LOADED,
		tty_module_std_ops
	},

	&tty_create,
	&tty_destroy,
	&tty_create_cookie,
	&tty_close_cookie,
	&tty_destroy_cookie,
	&tty_read,
	&tty_write,
	&tty_control,
	&tty_select,
	&tty_deselect,
	&tty_hardware_signal
};


module_info *modules[] = {
	(module_info *)&sTTYModule,
	NULL
};
