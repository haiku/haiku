/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>


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

	if (_status != NULL) {
		// ToDo: fill in _status!
	}

	if (thread < B_OK) {
		errno = thread;
		if (thread == B_WOULD_BLOCK && (options & WNOHANG) != 0)
			return 0;
		return -1;
	}

	return thread;
}


int
waitid(idtype_t idType, id_t id, siginfo_t *info, int options)
{
	// waitid() is not available on BeOS so we may be lazy here and remove it...
	fprintf(stderr, "waitid(): NOT IMPLEMENTED\n");
	return -1;
}

