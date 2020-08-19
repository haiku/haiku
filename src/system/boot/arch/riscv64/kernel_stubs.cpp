/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

// This file just collects stubs that allow kernel sources to be used in the
// bootloader more easily.

#include <OS.h>
#include <lock.h>
#include <arch_config.h>


extern "C" bool
in_command_invocation()
{
	return false;
}


extern "C" void
abort_debugger_command()
{
}


extern "C" char
kgetc()
{
	return -1;
}


extern "C" status_t
_mutex_lock(mutex*, void*)
{
	return B_OK;
}


extern "C" void
_mutex_unlock(mutex*)
{
}


#ifdef ATOMIC_FUNCS_ARE_SYSCALLS

/* needed by packagefs */
extern "C" int32
atomic_add(vint32 *value, int32 addValue)
{
	int32 old = *value;
	*value += addValue;
	return old;
}

#endif /*ATOMIC_FUNCS_ARE_SYSCALLS*/
