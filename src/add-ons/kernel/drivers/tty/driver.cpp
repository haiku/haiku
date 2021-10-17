/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h>

#include "tty_driver.h"
#include "tty_private.h"


//#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#define DRIVER_NAME "tty"

static const int kMaxCachedSemaphores = 8;

int32 api_version = B_CUR_DRIVER_API_VERSION;

char *gDeviceNames[kNumTTYs * 2 + 3];
	// reserve space for "pt/" and "tt/" entries, "ptmx", "tty", and the
	// terminating NULL

static mutex sTTYLocks[kNumTTYs];
static tty_settings sTTYSettings[kNumTTYs];

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
	kprintf("  index:        %" B_PRId32 "\n", tty.index);
	kprintf("  is_master:    %s\n", tty.is_master ? "true" : "false");
	kprintf("  open_count:   %" B_PRId32 "\n", tty.open_count);
	kprintf("  select_pool:  %p\n", tty.select_pool);
	kprintf("  pending_eof:  %" B_PRIu32 "\n", tty.pending_eof);
	kprintf("  lock:         %p\n", tty.lock);

	kprintf("  input_buffer:\n");
	kprintf("    first:      %" B_PRId32 "\n", tty.input_buffer.first);
	kprintf("    in:         %lu\n", tty.input_buffer.in);
	kprintf("    size:       %lu\n", tty.input_buffer.size);
	kprintf("    buffer:     %p\n", tty.input_buffer.buffer);

	kprintf("  reader queue:\n");
	tty.reader_queue.Dump("    ");
	kprintf("  writer queue:\n");
	tty.writer_queue.Dump("    ");

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
		kprintf("Usage: %s <tty index>\n", argv[0]);
		return 0;
	}

	int32 index = atol(argv[1]);
	if (index < 0 || index >= (int32)kNumTTYs) {
		kprintf("Invalid tty index.\n");
		return 0;
	}

	kprintf("master:\n");
	dump_tty_struct(gMasterTTYs[index]);
	kprintf("slave:\n");
	dump_tty_struct(gSlaveTTYs[index]);
	kprintf("settings:\n");
	dump_tty_settings(sTTYSettings[index]);

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


status_t
init_hardware(void)
{
	TRACE((DRIVER_NAME ": init_hardware()\n"));
	return B_OK;
}


status_t
init_driver(void)
{
	TRACE((DRIVER_NAME ": init_driver()\n"));

	memset(gDeviceNames, 0, sizeof(gDeviceNames));

	// create the request mutex
	recursive_lock_init(&gTTYRequestLock, "tty requests");

	// create the global mutex
	mutex_init(&gGlobalTTYLock, "tty global");

	// create the cookie mutex
	mutex_init(&gTTYCookieLock, "tty cookies");

	// create driver name array and initialize basic TTY structures

	char letter = 'p';
	int8 digit = 0;

	for (uint32 i = 0; i < kNumTTYs; i++) {
		// For compatibility, we have to create the same mess in /dev/pt and
		// /dev/tt as BeOS does: we publish devices p0, p1, ..., pf, r1, ...,
		// sf. It would be nice if we could drop compatibility and create
		// something better. In fact we already don't need the master devices
		// anymore, since "/dev/ptmx" does the job. The slaves entries could
		// be published on the fly when a master is opened (e.g via
		// vfs_create_special_node()).
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "pt/%c%x", letter, digit);
		gDeviceNames[i] = strdup(buffer);

		snprintf(buffer, sizeof(buffer), "tt/%c%x", letter, digit);
		gDeviceNames[i + kNumTTYs] = strdup(buffer);

		if (++digit > 15)
			digit = 0, letter++;

		mutex_init(&sTTYLocks[i], "tty lock");
		reset_tty(&gMasterTTYs[i], i, &sTTYLocks[i], true);
		reset_tty(&gSlaveTTYs[i], i, &sTTYLocks[i], false);
		reset_tty_settings(&sTTYSettings[i]);

		if (!gDeviceNames[i] || !gDeviceNames[i + kNumTTYs]) {
			uninit_driver();
			return B_NO_MEMORY;
		}
	}

	gDeviceNames[2 * kNumTTYs] = (char *)"ptmx";
	gDeviceNames[2 * kNumTTYs + 1] = (char *)"tty";

	tty_add_debugger_commands();

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));

	tty_remove_debugger_commands();

	for (int32 i = 0; i < (int32)kNumTTYs * 2; i++)
		free(gDeviceNames[i]);

	for (int32 i = 0; i < (int32)kNumTTYs; i++)
		mutex_destroy(&sTTYLocks[i]);

	recursive_lock_destroy(&gTTYRequestLock);
	mutex_destroy(&gTTYCookieLock);
	mutex_destroy(&gGlobalTTYLock);
}


const char **
publish_devices(void)
{
	TRACE((DRIVER_NAME ": publish_devices()\n"));
	return const_cast<const char **>(gDeviceNames);
}


device_hooks *
find_device(const char *name)
{
	TRACE((DRIVER_NAME ": find_device(\"%s\")\n", name));

	for (uint32 i = 0; gDeviceNames[i] != NULL; i++)
		if (!strcmp(name, gDeviceNames[i])) {
			return i < kNumTTYs || i == (2 * kNumTTYs)
				? &gMasterTTYHooks : &gSlaveTTYHooks;
		}

	return NULL;
}


int32
get_tty_index(const char* name)
{
	// device names follow this form: "pt/%c%x"
	int8 digit = name[4];
	if (digit >= 'a') {
		// hexadecimal digits
		digit -= 'a' - 10;
	} else
		digit -= '0';

	return (name[3] - 'p') * 16 + digit;
}


void
reset_tty(struct tty* tty, int32 index, mutex* lock, bool isMaster)
{
	tty->ref_count = 0;
	tty->open_count = 0;
	tty->opened_count = 0;
	tty->index = index;
	tty->lock = lock;
	tty->settings = &sTTYSettings[index];
	tty->select_pool = NULL;
	tty->is_master = isMaster;
	tty->pending_eof = 0;
}
