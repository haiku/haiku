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
_mutex_lock(mutex*, bool)
{
	return true;
}


extern "C" void
_mutex_unlock(mutex*, bool)
{
}

