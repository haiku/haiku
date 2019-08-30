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


static status_t
init_tty_module()
{
	// create the request mutex
	recursive_lock_init(&gTTYRequestLock, "tty requests");

	// create the global mutex
	mutex_init(&gGlobalTTYLock, "tty global");

	// create the cookie mutex
	mutex_init(&gTTYCookieLock, "tty cookies");

	return B_OK;
}


static void
uninit_tty_module()
{
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
