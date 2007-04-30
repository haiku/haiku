/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_os.h"

#include <OS.h>

#include "fssh_errors.h"


fssh_status_t
fssh_kill_thread(fssh_thread_id thread)
{
	return kill_thread(thread);
}


fssh_status_t
fssh_resume_thread(fssh_thread_id thread)
{
	return resume_thread(thread);
}


fssh_status_t
fssh_suspend_thread(fssh_thread_id thread)
{
	return suspend_thread(thread);
}


fssh_thread_id 
fssh_find_thread(const char *name)
{
	return find_thread(name);
}


fssh_status_t
fssh_snooze(fssh_bigtime_t amount)
{
	return snooze(amount);
}


fssh_status_t
fssh_snooze_until(fssh_bigtime_t time, int timeBase)
{
	return snooze_until(time, timeBase);
}
