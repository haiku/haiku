/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <unistd.h>

#include <libroot/errno_private.h>
#include <random_defs.h>
#include <syscalls.h>


int
getentropy(void *buf, size_t buflen)
{
	status_t status;
	struct random_get_entropy_args args;
	args.buffer = buf;
	args.length = buflen;

	status = _kern_generic_syscall(RANDOM_SYSCALLS, RANDOM_GET_ENTROPY,
		&args, sizeof(args));
	if (args.length < buflen)
		status = EIO;

	if (status < 0) {
		__set_errno(status);
		return -1;
	}
	return 0;
}
