/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <stdio.h>

#include "syscalls.h"


thread_id
spawn_thread(thread_func function, const char *name, int32 priority, void *data)
{
	return sys_spawn_thread(function, name, priority, data);
}


status_t
kill_thread(thread_id thread)
{
	return sys_kill_thread(thread);
}


status_t
resume_thread(thread_id thread)
{
	return sys_resume_thread(thread);
}


status_t
suspend_thread(thread_id thread)
{
	return sys_suspend_thread(thread);
}


thread_id
find_thread(const char *name)
{
	// ToDo: fix find_thread()!
	if (!name)
		return sys_get_current_thread_id();

	printf("find_thread(): Not yet implemented!!\n");
	return B_ERROR;
}


#ifdef __INTEL__
// see OS.h for the reason why we need this
thread_id _kfind_thread_(const char *name);

thread_id
_kfind_thread_(const char *name)
{
	// ToDo: _kfind_thread_()
	printf("find_thread(): Not yet implemented!!\n");
	return B_ERROR;
}
#endif


status_t
rename_thread(thread_id thread, const char *name)
{
	// ToDo: rename_thread not implemented
	return B_ERROR;
}


status_t
set_thread_priority(thread_id thread, int32 priority)
{
	// ToDo: set_thread_priority not implemented
	return B_ERROR;
}


void
exit_thread(status_t status)
{
	sys_exit(status);
}


status_t
wait_for_thread(thread_id thread, status_t *thread_return_value)
{
	return sys_wait_on_thread(thread, thread_return_value);
}


status_t on_exit_thread(void (*callback)(void *), void *data)
{
	// ToDo: on_exit_thread not implemented
	return B_ERROR;
}


status_t
_get_thread_info(thread_id thread, thread_info *info, size_t size)
{
	// size parameter is not yet used (but may, if the thread_info structure ever changes)
	(void)size;
	return sys_get_thread_info(thread, info);
}


status_t
_get_next_thread_info(team_id team, int32 *cookie, thread_info *info, size_t size)
{
	// size parameter is not yet used (but may, if the thread_info structure ever changes)
	(void)size;
	return sys_get_next_thread_info(team, cookie, info);
}

/* ToDo: Those are currently defined in syscalls.S - we need some
 * 		consistency here...
 *		send_data(), receive_data(), has_data()

status_t
send_data(thread_id thread, int32 code, const void *buffer, size_t bufferSize)
{
	// ToDo: send_data()
	return B_ERROR;
}


status_t
receive_data(thread_id *sender, void *buffer, size_t bufferSize)
{
	// ToDo: receive_data()
	return B_ERROR;
}


bool
has_data(thread_id thread)
{
	// ToDo: has_data()
	return false;
}
*/

status_t
snooze(bigtime_t microseconds)
{
	return sys_snooze_until(system_time() + microseconds, B_SYSTEM_TIMEBASE);
}


status_t
snooze_until(bigtime_t time, int timeBase)
{
	return sys_snooze_until(time, timeBase);
}


