/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <unistd.h>

#include <errno_private.h>
#include <random_defs.h>
#include <syscall_utils.h>
#include <syscalls.h>


int
getentropy(void *buf, size_t buflen)
{
	if (buflen > GETENTROPY_MAX)
		RETURN_AND_SET_ERRNO(EINVAL);

	while (buflen > 0) {
		status_t status;
		struct random_get_entropy_args args;
		args.buffer = buf;
		args.length = buflen;

		status = _kern_generic_syscall(RANDOM_SYSCALLS, RANDOM_GET_ENTROPY,
			&args, sizeof(args));
		if (status == EINTR)
			continue;
		if (status < 0)
			RETURN_AND_SET_ERRNO(status);

		buf = (char*)buf + args.length;
		buflen -= args.length;
	}

	return 0;
}
