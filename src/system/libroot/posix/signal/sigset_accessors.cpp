/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold, danielre@users.sf.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <signal.h>

#include <errno.h>

#include <signal_defs.h>
#include <syscalls.h>

#include <symbol_versioning.h>

#include <signal_private.h>


// #pragma - backward compatibility implementations


int
__sigemptyset_beos(sigset_t_beos* set)
{
	*set = (sigset_t_beos)0;
	return 0;
}


int
__sigfillset_beos(sigset_t_beos* set)
{
	*set = ~(sigset_t_beos)0;
	return 0;
}


int
__sigismember_beos(const sigset_t_beos* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER_BEOS) {
		errno = EINVAL;
		return -1;
	}

	return (*set & SIGNAL_TO_MASK(signal)) != 0 ? 1 : 0;
}


int
__sigaddset_beos(sigset_t_beos* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER_BEOS) {
		errno = EINVAL;
		return -1;
	}

	*set |= SIGNAL_TO_MASK(signal);
	return 0;
}


int
__sigdelset_beos(sigset_t_beos* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER_BEOS) {
		errno = EINVAL;
		return -1;
	}

	*set &= ~SIGNAL_TO_MASK(signal);
	return 0;
}


// #pragma - current implementations


int
__sigemptyset(sigset_t* set)
{
	*set = (sigset_t)0;
	return 0;
}


int
__sigfillset(sigset_t* set)
{
	*set = ~(sigset_t)0;
	return 0;
}


int
__sigismember(const sigset_t* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER) {
		errno = EINVAL;
		return -1;
	}

	return (*set & SIGNAL_TO_MASK(signal)) != 0 ? 1 : 0;
}


int
__sigaddset(sigset_t* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER) {
		errno = EINVAL;
		return -1;
	}

	*set |= SIGNAL_TO_MASK(signal);
	return 0;
}


int
__sigdelset(sigset_t* set, int signal)
{
	if (signal <= 0 || signal > MAX_SIGNAL_NUMBER) {
		errno = EINVAL;
		return -1;
	}

	*set &= ~SIGNAL_TO_MASK(signal);
	return 0;
}


// #pragma - versioned symbols


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigemptyset_beos", "sigemptyset@",
	"BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigfillset_beos", "sigfillset@",
	"BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigismember_beos", "sigismember@",
	"BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigaddset_beos", "sigaddset@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigdelset_beos", "sigdelset@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigemptyset", "sigemptyset@@",
	"1_ALPHA4");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigfillset", "sigfillset@@",
	"1_ALPHA4");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigismember", "sigismember@@",
	"1_ALPHA4");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigaddset", "sigaddset@@", "1_ALPHA4");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigdelset", "sigdelset@@", "1_ALPHA4");
