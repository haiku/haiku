/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>
#include <syscall_process_info.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <errno_private.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		__set_errno(err); \
		return -1; \
	} \
	return err;


extern thread_id __main_thread_id;


pid_t
getpgrp(void)
{
	return getpgid(__main_thread_id);
}


pid_t
getpid(void)
{
	// this one returns the ID of the main thread
	return __main_thread_id;
}


pid_t
getppid(void)
{
	return _kern_process_info(0, PARENT_ID);
		// this is not supposed to return an error value
}


pid_t
getsid(pid_t process)
{
	pid_t session = _kern_process_info(process, SESSION_ID);

	RETURN_AND_SET_ERRNO(session);
}


pid_t
getpgid(pid_t process)
{
	pid_t group = _kern_process_info(process, GROUP_ID);

	RETURN_AND_SET_ERRNO(group);
}


int
setpgid(pid_t process, pid_t group)
{
	pid_t result = _kern_setpgid(process, group);
	if (result >= 0)
		return 0;

	RETURN_AND_SET_ERRNO(result);
}


pid_t
setpgrp(void)
{
	// setpgrp() never fails -- setpgid() fails when the process is a session
	// leader, though.
	pid_t result = _kern_setpgid(0, 0);
	return result >= 0 ? result : getpgrp();
}


pid_t
setsid(void)
{
	status_t status = _kern_setsid();

	RETURN_AND_SET_ERRNO(status);
}

