/* 
** Copyright 2002-2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>

#include <stdlib.h>
#include <stdio.h>

#include "tls.h"
#include "syscalls.h"


#undef thread_entry
	// thread_entry is still defined in OS.h for compatibility reasons


typedef struct callback_node {
	struct callback_node *next;
	void (*function)(void *);
	void *argument;
} callback_node;


void _thread_do_exit_notification(void);


static int32
thread_entry(thread_func entry, void *data)
{
	int32 returnCode = entry(data);

	_thread_do_exit_notification();

	return returnCode;
}


thread_id
spawn_thread(thread_func entry, const char *name, int32 priority, void *data)
{
	return sys_spawn_thread(thread_entry, name, priority, entry, data);
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
	_thread_do_exit_notification();

	sys_exit(status);
}


status_t
wait_for_thread(thread_id thread, status_t *thread_return_value)
{
	return sys_wait_on_thread(thread, thread_return_value);
}


void
_thread_do_exit_notification(void)
{
	callback_node *node = tls_get(TLS_ON_EXIT_THREAD_SLOT);
	callback_node *next;
	
	while (node != NULL) {
		next = node->next;

		node->function(node->argument);
		free(node);

		node = next;
	}
}


status_t
on_exit_thread(void (*callback)(void *), void *data)
{
	callback_node **head = (callback_node **)tls_address(TLS_ON_EXIT_THREAD_SLOT);

	callback_node *node = malloc(sizeof(callback_node));
	if (node == NULL)
		return B_NO_MEMORY;

	node->function = callback;
	node->argument = data;

	// add this node to the list
	node->next = *head;
	*head = node;

	return B_OK;
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
snooze_etc(bigtime_t timeout, int timeBase, uint32 flags)
{
	return sys_snooze_etc(timeout, timeBase, flags);
}


status_t
snooze(bigtime_t timeout)
{
	return snooze_etc(timeout, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
}


status_t
snooze_until(bigtime_t timeout, int timeBase)
{
	return snooze_etc(timeout, timeBase, B_ABSOLUTE_TIMEOUT);
}

