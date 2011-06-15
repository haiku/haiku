/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/wait.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <syscalls.h>
#include <thread_defs.h>


pid_t
wait(int* _status)
{
	return waitpid(-1, _status, 0);
}


pid_t
waitpid(pid_t pid, int* _status, int options)
{
	// wait
	siginfo_t info;
	pid_t child = _kern_wait_for_child(pid, options, &info);

	pthread_testcancel();

	if (child < 0) {
		// When not getting a child status when WNOHANG was specified, don't
		// fail.
		if (child == B_WOULD_BLOCK && (options & WNOHANG) != 0)
			return 0;
		RETURN_AND_SET_ERRNO(child);
	}

	// prepare the status
	if (_status != NULL) {
		int status;
		switch (info.si_code) {
			case CLD_EXITED:
				// fill in exit status for WIFEXITED() and WEXITSTATUS()
				status = info.si_status & 0xff;
				break;

			case CLD_KILLED:
			case CLD_DUMPED:
				// fill in signal for WIFSIGNALED() and WTERMSIG()
				status = (info.si_status << 8) & 0xff00;
				// if core dumped, set flag for WIFCORED()
				if (info.si_code == CLD_DUMPED)
					status |= 0x10000;
				break;

			case CLD_CONTINUED:
				// set flag for WIFCONTINUED()
				status = 0x20000;
				break;

			case CLD_STOPPED:
				// fill in signal for WIFSTOPPED() and WSTOPSIG()
				status = (info.si_status << 16) & 0xff0000;
				break;

			case CLD_TRAPPED:
				// we don't do that
			default:
				// should never get here -- assume exited
				status = 0;
				break;
		}

		*_status = status;
	}

	return child;
}


int
waitid(idtype_t idType, id_t id, siginfo_t* info, int options)
{
	// translate the idType, id pair to a waitpid() style ID
	switch (idType) {
		case P_ALL:
			// any child
			id = -1;
			break;

		case P_PID:
			// the child with the given ID
			if (id <= 0)
				RETURN_AND_SET_ERRNO_TEST_CANCEL(EINVAL);
			break;

		case P_PGID:
			// any child in the given process group
			if (id <= 1)
				RETURN_AND_SET_ERRNO_TEST_CANCEL(EINVAL);
			id = -id;
			break;

		default:
			RETURN_AND_SET_ERRNO_TEST_CANCEL(EINVAL);
	}

	pid_t child = _kern_wait_for_child(id, options, info);
	if (child >= 0 || child == B_WOULD_BLOCK)
		return 0;

	RETURN_AND_SET_ERRNO_TEST_CANCEL(child);
}
