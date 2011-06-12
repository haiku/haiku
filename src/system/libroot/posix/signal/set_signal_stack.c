/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <errno.h>

#include <syscalls.h>


void
set_signal_stack(void *ptr, size_t size)
{
	stack_t alternateStack;
	status_t status;

	alternateStack.ss_sp = ptr;
	alternateStack.ss_size = size;
	alternateStack.ss_flags = 0;

	status = _kern_set_signal_stack(&alternateStack, NULL);
	if (status < B_OK)
		errno = status;
}


