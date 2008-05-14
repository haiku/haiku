/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/wait.h>

#include <errno.h>
#include <stdio.h>

#include <syscalls.h>
#include <thread_defs.h>


// ToDo: properly implement waitpid(), and waitid()


pid_t
wait(int *_status)
{
	return waitpid(-1, _status, 0);
}


pid_t
waitpid(pid_t pid, int *_status, int options)
{
	status_t returnCode;
	int32 reason;

	thread_id thread = _kern_wait_for_child(pid, options, &reason, &returnCode);

	if (thread >= B_OK && _status != NULL) {

		// See kernel's wait_for_child() for how the return information is
		// encoded:
		// reason = (signal << 16) | reason

		int status;
		switch (reason & 0xffff) {
			case THREAD_RETURN_INTERRUPTED:
				// fill in signal for WIFSIGNALED() and WTERMSIG()
				status = (reason >> 8) & 0xff00;
				break;
			case THREAD_STOPPED:
				// WIFSTOPPED() and WSTOPSIG()
				status = reason & 0xff0000;
				break;
			case THREAD_CONTINUED:
				// WIFCONTINUED()
				status = 0x20000;
				break;
			case THREAD_RETURN_EXIT:
			default:
				// WEXITSTATUS()
				status = returnCode & 0xff;
				break;
		}

		*_status = status;
	}

	if (thread < B_OK) {
		errno = thread;
		if (thread == B_WOULD_BLOCK && (options & WNOHANG) != 0)
			return 0;
		return -1;
	}

	return thread;
}


// TODO: Implement as part of real-time signal support!
//int
//waitid(idtype_t idType, id_t id, siginfo_t *info, int options)
//{
//	// waitid() is not available on BeOS so we may be lazy here and remove it...
//	fprintf(stderr, "waitid(): NOT IMPLEMENTED\n");
//	return -1;
//}

